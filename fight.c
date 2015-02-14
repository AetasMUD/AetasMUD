/**************************************************************************
*  File: fight.c                                           Part of tbaMUD *
*  Usage: Combat system.                                                  *
*                                                                         *
*  All rights reserved.  See license for complete information.            *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
**************************************************************************/

#define __FIGHT_C__

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "interpreter.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "constants.h"
#include "dg_scripts.h"
#include "act.h"
#include "class.h"
#include "fight.h"
#include "shop.h"
#include "quest.h"


/* locally defined global variables, used externally */
/* head of l-list of fighting chars */
struct char_data *combat_list = NULL;
/* Weapon attack texts */
struct attack_hit_type attack_hit_text[] =
{
  {"hit", "hits"},    /* 0 */
  {"sting", "stings"},
  {"whip", "whips"},
  {"slash", "slashes"},
  {"bite", "bites"},
  {"bludgeon", "bludgeons"},  /* 5 */
  {"crush", "crushes"},
  {"pound", "pounds"},
  {"claw", "claws"},
  {"maul", "mauls"},
  {"thrash", "thrashes"}, /* 10 */
  {"pierce", "pierces"},
  {"blast", "blasts"},
  {"punch", "punches"},
  {"stab", "stabs"}
};

/* local (file scope only) variables */
static struct char_data *next_combat_list = NULL;

/* local file scope utility functions */
static void perform_group_gain(struct char_data *ch, int base, struct char_data *victim);
static void dam_message(int dam, struct char_data *ch, struct char_data *victim, int w_type);
static void make_corpse(struct char_data *ch);
static void make_head(struct char_data * ch);
static void change_alignment(struct char_data *ch, struct char_data *victim);
static void group_gain(struct char_data *ch, struct char_data *victim);
static void solo_gain(struct char_data *ch, struct char_data *victim);
void increase_blood(int room);
/** @todo refactor this function name */
static char *replace_string(const char *str, const char *weapon_singular, const char *weapon_plural);
static int compute_thaco(struct char_data *ch, struct char_data *vict);


#define IS_WEAPON(type) (((type) >= TYPE_HIT) && ((type) < TYPE_SUFFERING))
/* The Fight related routines */
bool is_arena_combat(struct char_data *ch, struct char_data *vict)
{
  if (!ch || !ch->in_room)
    return false;

  if (!vict->in_room)
    return false;

  if (ROOM_FLAGGED(vict->in_room, ROOM_ARENA) || ROOM_FLAGGED(ch->in_room, ROOM_ARENA))
    return true;

  if (ZONE_FLAGGED(GET_ROOM_ZONE(vict->in_room), ZONE_ARENA) || ZONE_FLAGGED(GET_ROOM_ZONE(ch->in_room), ZONE_ARENA))
    return true;

  return false;
}

void appear(struct char_data *ch)
{
  if (affected_by_spell(ch, SPELL_INVISIBLE))
    affect_from_char(ch, SPELL_INVISIBLE);

  REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_INVISIBLE);
  REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_HIDE);

  if (GET_LEVEL(ch) < LVL_IMMORT)
    act("$n slowly fades into existence.", FALSE, ch, 0, 0, TO_ROOM);
  else
    act("You feel a strange presence as $n appears, seemingly from nowhere.",
	FALSE, ch, 0, 0, TO_ROOM);
}

int compute_armor_class(struct char_data *ch)
{
  int armorclass = GET_AC(ch);

  if (AWAKE(ch))
    armorclass += dex_app[GET_DEX(ch)].defensive * 10;

  return (MAX(-100, armorclass));      /* -100 is lowest */
}

void update_pos(struct char_data *victim)
{
  if ((GET_HIT(victim) > 0) && (GET_POS(victim) > POS_STUNNED))
    return;
  else if (GET_HIT(victim) > 0)
    GET_POS(victim) = POS_STANDING;
  else if (GET_HIT(victim) <= -11)
    GET_POS(victim) = POS_DEAD;
  else if (GET_HIT(victim) <= -6)
    GET_POS(victim) = POS_MORTALLYW;
  else if (GET_HIT(victim) <= -3)
    GET_POS(victim) = POS_INCAP;
  else
    GET_POS(victim) = POS_STUNNED;
}

void check_killer(struct char_data *ch, struct char_data *vict)
{
  if (PLR_FLAGGED(vict, PLR_KILLER) || PLR_FLAGGED(vict, PLR_THIEF))
    return;
  if (PLR_FLAGGED(ch, PLR_KILLER) || IS_NPC(ch) || IS_NPC(vict) || ch == vict)
    return;

  if (is_arena_combat(ch, vict))
    return;

  SET_BIT_AR(PLR_FLAGS(ch), PLR_KILLER);
  send_to_char(ch, "If you want to be a \tRPLAYER KILLER\tn, so be it...\r\n");
  mudlog(BRF, LVL_IMMORT, TRUE, "PC Killer bit set on %s for initiating attack on %s at %s.",
	    GET_NAME(ch), GET_NAME(vict), world[IN_ROOM(vict)].name);
}

/* start one char fighting another (yes, it is horrible, I know... )  */
void set_fighting(struct char_data *ch, struct char_data *vict)
{
  if (ch == vict)
    return;

  if (FIGHTING(ch)) {
    core_dump();
    return;
  }

  ch->next_fighting = combat_list;
  combat_list = ch;

  if (AFF_FLAGGED(ch, AFF_SLEEP))
    affect_from_char(ch, SPELL_SLEEP);

  FIGHTING(ch) = vict;
  GET_POS(ch) = POS_FIGHTING;

  if (!CONFIG_PK_ALLOWED)
    check_killer(ch, vict);
}

/* remove a char from the list of fighting chars */
void stop_fighting(struct char_data *ch)
{
  struct char_data *temp;

  if (ch == next_combat_list)
    next_combat_list = ch->next_fighting;

  REMOVE_FROM_LIST(ch, combat_list, next_fighting);
  ch->next_fighting = NULL;
  FIGHTING(ch) = NULL;
  GET_POS(ch) = POS_STANDING;
  update_pos(ch);
}

static void make_head(struct char_data * ch)
{
  char buf[MAX_NAME_LENGTH + 64];
  struct obj_data *head;

  head = create_obj();

  head->item_number = NOTHING;
  head->in_room = NOWHERE;
  snprintf(buf, sizeof(buf), "head blood %s", GET_NAME(ch));
  head->name = strdup(buf);

  snprintf(buf, sizeof(buf), "The severed head of %s is lying here.", GET_NAME(ch));
  head->description = strdup(buf);

  snprintf(buf, sizeof(buf), "the severed head of %s", GET_NAME(ch));
  head->short_description = strdup(buf);

  GET_OBJ_TYPE(head) = ITEM_DRINKCON;

  SET_BIT_AR(GET_OBJ_WEAR(head), ITEM_WEAR_TAKE);
  SET_BIT_AR(GET_OBJ_EXTRA(head), ITEM_NODONATE);
  
  GET_OBJ_VAL(head, 0) = 5;
  GET_OBJ_VAL(head, 1) = 5;
  GET_OBJ_VAL(head, 2) = 13;
  GET_OBJ_VAL(head, 3) = 0;
  GET_OBJ_VAL(head, 4) = 1; /* makes it a corpse, so it'll decay */
  
  GET_OBJ_WEIGHT(head) = 20;
  GET_OBJ_RENT(head) = 100000;
  if (IS_NPC(ch))
    GET_OBJ_TIMER(head) = CONFIG_MAX_NPC_CORPSE_TIME;
  else
    GET_OBJ_TIMER(head) = CONFIG_MAX_PC_CORPSE_TIME;

  if (GET_EQ(ch, WEAR_HEAD)) {
    obj_to_room(unequip_char(ch, WEAR_HEAD), ch->in_room);
    GET_EQ(ch, WEAR_HEAD) = NULL;
  }

  obj_to_room(head, ch->in_room);

 }

static void make_corpse(struct char_data *ch)
{
  char keywords[MAX_NAME_LENGTH + 64], shortdesc[MAX_NAME_LENGTH + 64], longdesc[MAX_NAME_LENGTH + 64];
  struct obj_data *corpse, *o;
  struct obj_data *money;
  int i, x, y;

  /*-----DID OUR VICTIM LOSE THEIR HEAD?--------*/

  if (((GET_KILLED_BY(ch, DEATH_ATTACKTYPE) == 303) && (GET_KILLED_BY(ch, DEATH_MSG_NO) == 1)) ||
      ((GET_KILLED_BY(ch, DEATH_ATTACKTYPE) == 302) && (GET_KILLED_BY(ch, DEATH_MSG_NO) == 2))) 
   make_head(ch);

  /*----NOW MAKE THE CORPSE---------------------*/

  corpse = create_obj();

  corpse->item_number = NOTHING;
  IN_ROOM(corpse) = NOWHERE;
  
  /*------ CORPSE SWITCHES ------*/

  if ((GET_KILLED_BY(ch, DEATH_ATTACKTYPE) == 303) && (GET_KILLED_BY(ch, DEATH_MSG_NO) != 1)) {
	snprintf(keywords, sizeof(keywords), "%s chopped %s", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
	snprintf(longdesc, sizeof(longdesc), "The chopped up %s of %s is lying here.", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
	snprintf(shortdesc, sizeof(shortdesc), "the chopped up %s of %s", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
  }
  else if (((GET_KILLED_BY(ch, DEATH_ATTACKTYPE) == 303) && (GET_KILLED_BY(ch, DEATH_MSG_NO) == 1)) ||
           ((GET_KILLED_BY(ch, DEATH_ATTACKTYPE) == 302) && (GET_KILLED_BY(ch, DEATH_MSG_NO) == 2))) {
	snprintf(keywords, sizeof(keywords), "%s headless %s", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
	snprintf(longdesc, sizeof(longdesc), "The headless %s of %s is lying here.", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
	snprintf(shortdesc, sizeof(shortdesc), "the headless %s of %s", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
  }
  else if ((GET_KILLED_BY(ch, DEATH_ATTACKTYPE) == 5) || (GET_KILLED_BY(ch, DEATH_ATTACKTYPE) == 26)) {
	snprintf(keywords, sizeof(keywords), "%s charred %s", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
	snprintf(longdesc, sizeof(longdesc), "The charred %s of %s is lying here.", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
	snprintf(shortdesc, sizeof(shortdesc), "the charred %s of %s", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
  }
  else if (GET_KILLED_BY(ch, DEATH_ATTACKTYPE) == 131) {
	snprintf(keywords, sizeof(keywords), "%s backstabbed %s", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
	snprintf(longdesc, sizeof(longdesc), "The backstabbed %s of %s is lying here.", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
	snprintf(shortdesc, sizeof(shortdesc), "the backstabbed %s of %s", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
  }
  else if (GET_KILLED_BY(ch, DEATH_ATTACKTYPE) == 314) {
	snprintf(keywords, sizeof(keywords), "%s stabbed %s", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
	snprintf(longdesc, sizeof(longdesc), "The stabbed %s of %s is lying here.", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
	snprintf(shortdesc, sizeof(shortdesc), "the stabbed %s of %s", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
  }
  else if (GET_KILLED_BY(ch, DEATH_ATTACKTYPE) == 312) {
	snprintf(keywords, sizeof(keywords), "%s blasted %s", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
	snprintf(longdesc, sizeof(longdesc), "The blasted %s of %s is lying here.", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
	snprintf(shortdesc, sizeof(shortdesc), "the blasted %s of %s", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
  }
  else if (GET_KILLED_BY(ch, DEATH_ATTACKTYPE) == 30) {
	snprintf(keywords, sizeof(keywords), "%s shattered %s", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
	snprintf(longdesc, sizeof(longdesc), "The shattered %s of %s is lying here.", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
	snprintf(shortdesc, sizeof(shortdesc), "the shattered %s of %s", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
  }
  else if (GET_KILLED_BY(ch, DEATH_ATTACKTYPE) == 301) {
	snprintf(keywords, sizeof(keywords), "%s stung %s", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
	snprintf(longdesc, sizeof(longdesc), "The %s of %s is lying here, covered with stings.", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
	snprintf(shortdesc, sizeof(shortdesc), "the stung %s of %s", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
  }
  else if ((GET_KILLED_BY(ch, DEATH_ATTACKTYPE) == 302) && (GET_KILLED_BY(ch, DEATH_MSG_NO) == 1)) {
	snprintf(keywords, sizeof(keywords), "%s welted %s", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
	snprintf(longdesc, sizeof(longdesc), "The %s of %s is lying here, covered with welts.", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
	snprintf(shortdesc, sizeof(shortdesc), "the welted %s of %s", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
  }
  else if (GET_KILLED_BY(ch, DEATH_ATTACKTYPE) == 304) {
	snprintf(keywords, sizeof(keywords), "%s chewed %s", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
	snprintf(longdesc, sizeof(longdesc), "The %s of %s is lying here, covered with bite marks.", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
	snprintf(shortdesc, sizeof(shortdesc), "the chewed %s of %s", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
  }
  else if (GET_KILLED_BY(ch, DEATH_ATTACKTYPE) == 305) {
	snprintf(keywords, sizeof(keywords), "%s bruised %s", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
	snprintf(longdesc, sizeof(longdesc), "The bruised %s of %s is lying here.", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
	snprintf(shortdesc, sizeof(shortdesc), "the bruised %s of %s", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
  }
  else if (GET_KILLED_BY(ch, DEATH_ATTACKTYPE) == 306) {
	snprintf(keywords, sizeof(keywords), "%s crushed %s", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
	snprintf(longdesc, sizeof(longdesc), "The crushed %s of %s is lying here.", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
	snprintf(shortdesc, sizeof(shortdesc), "the crushed %s of %s", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
  }
  else if (GET_KILLED_BY(ch, DEATH_ATTACKTYPE) == 307) {
	snprintf(keywords, sizeof(keywords), "%s battered %s", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
	snprintf(longdesc, sizeof(longdesc), "The %s of %s is lying here, severely battered.", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
	snprintf(shortdesc, sizeof(shortdesc), "the battered %s of %s", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
  }
  else if (GET_KILLED_BY(ch, DEATH_ATTACKTYPE) == 308) {
	snprintf(keywords, sizeof(keywords), "%s shredded %s", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
	snprintf(longdesc, sizeof(longdesc), "The %s of %s is lying here in shreds.", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
	snprintf(shortdesc, sizeof(shortdesc), "the shredded %s of %s", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
  }
  else if (GET_KILLED_BY(ch, DEATH_ATTACKTYPE) == 309) {
	snprintf(keywords, sizeof(keywords), "%s mauled %s", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
	snprintf(longdesc, sizeof(longdesc), "The mauled %s of %s is lying here.", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
	snprintf(shortdesc, sizeof(shortdesc), "the mauled %s of %s", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
  }
  else if (GET_KILLED_BY(ch, DEATH_ATTACKTYPE) == 310) {
	snprintf(keywords, sizeof(keywords), "%s thrashed %s", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
	snprintf(longdesc, sizeof(longdesc), "The thrashed %s of %s is lying here.", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
	snprintf(shortdesc, sizeof(shortdesc), "the thrashed %s of %s", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
  }
  else if (GET_KILLED_BY(ch, DEATH_ATTACKTYPE) == 311) {
	snprintf(keywords, sizeof(keywords), "%s pierced %s", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
	snprintf(longdesc, sizeof(longdesc), "The %s of %s is lying here, full of holes.", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
	snprintf(shortdesc, sizeof(shortdesc), "the pierced %s of %s", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
  }
  else {
	snprintf(keywords, sizeof(keywords), "%s %s", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
	snprintf(longdesc, sizeof(longdesc), "The %s of %s is lying here.", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
	snprintf(shortdesc, sizeof(shortdesc), "the %s of %s", IS_UNDEAD(ch) ? "remains" : "corpse", GET_NAME(ch));
  }
  
  corpse->name = strdup(keywords);
  corpse->description = strdup(longdesc);
  corpse->short_description = strdup(shortdesc);
  
  /*-----CORPSE CREATION CONSTANTS-----------------*/

  GET_OBJ_TYPE(corpse) = ITEM_CONTAINER;
  for(x = y = 0; x < EF_ARRAY_MAX || y < TW_ARRAY_MAX; x++, y++) {
    if (x < EF_ARRAY_MAX)
      GET_OBJ_EXTRA_AR(corpse, x) = 0;
    if (y < TW_ARRAY_MAX)
      corpse->obj_flags.wear_flags[y] = 0;
  }
  SET_BIT_AR(GET_OBJ_WEAR(corpse), ITEM_WEAR_TAKE);
  SET_BIT_AR(GET_OBJ_EXTRA(corpse), ITEM_NODONATE);
  GET_OBJ_VAL(corpse, 0) = 0;	/* You can't store stuff in a corpse */
  GET_OBJ_VAL(corpse, 3) = 1;	/* corpse identifier */
  GET_OBJ_RENT(corpse) = 100000;
  
  /* if the death occurs in an arena room or zone, the victim keeps his stuff */
  /* normally we would have a ch and a vict, but here we just need to know the dead guy */
  if (!IS_NPC(ch) && is_arena_combat(ch, ch)) {
    GET_OBJ_WEIGHT(corpse) = GET_WEIGHT(ch);
    GET_OBJ_TIMER(corpse) = CONFIG_MAX_NPC_CORPSE_TIME;
  /* else all of his stuff ends up in his corpse */
  } else {
    GET_OBJ_WEIGHT(corpse) = GET_WEIGHT(ch) + IS_CARRYING_W(ch);
  
    if (IS_NPC(ch))
      GET_OBJ_TIMER(corpse) = CONFIG_MAX_NPC_CORPSE_TIME;
    else
      GET_OBJ_TIMER(corpse) = CONFIG_MAX_PC_CORPSE_TIME;

    /* transfer character's inventory to the corpse */
    corpse->contains = ch->carrying;
    for (o = corpse->contains; o != NULL; o = o->next_content)
      o->in_obj = corpse;
  
    object_list_new_owner(corpse, NULL);

    /* transfer character's equipment to the corpse */
    for (i = 0; i < NUM_WEARS; i++)
      if (GET_EQ(ch, i)) {
        remove_otrigger(GET_EQ(ch, i), ch);
        obj_to_obj(unequip_char(ch, i), corpse);
      }

    /* transfer gold */
    if (GET_GOLD(ch) > 0) {
      /* following 'if' clause added to fix gold duplication loophole. The above
       * line apparently refers to the old "partially log in, kill the game
       * character, then finish login sequence" duping bug. The duplication has
       * been fixed (knock on wood) but the test below shall live on, for a
       * while. -gg 3/3/2002 */
      if (IS_NPC(ch) || ch->desc) {
        money = create_money(GET_GOLD(ch));
        obj_to_obj(money, corpse);
      }
      GET_GOLD(ch) = 0;
    }
    ch->carrying = NULL;
    IS_CARRYING_N(ch) = 0;
    IS_CARRYING_W(ch) = 0;
    }
  obj_to_room(corpse, IN_ROOM(ch));
}

/* When ch kills victim */
static void change_alignment(struct char_data *ch, struct char_data *victim)
{
  /* new alignment change algorithm: if you kill a monster with alignment A,
   * you move 1/16th of the way to having alignment -A.  Simple and fast. */
  /*GET_ALIGNMENT(ch) += (-GET_ALIGNMENT(victim) - GET_ALIGNMENT(ch)) / 20;*/
  if (GET_ALIGNMENT(ch) < 0 && GET_ALIGNMENT(victim) > 0) 
	GET_ALIGNMENT(ch) = MAX(-1000, (GET_ALIGNMENT(ch) - ((GET_ALIGNMENT(victim) - GET_ALIGNMENT(ch))/190)));

  if (GET_ALIGNMENT(ch) > 0 && GET_ALIGNMENT(victim) < 0)
	GET_ALIGNMENT(ch) = MIN(1000, (GET_ALIGNMENT(ch) + (GET_ALIGNMENT(ch) - GET_ALIGNMENT(victim)/190)));

  if (GET_ALIGNMENT(ch) > 0 && GET_ALIGNMENT(victim) > 0) 
	GET_ALIGNMENT(ch) -= (GET_ALIGNMENT(ch) + GET_ALIGNMENT(victim))/190;
 
  if (GET_ALIGNMENT(ch) < 0 && GET_ALIGNMENT(victim) < 0) 
	GET_ALIGNMENT(ch) += (-GET_ALIGNMENT(ch) + -GET_ALIGNMENT(victim))/190;

  if (GET_ALIGNMENT(ch) == 0 && GET_ALIGNMENT(victim) < 0)
	GET_ALIGNMENT(ch) = GET_ALIGNMENT(ch) + MAX(1, (-GET_ALIGNMENT(victim) / 190));

  if (GET_ALIGNMENT(ch) == 0 && GET_ALIGNMENT(victim) > 0)
	GET_ALIGNMENT(ch) = GET_ALIGNMENT(ch) - MAX(1, (GET_ALIGNMENT(victim) / 190));

  if (GET_ALIGNMENT(ch) > 0 && GET_ALIGNMENT(victim) == 0) 
	GET_ALIGNMENT(ch) = MIN(1000, (GET_ALIGNMENT(ch) + (GET_ALIGNMENT(ch)/190)));
 
  if (GET_ALIGNMENT(ch) < 0 && GET_ALIGNMENT(victim) == 0) 
	GET_ALIGNMENT(ch) = MAX(-1000, (GET_ALIGNMENT(ch) - (GET_ALIGNMENT(ch)/190)));

}

void death_cry(struct char_data *ch)
{
  int door;

  if (IS_UNDEAD(ch)) {
   act("$n shrieks loudly as $s animated body collapses into a heap.", FALSE, ch, 0, 0, TO_ROOM);

  for (door = 0; door < DIR_COUNT; door++)
   if (CAN_GO(ch, door))
    send_to_room(world[IN_ROOM(ch)].dir_option[door]->to_room, "Your blood freezes as you hear a horrible shriek.\r\n");
  }
  
  else {
   act("Your blood freezes as you hear $n's death cry.", FALSE, ch, 0, 0, TO_ROOM);

  for (door = 0; door < NUM_OF_DIRS; door++)
    if (CAN_GO(ch, door))
      send_to_room(world[IN_ROOM(ch)].dir_option[door]->to_room, "Your blood freezes as you hear someone's death cry.\r\n");
  }
}

void raw_kill(struct char_data * ch, struct char_data * killer)
{
  if (FIGHTING(ch))
    stop_fighting(ch);

  while (ch->affected)
    affect_remove(ch, ch->affected);

  /* To make ordinary commands work in scripts.  welcor*/
  GET_POS(ch) = POS_STANDING;

  if (killer) {
    if (death_mtrigger(ch, killer))
      death_cry(ch);
  } else
    death_cry(ch);

  if (killer)
    autoquest_trigger_check(killer, ch, NULL, AQ_MOB_KILL);

  /* Alert Group if Applicable */
  if (GROUP(ch))
    send_to_group(ch, GROUP(ch), "%s has died.\r\n", GET_NAME(ch));

  update_pos(ch);

  make_corpse(ch);
  
  /* if the victim died in arena, crashsave him */
  if (!IS_NPC(ch) && is_arena_combat(killer, ch))
    Crash_rentsave(ch, 0);

  extract_char(ch);

  if (killer) {
    autoquest_trigger_check(killer, NULL, NULL, AQ_MOB_SAVE);
    autoquest_trigger_check(killer, NULL, NULL, AQ_ROOM_CLEAR);
  }
}

void die(struct char_data * ch, struct char_data * killer)
{
  int soul;
 
  if (!IS_NPC(ch) && !is_arena_combat(killer, ch)) {
    gain_exp(ch, -(GET_EXP(ch) / 2));
    soul = (GET_SOUL_POINTS(ch) - (GET_LEVEL(ch) / 4));

    if ((soul < 0) && (GET_LEVEL(ch) > 15))
      GET_MAX_HIT(ch) = MAX(50, (GET_MAX_HIT(ch) * .9));

    GET_SOUL_POINTS(ch) = MAX(0, soul);
    REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_KILLER);
    REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_THIEF);
  }
  raw_kill(ch, killer);
}

static void perform_group_gain(struct char_data *ch, int base,
			     struct char_data *victim)
{
  int share, hap_share;

  share = MIN(CONFIG_MAX_EXP_GAIN, MAX(1, base));
  
  if ((IS_HAPPYHOUR) && (IS_HAPPYEXP))
  {
    /* This only reports the correct amount - the calc is done in gain_exp */
    hap_share = share + (int)((float)share * ((float)HAPPY_EXP / (float)(100)));
    share = MIN(CONFIG_MAX_EXP_GAIN, MAX(1, hap_share));
  }

  /* if he is in arena, award only 1 exp */
  if (is_arena_combat(ch, victim))
    share = 1;
  
  if (GET_LEVEL(ch) <= 49) {
   if (share > 1)
    send_to_char(ch, "\tYYou receive your share of experience -- %d points.\tn\r\n", share);
   else
    send_to_char(ch, "\tYYou receive your share of experience -- one measly little point!\tn\r\n");
	}

  gain_exp(ch, share);
  change_alignment(ch, victim);
}

static void group_gain(struct char_data *ch, struct char_data *victim)
{
  int tot_members = 0, base, tot_gain;
  struct char_data *k;

  while ((k = (struct char_data *) simple_list(GROUP(ch)->members)) != NULL)
    if (IN_ROOM(ch) == IN_ROOM(k))
      tot_members++;

  /* round up to the nearest tot_members */
  tot_gain = (GET_EXP(victim) / 3) + tot_members - 1;

  /* prevent illegal xp creation when killing players */
  if (!IS_NPC(victim))
    tot_gain = MIN(CONFIG_MAX_EXP_LOSS * 2 / 3, tot_gain);

  if (tot_members >= 1)
    base = MAX(1, tot_gain / tot_members);
  else
    base = 0;

  while ((k = (struct char_data *) simple_list(GROUP(ch)->members)) != NULL)
    if (IN_ROOM(k) == IN_ROOM(ch))
    perform_group_gain(k, base, victim);
}

static void solo_gain(struct char_data *ch, struct char_data *victim)
{
  int exp, happy_exp;

  exp = MIN(CONFIG_MAX_EXP_GAIN, GET_EXP(victim) / 3);

  /* Calculate level-difference bonus */
  if (IS_NPC(ch))
    exp += MAX(0, (exp * MIN(4, (GET_LEVEL(victim) - GET_LEVEL(ch)))) / 8);
  else
    exp += MAX(0, (exp * MIN(8, (GET_LEVEL(victim) - GET_LEVEL(ch)))) / 8);

  exp = MAX(exp, 1);
  
  if (IS_HAPPYHOUR && IS_HAPPYEXP) {
    happy_exp = exp + (int)((float)exp * ((float)HAPPY_EXP / (float)(100)));
    exp = MAX(happy_exp, 1);
  }

  /* if he is in arena, award only 1 exp */
  if (is_arena_combat(ch, victim))
    exp = 1;
  
  if (GET_LEVEL(ch) <= 49) {
   if (exp > 1)
    send_to_char(ch, "\tYYou receive %d experience points.\tn\r\n", exp);
   else
    send_to_char(ch, "\tYYou receive one lousy experience point.\tn\r\n");
}

  gain_exp(ch, exp);
  change_alignment(ch, victim);
}

static char *replace_string(const char *str, const char *weapon_singular, const char *weapon_plural)
{
  static char buf[256];
  char *cp = buf;

  for (; *str; str++) {
    if (*str == '#') {
      switch (*(++str)) {
      case 'W':
	for (; *weapon_plural; *(cp++) = *(weapon_plural++));
	break;
      case 'w':
	for (; *weapon_singular; *(cp++) = *(weapon_singular++));
	break;
      default:
	*(cp++) = '#';
	break;
      }
    } else
      *(cp++) = *str;

    *cp = 0;
  }				/* For */

  return (buf);
}

/* increases the blood */
void increase_blood(int room)
{
 RM_BLOOD(room) = MIN(RM_BLOOD(room) + 1, 16);
}

/* message for doing damage with a weapon */
static void dam_message(int dam, struct char_data *ch, struct char_data *victim,
		      int w_type)
{
  char *buf;
  int msgnum;
  int bleed = 0;

  static struct dam_weapon_type {
    const char *to_room;
    const char *to_char;
    const char *to_victim;
  } dam_weapons[] = {

    /* use #w for singular (i.e. "slash") and #W for plural (i.e. "slashes") */

    {
      "$n tries to #w $N, but misses.",	/* 0: 0     */
      "You try to #w $N, but miss.",
      "$n tries to #w you, but misses."
    },

    {
      "$n tickles $N as $e #W $M.",	/* 1: 1..2  */
      "You tickle $N as you #w $M.",
      "$n tickles you as $e #W you."
    },

    {
      "$n barely #W $N.",		/* 2: 3..4  */
      "You barely #w $N.",
      "$n barely #W you."
    },

    {
      "$n #W $N.",			/* 3: 5..6  */
      "You #w $N.",
      "$n #W you."
    },

    {
      "$n #W $N hard.",			/* 4: 7..10  */
      "You #w $N hard.",
      "$n #W you hard."
    },

    {
      "$n #W $N very hard.",		/* 5: 11..14  */
      "You #w $N very hard.",
      "$n #W you very hard."
    },

    {
      "$n #W $N extremely hard.",	/* 6: 15..19  */
      "You #w $N extremely hard.",
      "$n #W you extremely hard."
    },

    {
      "$n massacres $N to small fragments with $s #w.",	/* 7: 19..23 */
      "You massacre $N to small fragments with your #w.",
      "$n massacres you to small fragments with $s #w."
    },

    {
      "$n OBLITERATES $N with $s deadly #w!!",	/* 8: > 23   */
      "You OBLITERATE $N with your deadly #w!!",
      "$n OBLITERATES you with $s deadly #w!!"
    }
  };

  w_type -= TYPE_HIT;		/* Change to base of table with text */

  if (dam == 0)		msgnum = 0;
  else if (dam <= 2)    msgnum = 1;
  else if (dam <= 4)    msgnum = 2;
  else if (dam <= 6)    msgnum = 3;
  else if (dam <= 10)   msgnum = 4;
  else if (dam <= 14)   msgnum = 5;
  else if (dam <= 19)   msgnum = 6;
  else if (dam <= 23)   msgnum = 7;
  else			msgnum = 8;

  /* damage message to onlookers */
  buf = replace_string(dam_weapons[msgnum].to_room,
	  attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
  act(buf, FALSE, ch, NULL, victim, TO_NOTVICT);

  /* damage message to damager */
  if (GET_LEVEL(ch) >= LVL_IMMORT)
	send_to_char(ch, "(%d) ", dam);
  send_to_char(ch, CCYEL(ch, C_SPR));
  buf = replace_string(dam_weapons[msgnum].to_char,
	  attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
  act(buf, FALSE, ch, NULL, victim, TO_CHAR);
  send_to_char(ch, CCNRM(ch, C_SPR));

  /* damage message to damagee */
  if (GET_LEVEL(victim) >= LVL_IMMORT)
    send_to_char(victim, "\tR(%d)", dam);
  send_to_char(victim, CCRED(ch, C_SPR));
  buf = replace_string(dam_weapons[msgnum].to_victim,
	  attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
  act(buf, FALSE, ch, NULL, victim, TO_VICT | TO_SLEEP);
  send_to_char(victim, CCNRM(victim, C_SPR));
  
  /* LET THE BLOOD FLY! */ 
 if (dam >= 14) {
  if (IS_NPC(victim) && (GET_RACE(victim) == RACE_UNDEAD));
 else {
  bleed = rand_number(0, 100);
   if (bleed >= 90) {

 sprintf(buf,
  "\trA stream of your blood sprays into the air as a result of $n's %s!\tn",
   attack_hit_text[w_type].singular);
   act(buf, FALSE, ch, NULL, victim, TO_VICT | TO_SLEEP);

 sprintf(buf, 
  "A stream of $n's blood sprays into the air as a result of $N's %s!", 
   attack_hit_text[w_type].singular);
   act(buf, FALSE, victim, NULL, ch, TO_NOTVICT); 

 sprintf(buf,
  "\trA stream of $N's blood sprays into the air as a result of your %s!\tn",
   attack_hit_text[w_type].singular);
   act(buf, FALSE, ch, NULL, victim, TO_CHAR);

  increase_blood(victim->in_room);
   }
  }
 }
}

/*  message for doing damage with a spell or skill. Also used for weapon
 *  damage on miss and death blows. */
int skill_message(int dam, struct char_data *ch, struct char_data *vict,
		      int attacktype)
{
  int i, j, nr;
  struct message_type *msg;

  struct obj_data *weap = GET_EQ(ch, WEAR_WIELD);
  
  /* @todo restructure the messages library to a pointer based system as
   * opposed to the current cyclic location system. */

  for (i = 0; i < MAX_MESSAGES; i++) {
    if (fight_messages[i].a_type == attacktype) {
      nr = dice(1, fight_messages[i].number_of_attacks);
	  GET_KILLED_BY(vict, DEATH_MSG_NO) = nr;
      for (j = 1, msg = fight_messages[i].msg; (j < nr) && msg; j++)
	msg = msg->next;

      if (!IS_NPC(vict) && (GET_LEVEL(vict) >= LVL_IMPL)) {
	act(msg->god_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
	act(msg->god_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT);
	act(msg->god_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
      } else if (dam != 0) {
        /*
         * Don't send redundant color codes for TYPE_SUFFERING & other types
         * of damage without attacker_msg.
         */
	if (GET_POS(vict) == POS_DEAD) {
          if (msg->die_msg.attacker_msg) {
            send_to_char(ch, CCYEL(ch, C_CMP));
            act(msg->die_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
            send_to_char(ch, CCNRM(ch, C_CMP));
          }

	  send_to_char(vict, CBRED(vict, C_CMP));
	  act(msg->die_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
	  send_to_char(vict, CCNRM(vict, C_CMP));

	  act(msg->die_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
	} else {
          if (msg->hit_msg.attacker_msg) {
	    send_to_char(ch, CCYEL(ch, C_CMP));
	    act(msg->hit_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
	    send_to_char(ch, CCNRM(ch, C_CMP));
          }

	  send_to_char(vict, CBRED(vict, C_CMP));
	  act(msg->hit_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
	  send_to_char(vict, CCNRM(vict, C_CMP));

	  act(msg->hit_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
	}
      } else if (ch != vict) {	/* Dam == 0 */
        if (msg->miss_msg.attacker_msg) {
	  send_to_char(ch, CCYEL(ch, C_CMP));
	  act(msg->miss_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
	  send_to_char(ch, CCNRM(ch, C_CMP));
        }

	send_to_char(vict, CCRED(vict, C_CMP));
	act(msg->miss_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
	send_to_char(vict, CCNRM(vict, C_CMP));

	act(msg->miss_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
      }
      return (1);
    }
  }
  return (0);
}

/* This function returns the following codes:
 *	< 0	Victim died.
 *	= 0	No damage.
 *	> 0	How much damage done. */
int damage(struct char_data *ch, struct char_data *victim, int dam, int attacktype)
{
  long local_gold = 0, happy_gold = 0;
  char local_buf[256];
  struct char_data *tmp_char;
  struct obj_data *corpse_obj;
  struct obj_data *remains_obj;

  if (GET_POS(victim) <= POS_DEAD) {
    /* This is "normal"-ish now with delayed extraction. -gg 3/15/2001 */
    if (PLR_FLAGGED(victim, PLR_NOTDEADYET) || MOB_FLAGGED(victim, MOB_NOTDEADYET))
      return (-1);

    log("SYSERR: Attempt to damage corpse '%s' in room #%d by '%s'.",
		GET_NAME(victim), GET_ROOM_VNUM(IN_ROOM(victim)), GET_NAME(ch));
    die(victim, ch);
    return (-1);			/* -je, 7/7/92 */
  }

  /* peaceful rooms */
  if (ch->nr != real_mobile(DG_CASTER_PROXY) &&
      ch != victim && ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return (0);
  }

  /* shopkeeper and MOB_NOKILL protection */
  if (!ok_damage_shopkeeper(ch, victim) || MOB_FLAGGED(victim, MOB_NOKILL)) {
    send_to_char(ch, "This mob is protected.\r\n");
    return (0);
  }

  /* You can't damage an immortal! */
  if (!IS_NPC(victim) && ((GET_LEVEL(victim) >= LVL_IMMORT) && PRF_FLAGGED(victim, PRF_NOHASSLE)))
    dam = 0;

  if (victim != ch) {
    /* Start the attacker fighting the victim */
    if (GET_POS(ch) > POS_STUNNED && (FIGHTING(ch) == NULL))
      set_fighting(ch, victim);

    /* Start the victim fighting the attacker */
    if (GET_POS(victim) > POS_STUNNED && (FIGHTING(victim) == NULL)) {
      set_fighting(victim, ch);
      if (MOB_FLAGGED(victim, MOB_MEMORY) && !IS_NPC(ch))
	remember(victim, ch);
    }
  }

  /* If you attack a pet, it hates your guts */
  if (victim->master == ch)
    stop_follower(victim);

  /* If the attacker is invisible, he becomes visible */
  if (AFF_FLAGGED(ch, AFF_INVISIBLE) || AFF_FLAGGED(ch, AFF_HIDE))
    appear(ch);

  /* Cut damage in half if victim has sanct, to a minimum 1 */
  if (AFF_FLAGGED(victim, AFF_SANCTUARY) && dam >= 2)
    dam /= 2;

  /* Check for PK if this is not a PK MUD */
  if (!CONFIG_PK_ALLOWED) {
    check_killer(ch, victim);
    if (PLR_FLAGGED(ch, PLR_KILLER) && (ch != victim))
      dam = 0;
  }

  /* Set the maximum damage per round and subtract the hit points */
  dam = MAX(MIN(dam, 100), 0);
  GET_HIT(victim) -= dam;

  /* Gain exp for the hit */
  if (ch != victim && !is_arena_combat(ch, victim))
    gain_exp(ch, GET_LEVEL(victim) * dam);

  update_pos(victim);

  /* skill_message sends a message from the messages file in lib/misc.
   * dam_message just sends a generic "You hit $n extremely hard.".
   * skill_message is preferable to dam_message because it is more
   * descriptive.
   *
   * If we are _not_ attacking with a weapon (i.e. a spell), always use
   * skill_message. If we are attacking with a weapon: If this is a miss or a
   * death blow, send a skill_message if one exists; if not, default to a
   * dam_message. Otherwise, always send a dam_message. */
  if (!IS_WEAPON(attacktype))
    skill_message(dam, ch, victim, attacktype);
  else {
    if (GET_POS(victim) == POS_DEAD || dam == 0) {
      if (!skill_message(dam, ch, victim, attacktype))
	dam_message(dam, ch, victim, attacktype);
    } else {
      dam_message(dam, ch, victim, attacktype);
    }
  }
 
  GET_KILLED_BY(victim, DEATH_ATTACKTYPE) = attacktype;

  /* Use send_to_char -- act() doesn't send message if you are DEAD. */
  switch (GET_POS(victim)) {
  case POS_MORTALLYW:
    act("$n is mortally wounded, and will die soon, if not aided.", TRUE, victim, 0, 0, TO_ROOM);
    send_to_char(victim, "You are mortally wounded, and will die soon, if not aided.\r\n");
    break;
  case POS_INCAP:
    act("$n is incapacitated and will slowly die, if not aided.", TRUE, victim, 0, 0, TO_ROOM);
    send_to_char(victim, "You are incapacitated and will slowly die, if not aided.\r\n");
    break;
  case POS_STUNNED:
    act("$n is stunned, but will probably regain consciousness again.", TRUE, victim, 0, 0, TO_ROOM);
    send_to_char(victim, "You're stunned, but will probably regain consciousness again.\r\n");
    break;
  case POS_DEAD:
    act("$n is dead!  R.I.P.", FALSE, victim, 0, 0, TO_ROOM);
    send_to_char(victim, "You are dead!  Sorry...\r\n");
    break;

  default:			/* >= POSITION SLEEPING */
    if (dam > (GET_MAX_HIT(victim) / 4))
      send_to_char(victim, "That really did HURT!\r\n");

    if (GET_HIT(victim) < (GET_MAX_HIT(victim) / 10)) {
      send_to_char(victim, "%sYou wish that your wounds would stop BLEEDING so much!%s\r\n",
		CCRED(victim, C_SPR), CCNRM(victim, C_SPR));
      if (ch != victim && MOB_FLAGGED(victim, MOB_WIMPY))
	do_flee(victim, NULL, 0, 0);
    }
    if (!IS_NPC(victim) && GET_WIMP_LEV(victim) && (victim != ch) &&
	GET_HIT(victim) < GET_WIMP_LEV(victim) && GET_HIT(victim) > 0) {
      send_to_char(victim, "You wimp out, and attempt to flee!\r\n");
      do_flee(victim, NULL, 0, 0);
    }
    break;
  }

  /* Help out poor linkless people who are attacked */
  if (!IS_NPC(victim) && !(victim->desc) && GET_POS(victim) > POS_STUNNED) {
    do_flee(victim, NULL, 0, 0);
    if (!FIGHTING(victim)) {
      act("$n is rescued by divine forces.", FALSE, victim, 0, 0, TO_ROOM);
      GET_WAS_IN(victim) = IN_ROOM(victim);
      char_from_room(victim);
      char_to_room(victim, 0);
    }
  }

  /* stop someone from fighting if they're stunned or worse */
  if (GET_POS(victim) <= POS_STUNNED && FIGHTING(victim) != NULL)
    stop_fighting(victim);

  /* Uh oh.  Victim died. */
  if (GET_POS(victim) == POS_DEAD) {
    if (ch != victim && (IS_NPC(victim) || victim->desc)) {
      if (GROUP(ch))
	group_gain(ch, victim);
      else
        solo_gain(ch, victim);
    }

    if (!IS_NPC(victim)) {
      if (is_arena_combat(ch, victim))
        mudlog(BRF, LVL_IMMORT, TRUE, "[ARENA] %s killed by %s at %s", GET_NAME(victim), GET_NAME(ch), world[IN_ROOM(victim)].name);
      else
        mudlog(BRF, LVL_IMMORT, TRUE, "%s killed by %s at %s", GET_NAME(victim), GET_NAME(ch), world[IN_ROOM(victim)].name);
      if (MOB_FLAGGED(ch, MOB_MEMORY))
	    forget(ch, victim);
    }
    /* Cant determine GET_GOLD on corpse, so do now and store */
    if (IS_NPC(victim)) {
      if ((IS_HAPPYHOUR) && (IS_HAPPYGOLD))
      {
        happy_gold = (long)(GET_GOLD(victim) * (((float)(HAPPY_GOLD))/(float)100));
        happy_gold = MAX(0, happy_gold);
        increase_gold(victim, happy_gold);
      }
      local_gold = GET_GOLD(victim);
      sprintf(local_buf,"%ld", (long)local_gold);
    }
	
	if (!IS_NPC(victim)) {
      if (is_arena_combat(ch, victim))
        GET_ARENA_RIP_CNT(victim) += 1;
      else
        GET_RIP_CNT(victim) += 1;
    }
    if (!IS_NPC(ch)) {
      if (is_arena_combat(ch, victim))
        GET_ARENA_KILL_CNT(ch) += 1;
      else
        GET_KILL_CNT(ch) += 1;
    }
	  
	if (IS_NPC(victim) && (GET_RACE(victim) != RACE_UNDEAD)) 
        increase_blood(victim->in_room);
 
    die(victim, ch);
    if (GROUP(ch) && (local_gold > 0) && PRF_FLAGGED(ch, PRF_AUTOSPLIT) ) {
      generic_find("corpse", FIND_OBJ_ROOM, ch, &tmp_char, &corpse_obj);
	  generic_find("remains", FIND_OBJ_ROOM, ch, &tmp_char, &remains_obj);
      if (corpse_obj) {
        do_get(ch, "all.coin corpse", 0, 0);
        do_split(ch, local_buf, 0, 0);
      }
	  if (remains_obj) {
        do_get(ch, "all.coin remains", 0, 0);
        do_split(ch, local_buf, 0, 0);
      }
      /* need to remove the gold from the corpse */
    } else if (!IS_NPC(ch) && (ch != victim) && PRF_FLAGGED(ch, PRF_AUTOGOLD)) {
	   generic_find("corpse", FIND_OBJ_ROOM, ch, &tmp_char, &corpse_obj);
	   generic_find("remains", FIND_OBJ_ROOM, ch, &tmp_char, &remains_obj);
      if (corpse_obj)
        do_get(ch, "all.coin corpse", 0, 0);
	  if (remains_obj)
        do_get(ch, "all.coin remains", 0, 0);
     }
    if (!IS_NPC(ch) && (ch != victim) && PRF_FLAGGED(ch, PRF_AUTOLOOT)) {
	   generic_find("corpse", FIND_OBJ_ROOM, ch, &tmp_char, &corpse_obj);
	   generic_find("remains", FIND_OBJ_ROOM, ch, &tmp_char, &remains_obj);
      if (corpse_obj)
        do_get(ch, "all corpse", 0, 0);
	  if (remains_obj)
        do_get(ch, "all remains", 0, 0);
      }
    if (IS_NPC(victim) && !IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOSAC)) {
	   generic_find("corpse", FIND_OBJ_ROOM, ch, &tmp_char, &corpse_obj);
	   generic_find("remains", FIND_OBJ_ROOM, ch, &tmp_char, &remains_obj);
      if (corpse_obj)
        do_sac(ch, "corpse", 0, 0);
	  if (remains_obj)
        do_sac(ch, "remains", 0, 0);
      }
    return (-1);
  }
  return (dam);
}

/* Calculate the THAC0 of the attacker. 'victim' currently isn't used but you
 * could use it for special cases like weapons that hit evil creatures easier
 * or a weapon that always misses attacking an animal. */
static int compute_thaco(struct char_data *ch, struct char_data *victim)
{
  int calc_thaco;

  if (!IS_NPC(ch))
    calc_thaco = thaco(GET_CLASS(ch), GET_LEVEL(ch));
  else		/* THAC0 for monsters is set in the HitRoll */
    calc_thaco = 20;
  calc_thaco -= str_app[STRENGTH_APPLY_INDEX(ch)].tohit;
  calc_thaco -= GET_HITROLL(ch);
  calc_thaco -= (int) ((GET_INT(ch) - 13) / 1.5);	/* Intelligence helps! */
  calc_thaco -= (int) ((GET_WIS(ch) - 13) / 1.5);	/* So does wisdom */

  return calc_thaco;
}

void hit(struct char_data *ch, struct char_data *victim, int type)
{
  struct obj_data *wielded = GET_EQ(ch, WEAR_WIELD);
  int w_type, victim_ac, calc_thaco, dam, diceroll;

  /* Check that the attacker and victim exist */
  if (!ch || !victim) return;

  /* check if the character has a fight trigger */
  fight_mtrigger(ch);

  /* Do some sanity checking, in case someone flees, etc. */
  if (IN_ROOM(ch) != IN_ROOM(victim)) {
    if (FIGHTING(ch) && FIGHTING(ch) == victim)
      stop_fighting(ch);
    return;
  }

  /* Find the weapon type (for display purposes only) */
  if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON)
    w_type = GET_OBJ_VAL(wielded, 3) + TYPE_HIT;
  else {
    if (IS_NPC(ch) && ch->mob_specials.attack_type != 0)
      w_type = ch->mob_specials.attack_type + TYPE_HIT;
    else
      w_type = TYPE_HIT;
  }

  /* Calculate chance of hit. Lower THAC0 is better for attacker. */
  calc_thaco = compute_thaco(ch, victim);

  /* Calculate the raw armor including magic armor.  Lower AC is better for defender. */
  victim_ac = compute_armor_class(victim) / 10;

  /* roll the die and take your chances... */
  diceroll = rand_number(1, 20);
  
  /* report for debugging if necessary */
  if (CONFIG_DEBUG_MODE >= NRM)
    send_to_char(ch, "\t1Debug:\r\n   \t2Thaco: \t3%d\r\n   \t2AC: \t3%d\r\n   \t2Diceroll: \t3%d\tn\r\n", 
      calc_thaco, victim_ac, diceroll);

  /* Decide whether this is a hit or a miss.
   *  Victim asleep = hit, otherwise:
   *     1   = Automatic miss.
   *   2..19 = Checked vs. AC.
   *    20   = Automatic hit. */
  if (diceroll == 20 || !AWAKE(victim))
    dam = TRUE;
  else if (diceroll == 1)
    dam = FALSE;
  else
    dam = (calc_thaco - diceroll <= victim_ac);

  if (!dam)
    /* the attacker missed the victim */
    damage(ch, victim, 0, type == SKILL_BACKSTAB ? SKILL_BACKSTAB : w_type);
  else {
    /* okay, we know the guy has been hit.  now calculate damage.
     * Start with the damage bonuses: the damroll and strength apply */
    dam = str_app[STRENGTH_APPLY_INDEX(ch)].todam;
    dam += GET_DAMROLL(ch);

    /* Maybe holding arrow? */
    if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON) {
      /* Add weapon-based damage if a weapon is being wielded */
      dam += dice(GET_OBJ_VAL(wielded, 1), GET_OBJ_VAL(wielded, 2));
    } else {
      /* If no weapon, add bare hand damage instead */
      if (IS_NPC(ch))
	dam += dice(ch->mob_specials.damnodice, ch->mob_specials.damsizedice);
      else
	dam += rand_number(0, 2);	/* Max 2 bare hand damage for players */
    }

    /* Include a damage multiplier if victim isn't ready to fight:
     * Position sitting  1.33 x normal
     * Position resting  1.66 x normal
     * Position sleeping 2.00 x normal
     * Position stunned  2.33 x normal
     * Position incap    2.66 x normal
     * Position mortally 3.00 x normal
     * Note, this is a hack because it depends on the particular
     * values of the POSITION_XXX constants. */
    if (GET_POS(victim) < POS_FIGHTING)
      dam *= 1 + (POS_FIGHTING - GET_POS(victim)) / 3;

    /* at least 1 hp damage min per hit */
    dam = MAX(1, dam);

    if (type == SKILL_BACKSTAB)
      damage(ch, victim, dam * backstab_mult(GET_LEVEL(ch)), SKILL_BACKSTAB);
    else
      damage(ch, victim, dam, w_type);
  }

  /* check if the victim has a hitprcnt trigger */
  hitprcnt_mtrigger(victim);
  
  if (dam && (type != SKILL_BACKSTAB) && GET_POS(victim) > POS_STUNNED)
   weapon_spells(ch, victim, wielded);

}

void weapon_spells(struct char_data *ch, struct char_data *vict, struct obj_data *wpn)
{
  int i=0, rndm;

  if(wpn&&HAS_SPELLS(wpn)) {
   while(GET_WEAPON_SPELL(wpn,i)&&i<MAX_WEAPON_SPELLS) {

/* Do some sanity checking, in case someone flees, etc. */
/* Can't have enough of 'em.  */

  if (ch->in_room != vict->in_room) {
   if (FIGHTING(ch) && FIGHTING(ch) == vict)
    stop_fighting(ch);
    return;
   }

  rndm=rand_number(1,100);
  if(GET_WEAPON_SPELL_PCT(wpn,i)>=rndm) {
   act("\tc$p suddenly begins to hum and shake in your hand.\tn",TRUE,ch,wpn,0,TO_CHAR);
   act("$p suddenly begins to hum and shake in $n's hand.",TRUE,ch,wpn,0,TO_ROOM);

   if (spell_info[GET_WEAPON_SPELL(wpn,i)].violent == TRUE) 
    if(call_magic(ch,vict,NULL,GET_WEAPON_SPELL(wpn,i),GET_WEAPON_SPELL_LVL(wpn,i),
     CAST_WAND)<0) return;

   if (spell_info[GET_WEAPON_SPELL(wpn,i)].violent == FALSE)
    if(call_magic(ch,ch,NULL,GET_WEAPON_SPELL(wpn,i),GET_WEAPON_SPELL_LVL(wpn,i),
     CAST_WAND)<0) return;
   
  }
  i++;
  }
 }
}

/* control the fights going on.  Called every 2 seconds from comm.c. */
void perform_violence(void)
{
  struct char_data *ch, *tch;

  for (ch = combat_list; ch; ch = next_combat_list) {
    next_combat_list = ch->next_fighting;

    if (FIGHTING(ch) == NULL || IN_ROOM(ch) != IN_ROOM(FIGHTING(ch))) {
      stop_fighting(ch);
      continue;
    }

    if (IS_NPC(ch)) {
      if (GET_MOB_WAIT(ch) > 0) {
	GET_MOB_WAIT(ch) -= PULSE_VIOLENCE;
	continue;
      }
      GET_MOB_WAIT(ch) = 0;
      if (GET_POS(ch) < POS_FIGHTING) {
	GET_POS(ch) = POS_FIGHTING;
	act("$n scrambles to $s feet!", TRUE, ch, 0, 0, TO_ROOM);
      }
    }

    if (GET_POS(ch) < POS_FIGHTING) {
      send_to_char(ch, "You can't fight while sitting!!\r\n");
      continue;
    }

    if (GROUP(ch)) {
      while ((tch = (struct char_data *) simple_list(GROUP(ch)->members)) != NULL) {
        if (tch == ch)
          continue;
        if (!IS_NPC(tch) && !PRF_FLAGGED(tch, PRF_AUTOASSIST))
          continue;
        if (IN_ROOM(ch) != IN_ROOM(tch))
          continue;
        if (FIGHTING(tch))
          continue;
        if (GET_POS(tch) != POS_STANDING)
          continue;
        if (!CAN_SEE(tch, ch))
          continue;

        do_assist(tch, GET_NAME(ch), 0, 0);				  
      }
    }

    hit(ch, FIGHTING(ch), TYPE_UNDEFINED);
    if (MOB_FLAGGED(ch, MOB_SPEC) && GET_MOB_SPEC(ch) && !MOB_FLAGGED(ch, MOB_NOTDEADYET)) {
      char actbuf[MAX_INPUT_LENGTH] = "";
      (GET_MOB_SPEC(ch)) (ch, ch, 0, actbuf);
    }
  }
}
