/**************************************************************************
*  File: act.other.c                                       Part of tbaMUD *
*  Usage: Miscellaneous player-level commands.                             *
*                                                                         *
*  All rights reserved.  See license for complete information.            *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
**************************************************************************/

/* needed by sysdep.h to allow for definition of <sys/stat.h> */
#define __ACT_OTHER_C__

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "house.h"
#include "constants.h"
#include "dg_scripts.h"
#include "act.h"
#include "spec_procs.h"
#include "class.h"
#include "fight.h"
#include "mail.h"  /* for has_mail() */
#include "shop.h"
#include "quest.h"

/* Local defined utility functions */
/* do_group utility functions */
static int perform_group(struct char_data *ch, struct char_data *vict);
static void print_group(struct char_data *ch);
char *show_exp_tnl(struct char_data *ch);


ACMD(do_quit)
{
  if (IS_NPC(ch) || !ch->desc)
    return;

  if (subcmd != SCMD_QUIT && GET_LEVEL(ch) < LVL_IMMORT)
    send_to_char(ch, "You have to type quit--no less, to quit!\r\n");
  else if (GET_POS(ch) == POS_FIGHTING)
    send_to_char(ch, "No way!  You're fighting for your life!\r\n");
  else if (GET_POS(ch) < POS_STUNNED) {
    send_to_char(ch, "You die before your time...\r\n");
    die(ch, NULL);
  } else {
    act("$n has left the game.", TRUE, ch, 0, 0, TO_ROOM);
    mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "%s has quit the game.", GET_NAME(ch));

    if (GET_QUEST_TIME(ch) != -1)
      quest_timeout(ch);

    send_to_char(ch, "Goodbye, friend.. Come back soon!\r\n");

    /* We used to check here for duping attempts, but we may as well do it right
     * in extract_char(), since there is no check if a player rents out and it
     * can leave them in an equally screwy situation. */

    if (CONFIG_FREE_RENT)
      Crash_rentsave(ch, 0);

    GET_LOADROOM(ch) = GET_ROOM_VNUM(IN_ROOM(ch));

    /* Stop snooping so you can't see passwords during deletion or change. */
    if (ch->desc->snoop_by) {
      write_to_output(ch->desc->snoop_by, "Your victim is no longer among us.\r\n");
      ch->desc->snoop_by->snooping = NULL;
      ch->desc->snoop_by = NULL;
    }

    extract_char(ch);		/* Char is saved before extracting. */
  }
}

ACMD(do_save)
{
  if (IS_NPC(ch) || !ch->desc)
    return;

  send_to_char(ch, "Saving %s.\r\n", GET_NAME(ch));
  save_char(ch);
  Crash_crashsave(ch);
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_HOUSE_CRASH))
    House_crashsave(GET_ROOM_VNUM(IN_ROOM(ch)));
  GET_LOADROOM(ch) = GET_ROOM_VNUM(IN_ROOM(ch));
}

/* Generic function for commands which are normally overridden by special
 * procedures - i.e., shop commands, mail commands, etc. */
ACMD(do_not_here)
{
  send_to_char(ch, "Sorry, but you cannot do that here!\r\n");
}

ACMD(do_sneak)
{
  struct affected_type af;
  byte percent;

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_SNEAK)) {
    send_to_char(ch, "You have no idea how to do that.\r\n");
    return;
  }
  send_to_char(ch, "Okay, you'll try to move silently for a while.\r\n");
  if (AFF_FLAGGED(ch, AFF_SNEAK))
    affect_from_char(ch, SKILL_SNEAK);

  percent = rand_number(1, 101);	/* 101% is a complete failure */

  if (percent > GET_SKILL(ch, SKILL_SNEAK) + dex_app_skill[GET_DEX(ch)].sneak)
    return;

  af.type = SKILL_SNEAK;
  af.duration = GET_LEVEL(ch);
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.bitvector = AFF_SNEAK;
  affect_to_char(ch, &af);
}

ACMD(do_hide)
{
  byte percent;

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_HIDE)) {
    send_to_char(ch, "You have no idea how to do that.\r\n");
    return;
  }

  send_to_char(ch, "You attempt to hide yourself.\r\n");

  if (AFF_FLAGGED(ch, AFF_HIDE))
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_HIDE);

  percent = rand_number(1, 101);	/* 101% is a complete failure */

  if (percent > GET_SKILL(ch, SKILL_HIDE) + dex_app_skill[GET_DEX(ch)].hide)
    return;

  SET_BIT_AR(AFF_FLAGS(ch), AFF_HIDE);
}

ACMD(do_steal)
{
  struct char_data *vict;
  struct obj_data *obj;
  char vict_name[MAX_INPUT_LENGTH], obj_name[MAX_INPUT_LENGTH];
  int percent, gold, eq_pos, pcsteal = 0, ohoh = 0;

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_STEAL)) {
    send_to_char(ch, "You have no idea how to do that.\r\n");
    return;
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  two_arguments(argument, obj_name, vict_name);

  if (!(vict = get_char_vis(ch, vict_name, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "Steal what from who?\r\n");
    return;
  } else if (vict == ch) {
    send_to_char(ch, "Come on now, that's rather stupid!\r\n");
    return;
  }

  /* 101% is a complete failure */
  percent = rand_number(1, 101) - dex_app_skill[GET_DEX(ch)].p_pocket;

  if (GET_POS(vict) < POS_SLEEPING)
    percent = -1;		/* ALWAYS SUCCESS, unless heavy object. */

  if (!CONFIG_PT_ALLOWED && !IS_NPC(vict))
    pcsteal = 1;

  if (!AWAKE(vict))	/* Easier to steal from sleeping people. */
    percent -= 50;

  /* No stealing if not allowed. If it is no stealing from Imm's or Shopkeepers. */
  if (GET_LEVEL(vict) >= LVL_IMMORT || pcsteal || GET_MOB_SPEC(vict) == shop_keeper)
    percent = 101;		/* Failure */

  if (str_cmp(obj_name, "coins") && str_cmp(obj_name, "gold")) {

    if (!(obj = get_obj_in_list_vis(ch, obj_name, NULL, vict->carrying))) {

      for (eq_pos = 0; eq_pos < NUM_WEARS; eq_pos++)
	if (GET_EQ(vict, eq_pos) &&
	    (isname(obj_name, GET_EQ(vict, eq_pos)->name)) &&
	    CAN_SEE_OBJ(ch, GET_EQ(vict, eq_pos))) {
	  obj = GET_EQ(vict, eq_pos);
	  break;
	}
      if (!obj) {
	act("$E hasn't got that item.", FALSE, ch, 0, vict, TO_CHAR);
	return;
      } else {			/* It is equipment */
	if ((GET_POS(vict) > POS_STUNNED)) {
	  send_to_char(ch, "Steal the equipment now?  Impossible!\r\n");
	  return;
	} else {
          if (!give_otrigger(obj, vict, ch) ||
              !receive_mtrigger(ch, vict, obj) ) {
            send_to_char(ch, "Impossible!\r\n");
            return;
          }
	  act("You unequip $p and steal it.", FALSE, ch, obj, 0, TO_CHAR);
	  act("$n steals $p from $N.", FALSE, ch, obj, vict, TO_NOTVICT);
	  obj_to_char(unequip_char(vict, eq_pos), ch);
	}
      }
    } else {			/* obj found in inventory */

      percent += GET_OBJ_WEIGHT(obj);	/* Make heavy harder */

      if (percent > GET_SKILL(ch, SKILL_STEAL)) {
	ohoh = TRUE;
	send_to_char(ch, "Oops..\r\n");
	act("$n tried to steal something from you!", FALSE, ch, 0, vict, TO_VICT);
	act("$n tries to steal something from $N.", TRUE, ch, 0, vict, TO_NOTVICT);
      } else {			/* Steal the item */
	if (IS_CARRYING_N(ch) + 1 < CAN_CARRY_N(ch)) {
          if (!give_otrigger(obj, vict, ch) ||
              !receive_mtrigger(ch, vict, obj) ) {
            send_to_char(ch, "Impossible!\r\n");
            return;
          }
	  if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj) < CAN_CARRY_W(ch)) {
	    obj_from_char(obj);
	    obj_to_char(obj, ch);
	    send_to_char(ch, "Got it!\r\n");
	  }
	} else
	  send_to_char(ch, "You cannot carry that much.\r\n");
      }
    }
  } else {			/* Steal some coins */
    if (AWAKE(vict) && (percent > GET_SKILL(ch, SKILL_STEAL))) {
      ohoh = TRUE;
      send_to_char(ch, "Oops..\r\n");
      act("You discover that $n has $s hands in your wallet.", FALSE, ch, 0, vict, TO_VICT);
      act("$n tries to steal gold from $N.", TRUE, ch, 0, vict, TO_NOTVICT);
    } else {
      /* Steal some gold coins */
      gold = (GET_GOLD(vict) * rand_number(1, 10)) / 100;
      gold = MIN(1782, gold);
      if (gold > 0) {
	GET_GOLD(ch) += gold;
	GET_GOLD(vict) -= gold;
        if (gold > 1)
	  send_to_char(ch, "Bingo!  You got %d gold coins.\r\n", gold);
	else
	  send_to_char(ch, "You manage to swipe a solitary gold coin.\r\n");
      } else {
	send_to_char(ch, "You couldn't get any gold...\r\n");
      }
    }
  }

  if (ohoh && IS_NPC(vict) && AWAKE(vict))
    hit(vict, ch, TYPE_UNDEFINED);
}

ACMD(do_practice)
{
  char arg[MAX_INPUT_LENGTH];

  if (IS_NPC(ch))
    return;

  one_argument(argument, arg);

  if (*arg)
    send_to_char(ch, "You can only practice skills in your guild.\r\n");
  else
    list_skills(ch);
}

ACMD(do_visible)
{
  if (GET_LEVEL(ch) >= LVL_IMMORT) {
    perform_immort_vis(ch);
    return;
  }

  if AFF_FLAGGED(ch, AFF_INVISIBLE) {
    appear(ch);
    send_to_char(ch, "You break the spell of invisibility.\r\n");
  } else
    send_to_char(ch, "You are already visible.\r\n");
}

ACMD(do_title)
{
  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (IS_NPC(ch))
    send_to_char(ch, "Your title is fine... go away.\r\n");
  else if (PLR_FLAGGED(ch, PLR_NOTITLE))
    send_to_char(ch, "You can't title yourself -- you shouldn't have abused it!\r\n");
  else if (strstr(argument, "(") || strstr(argument, ")"))
    send_to_char(ch, "Titles can't contain the ( or ) characters.\r\n");
  else if (strlen(argument) > MAX_TITLE_LENGTH)
    send_to_char(ch, "Sorry, titles can't be longer than %d characters.\r\n", MAX_TITLE_LENGTH);
  else {
    set_title(ch, argument);
    send_to_char(ch, "Okay, you're now %s%s%s.\r\n", GET_NAME(ch), *GET_TITLE(ch) ? " " : "", GET_TITLE(ch));
  }
}

ACMD(do_badge)
{
 skip_spaces(&argument);
 
 if (strlen(argument) > MAX_BADGE_LENGTH) {
	  send_to_char(ch, "Badges cannot exceed %d characters.\r\n", MAX_BADGE_LENGTH);
	  }
   else {
 
        if (GET_BADGE(ch))
          free(GET_BADGE(ch));

	if (!*argument)
          GET_BADGE(ch) = NULL;
        else
          GET_BADGE(ch) = strdup(argument);
		  send_to_char(ch, "Your badge is now: %s\r\n", GET_BADGE(ch));
        }
}

static int perform_group(struct char_data *ch, struct char_data *vict)
{
  if (AFF_FLAGGED(vict, AFF_GROUP) || !CAN_SEE(ch, vict))
    return (0);

  SET_BIT_AR(AFF_FLAGS(vict), AFF_GROUP);
  if (ch != vict)
    act("$N is now a member of your group.", FALSE, ch, 0, vict, TO_CHAR);
  act("You are now a member of $n's group.", FALSE, ch, 0, vict, TO_VICT);
  act("$N is now a member of $n's group.", FALSE, ch, 0, vict, TO_NOTVICT);
  return (1);
}
	
char *show_exp_tnl(struct char_data *ch)
{
    long tnl;
	static char buf[25];

    if (IS_NPC(ch) || (GET_LEVEL(ch) >= LVL_IMMORT))
      return "";
		
	if (GET_LEVEL(ch) == 50)
      return "can remort";

	tnl = level_exp(GET_CLASS(ch), GET_LEVEL(ch) + 1) - GET_EXP(ch);
	
    if (tnl > 1000000)
      snprintf(buf, sizeof(buf), "%ldmil tnl", tnl / 1000000);
	else if (tnl > 1000)
      snprintf(buf, sizeof(buf), "%ldk tnl", tnl / 1000);
	else
	  snprintf(buf, sizeof(buf), "%ld tnl", tnl);

    return buf;
}

static void print_group(struct char_data *ch)
{
  struct char_data *k;
  struct follow_type *f;

  if (!AFF_FLAGGED(ch, AFF_GROUP))
    send_to_char(ch, "But you are not the member of a group!\r\n");
  else {
    char buf[MAX_STRING_LENGTH];

    send_to_char(ch, "Your group consists of:\r\n");

    k = (ch->master ? ch->master : ch);

    if (AFF_FLAGGED(k, AFF_GROUP)) {
      snprintf(buf, sizeof(buf), "     %s[%s%4dH %4dM %4dV%s]%s %s[%s%10s%s]%s %s[%s%2d %s%s]%s $N %s(%sHead of group%s)%s",
	      CCCYN(ch, C_SPR), CCNRM(ch, C_SPR), 
		    GET_HIT(k), GET_MANA(k), GET_MOVE(k), 
			CCCYN(ch, C_SPR), CCNRM(ch, C_SPR),
		  CCYEL(ch, C_SPR), CCNRM(ch, C_SPR),
		    show_exp_tnl(k),
			CCYEL(ch, C_SPR), CCNRM(ch, C_SPR),
		  CCWHT(ch, C_SPR), CCNRM(ch, C_SPR), 
		    GET_LEVEL(k) >= LVL_IMMORT ? 0 : GET_LEVEL(k), 
		    GET_LEVEL(k) >= LVL_IMMORT ? " IMM" :CLASS_ABBR(k), 
		    CCWHT(ch, C_SPR), CCNRM(ch, C_SPR), 
		  CBCYN(ch, C_SPR), CBBLU(ch, C_SPR), CBCYN(ch, C_SPR), CCNRM(ch, C_SPR));
      act(buf, FALSE, ch, 0, k, TO_CHAR);
    }

    for (f = k->followers; f; f = f->next) {
      if (!AFF_FLAGGED(f->follower, AFF_GROUP))
	continue;

      snprintf(buf, sizeof(buf), "     %s[%s%4dH %4dM %4dV%s]%s %s[%s%10s%s]%s %s[%s%2d %s%s]%s $N", 
		  CCCYN(ch, C_SPR), CCNRM(ch, C_SPR), 
		    GET_HIT(f->follower), GET_MANA(f->follower), GET_MOVE(f->follower), 
		    CCCYN(ch, C_SPR), CCNRM(ch, C_SPR),
		  CCYEL(ch, C_SPR), CCNRM(ch, C_SPR),
		    show_exp_tnl(f->follower),
			CCYEL(ch, C_SPR), CCNRM(ch, C_SPR),
		  CCWHT(ch, C_SPR), CCNRM(ch, C_SPR), 
		    GET_LEVEL(f->follower) >= LVL_IMMORT ? 0 : GET_LEVEL(f->follower), 
		    GET_LEVEL(f->follower) >= LVL_IMMORT ? " IMM" :CLASS_ABBR(f->follower), 
		    CCWHT(ch, C_SPR), CCNRM(ch, C_SPR));
      act(buf, FALSE, ch, 0, f->follower, TO_CHAR);
    }
  }
}

ACMD(do_group)
{
  char buf[MAX_STRING_LENGTH];
  struct char_data *vict;
  struct follow_type *f;
  int found;

  one_argument(argument, buf);

  if (!*buf) {
    print_group(ch);
    return;
  }

  if (ch->master) {
    act("You cannot enroll group members without being head of a group.",
	FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  if (!str_cmp(buf, "all")) {
    perform_group(ch, ch);
    for (found = 0, f = ch->followers; f; f = f->next)
      found += perform_group(ch, f->follower);
    if (!found)
      send_to_char(ch, "Everyone following you is already in your group.\r\n");
    return;
  }

  if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM)))
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if ((vict->master != ch) && (vict != ch))
    act("$N must follow you to enter your group.", FALSE, ch, 0, vict, TO_CHAR);
  else {
    if (!AFF_FLAGGED(vict, AFF_GROUP))
      perform_group(ch, vict);
    else {
      if (ch != vict)
	act("$N is no longer a member of your group.", FALSE, ch, 0, vict, TO_CHAR);
      act("You have been kicked out of $n's group!", FALSE, ch, 0, vict, TO_VICT);
      act("$N has been kicked out of $n's group!", FALSE, ch, 0, vict, TO_NOTVICT);
      REMOVE_BIT_AR(AFF_FLAGS(vict), AFF_GROUP);
    }
  }
}

ACMD(do_ungroup)
{
  char buf[MAX_INPUT_LENGTH];
  struct follow_type *f, *next_fol;
  struct char_data *tch;

  one_argument(argument, buf);

  if (!*buf) {
    if (ch->master || !(AFF_FLAGGED(ch, AFF_GROUP))) {
      send_to_char(ch, "But you lead no group!\r\n");
      return;
    }

    for (f = ch->followers; f; f = next_fol) {
      next_fol = f->next;
      if (AFF_FLAGGED(f->follower, AFF_GROUP)) {
	REMOVE_BIT_AR(AFF_FLAGS(f->follower), AFF_GROUP);
        act("$N has disbanded the group.", TRUE, f->follower, NULL, ch, TO_CHAR);
        if (!AFF_FLAGGED(f->follower, AFF_CHARM))
	  stop_follower(f->follower);
      }
    }

    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_GROUP);
    send_to_char(ch, "You disband the group.\r\n");
    return;
  }
  if (!(tch = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "There is no such person!\r\n");
    return;
  }
  if (tch->master != ch) {
    send_to_char(ch, "That person is not following you!\r\n");
    return;
  }

  if (!AFF_FLAGGED(tch, AFF_GROUP)) {
    send_to_char(ch, "That person isn't in your group.\r\n");
    return;
  }

  REMOVE_BIT_AR(AFF_FLAGS(tch), AFF_GROUP);

  act("$N is no longer a member of your group.", FALSE, ch, 0, tch, TO_CHAR);
  act("You have been kicked out of $n's group!", FALSE, ch, 0, tch, TO_VICT);
  act("$N has been kicked out of $n's group!", FALSE, ch, 0, tch, TO_NOTVICT);

  if (!AFF_FLAGGED(tch, AFF_CHARM))
    stop_follower(tch);
}

ACMD(do_report)
{
  char buf[MAX_STRING_LENGTH];
  struct char_data *k;
  struct follow_type *f;

  if (!AFF_FLAGGED(ch, AFF_GROUP)) {
    send_to_char(ch, "But you are not a member of any group!\r\n");
    return;
  }

  snprintf(buf, sizeof(buf), "$n reports: %d/%dH, %d/%dM, %d/%dV\r\n",
	  GET_HIT(ch), GET_MAX_HIT(ch),
	  GET_MANA(ch), GET_MAX_MANA(ch),
	  GET_MOVE(ch), GET_MAX_MOVE(ch));

  k = (ch->master ? ch->master : ch);

  for (f = k->followers; f; f = f->next)
    if (AFF_FLAGGED(f->follower, AFF_GROUP) && f->follower != ch)
      act(buf, TRUE, ch, NULL, f->follower, TO_VICT);

  if (k != ch)
    act(buf, TRUE, ch, NULL, k, TO_VICT);

  send_to_char(ch, "You report to the group.\r\n");
}

ACMD(do_split)
{
  char buf[MAX_INPUT_LENGTH];
  int amount, num, share, rest;
  size_t len;
  struct char_data *k;
  struct follow_type *f;

  if (IS_NPC(ch))
    return;

  one_argument(argument, buf);

  if (is_number(buf)) {
    amount = atoi(buf);
    if (amount <= 0) {
      send_to_char(ch, "Sorry, you can't do that.\r\n");
      return;
    }
    if (amount > GET_GOLD(ch)) {
      send_to_char(ch, "You don't seem to have that much gold to split.\r\n");
      return;
    }
    k = (ch->master ? ch->master : ch);

    if (AFF_FLAGGED(k, AFF_GROUP) && (IN_ROOM(k) == IN_ROOM(ch)))
      num = 1;
    else
      num = 0;

    for (f = k->followers; f; f = f->next)
      if (AFF_FLAGGED(f->follower, AFF_GROUP) &&
	  (!IS_NPC(f->follower)) &&
	  (IN_ROOM(f->follower) == IN_ROOM(ch)))
	num++;

    if (num && AFF_FLAGGED(ch, AFF_GROUP)) {
      share = amount / num;
      rest = amount % num;
    } else {
      send_to_char(ch, "With whom do you wish to share your gold?\r\n");
      return;
    }

    GET_GOLD(ch) -= share * (num - 1);

    /* Abusing signed/unsigned to make sizeof work. */
    len = snprintf(buf, sizeof(buf), "%s splits %d coins; you receive %d.\r\n",
		GET_NAME(ch), amount, share);
    if (rest && len < sizeof(buf)) {
      snprintf(buf + len, sizeof(buf) - len,
		"%d coin%s %s not splitable, so %s keeps the money.\r\n", rest,
		(rest == 1) ? "" : "s", (rest == 1) ? "was" : "were", GET_NAME(ch));
    }
    if (AFF_FLAGGED(k, AFF_GROUP) && IN_ROOM(k) == IN_ROOM(ch) &&
		!IS_NPC(k) && k != ch) {
      GET_GOLD(k) += share;
      send_to_char(k, "%s", buf);
    }

    for (f = k->followers; f; f = f->next) {
      if (AFF_FLAGGED(f->follower, AFF_GROUP) &&
	  (!IS_NPC(f->follower)) &&
	  (IN_ROOM(f->follower) == IN_ROOM(ch)) &&
	  f->follower != ch) {

	GET_GOLD(f->follower) += share;
	send_to_char(f->follower, "%s", buf);
      }
    }
    send_to_char(ch, "You split %d coins among %d members -- %d coins each.\r\n",
	    amount, num, share);

    if (rest) {
      send_to_char(ch, "%d coin%s %s not splitable, so you keep the money.\r\n",
		rest, (rest == 1) ? "" : "s", (rest == 1) ? "was" : "were");
      GET_GOLD(ch) += rest;
    }
  } else {
    send_to_char(ch, "How many coins do you wish to split with your group?\r\n");
    return;
  }
}

ACMD(do_use)
{
  char buf[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];
  struct obj_data *mag_item;

  half_chop(argument, arg, buf);
  if (!*arg) {
    send_to_char(ch, "What do you want to %s?\r\n", CMD_NAME);
    return;
  }
  mag_item = GET_EQ(ch, WEAR_HOLD);

  if (!mag_item || !isname(arg, mag_item->name)) {
    switch (subcmd) {
    case SCMD_RECITE:
    case SCMD_QUAFF:
      if (!(mag_item = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
	send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
	return;
      }
      break;
    case SCMD_USE:
      send_to_char(ch, "You don't seem to be holding %s %s.\r\n", AN(arg), arg);
      return;
    default:
      log("SYSERR: Unknown subcmd %d passed to do_use.", subcmd);
      /* SYSERR_DESC: This is the same as the unhandled case in do_gen_ps(),
       * but in the function which handles 'quaff', 'recite', and 'use'. */
      return;
    }
  }
  switch (subcmd) {
  case SCMD_QUAFF:
    if (GET_OBJ_TYPE(mag_item) != ITEM_POTION) {
      send_to_char(ch, "You can only quaff potions.\r\n");
      return;
    }
    break;
  case SCMD_RECITE:
    if (GET_OBJ_TYPE(mag_item) != ITEM_SCROLL) {
      send_to_char(ch, "You can only recite scrolls.\r\n");
      return;
    }
    break;
  case SCMD_USE:
    if ((GET_OBJ_TYPE(mag_item) != ITEM_WAND) &&
	(GET_OBJ_TYPE(mag_item) != ITEM_STAFF)) {
      send_to_char(ch, "You can't seem to figure out how to use it.\r\n");
      return;
    }
    break;
  }

  mag_objectmagic(ch, mag_item, buf);
}

ACMD(do_wimpy)
{
  char buf[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];
  int wimp_lev;

  /* 'wimp_level' is a player_special. -gg 2/25/98 */
  if (IS_NPC(ch))
    return;

  one_argument(argument, arg);

  if (!*arg) {
    if (GET_WIMP_LEV(ch)) {
      sprintf(buf, "Your current wimp level is %d hit points.\r\n",
	      GET_WIMP_LEV(ch));
      send_to_char(ch, "%s", buf);
      return;
    } else {
      send_to_char(ch, "At the moment, you'll stick it out to the bitter end.\r\n");
      return;
    }
  }
  if (isdigit(*arg)) {
    if ((wimp_lev = atoi(arg)) != 0) {
      if (wimp_lev < 0)
	send_to_char(ch, "Heh, heh, heh.. we are jolly funny today, eh?\r\n");
      else if (wimp_lev > GET_MAX_HIT(ch))
	send_to_char(ch, "That doesn't make much sense, now does it?\r\n");
      else if (wimp_lev > (GET_MAX_HIT(ch) / 2))
	send_to_char(ch, "You can't set your wimp level above half your hit points.\r\n");
      else {
	sprintf(buf, "Okay, you'll wimp out if you drop below %d hit points.\r\n",
		wimp_lev);
	send_to_char(ch, "%s", buf);
	GET_WIMP_LEV(ch) = wimp_lev;
      }
    } else {
      send_to_char(ch, "Okay, you'll now tough out fights to the bitter end.\r\n");
      GET_WIMP_LEV(ch) = 0;
    }
  } else
    send_to_char(ch, "Specify at how many hit points you want to wimp out at.  (0 to disable)\r\n");
}

ACMD(do_display)
{
  size_t i;

  if (IS_NPC(ch)) {
    send_to_char(ch, "Monsters don't need displays.  Go away.\r\n");
    return;
  }
  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(ch, "Usage: prompt { { H | M | V | A } | all | auto | none }\r\n");
    return;
  }

  if (!str_cmp(argument, "auto")) {
    TOGGLE_BIT_AR(PRF_FLAGS(ch), PRF_DISPAUTO);
    send_to_char(ch, "Auto prompt %sabled.\r\n", PRF_FLAGGED(ch, PRF_DISPAUTO) ? "en" : "dis");
    return;
  }

  if (!str_cmp(argument, "on") || !str_cmp(argument, "all")) {
    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPHP);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPMANA);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPMOVE);
	SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPALIGN);
  } else if (!str_cmp(argument, "off") || !str_cmp(argument, "none")) {
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPHP);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPMANA);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPMOVE);
	REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPALIGN);
  } else {
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPHP);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPMANA);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPMOVE);
	REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPALIGN);

    for (i = 0; i < strlen(argument); i++) {
      switch (LOWER(argument[i])) {
      case 'h':
        SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPHP);
	break;
      case 'm':
        SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPMANA);
	break;
      case 'v':
        SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPMOVE);
	break;
	case 'a':
        SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPALIGN);
	break;
      default:
	send_to_char(ch, "Usage: prompt { { H | M | V | A } | all | auto | none }\r\n");
	return;
      }
    }
  }

  send_to_char(ch, "%s", CONFIG_OK);
}

#define TOG_OFF 0
#define TOG_ON  1
ACMD(do_gen_tog)
{
  long result;

  const char *tog_messages[][2] = {
    {"You are now safe from summoning by other players.\r\n",
    "You may now be summoned by other players.\r\n"},
    {"Nohassle disabled.\r\n",
    "Nohassle enabled.\r\n"},
    {"Brief mode off.\r\n",
    "Brief mode on.\r\n"},
    {"Compact mode off.\r\n",
    "Compact mode on.\r\n"},
    {"You can now hear tells.\r\n",
    "You are now deaf to tells.\r\n"},
    {"You can now hear auctions.\r\n",
    "You are now deaf to auctions.\r\n"},
    {"You can now hear shouts.\r\n",
    "You are now deaf to shouts.\r\n"},
    {"You can now hear gossip.\r\n",
    "You are now deaf to gossip.\r\n"},
    {"You can now hear the congratulation messages.\r\n",
    "You are now deaf to the congratulation messages.\r\n"},
    {"You can now hear the Wiz-channel.\r\n",
    "You are now deaf to the Wiz-channel.\r\n"},
    {"You are no longer part of the Quest.\r\n",
    "Okay, you are part of the Quest!\r\n"},
    {"You will no longer see the room flags.\r\n",
    "You will now see the room flags.\r\n"},
    {"You will now have your communication repeated.\r\n",
    "You will no longer have your communication repeated.\r\n"},
    {"HolyLight mode off.\r\n",
    "HolyLight mode on.\r\n"},
    {"Nameserver_is_slow changed to NO; IP addresses will now be resolved.\r\n",
    "Nameserver_is_slow changed to YES; sitenames will no longer be resolved.\r\n"},
    {"Autoexits disabled.\r\n",
    "Autoexits enabled.\r\n"},
    {"Will no longer track through doors.\r\n",
    "Will now track through doors.\r\n"},
    {"Will no longer clear screen in OLC.\r\n",
    "Will now clear screen in OLC.\r\n"},
    {"Buildwalk Off.\r\n",
    "Buildwalk On.\r\n"},
    {"AFK flag is now off.\r\n",
    "AFK flag is now on.\r\n"},
    {"Autoloot disabled.\r\n",
    "Autoloot enabled.\r\n"},
    {"Autogold disabled.\r\n",
    "Autogold enabled.\r\n"},
    {"Autosplit disabled.\r\n",
    "Autosplit enabled.\r\n"},
    {"Autosacrifice disabled.\r\n",
    "Autosacrifice enabled.\r\n"},
    {"Autoassist disabled.\r\n",
    "Autoassist enabled.\r\n"},
    {"Automap disabled.\r\n",
    "Automap enabled.\r\n"},
    {"Autokey disabled.\r\n",
    "Autokey enabled.\r\n"},
    {"Autodoor disabled.\r\n",
    "Autodoor enabled.\r\n"},
	{"\r\n", "\r\n"},
	{"\r\n", "\r\n"},
	{"\r\n", "\r\n"},
	{"\r\n", "\r\n"},
	{"\r\n", "\r\n"},
	{"Autodiagnose disabled.\r\n",
	"Autodiagnose enabled.\r\n"},
	{"Your affections will now be shown on score.\r\n",
	"Your affections will no longer be shown on score.\r\n"},
	{"Your level will now be shown on the who list.\r\n",
	"Your level will no longer be shown on the who list.\r\n"},
	{"You will no longer be able to edit any zone.\r\n",
	"You regain the ability to edit any zone.\r\n"},
	{"You will now be shown on the who list.\r\n",
	"You will no longer be shown on the who list.\r\n"}
  };

  if (IS_NPC(ch))
    return;

  switch (subcmd) {
  case SCMD_NOSUMMON:
    result = PRF_TOG_CHK(ch, PRF_SUMMONABLE);
    break;
  case SCMD_NOHASSLE:
    result = PRF_TOG_CHK(ch, PRF_NOHASSLE);
    break;
  case SCMD_BRIEF:
    result = PRF_TOG_CHK(ch, PRF_BRIEF);
    break;
  case SCMD_COMPACT:
    result = PRF_TOG_CHK(ch, PRF_COMPACT);
    break;
  case SCMD_NOTELL:
    result = PRF_TOG_CHK(ch, PRF_NOTELL);
    break;
  case SCMD_NOAUCTION:
    result = PRF_TOG_CHK(ch, PRF_NOAUCT);
    break;
  case SCMD_NOSHOUT:
    result = PRF_TOG_CHK(ch, PRF_NOSHOUT);
    break;
  case SCMD_NOGOSSIP:
    result = PRF_TOG_CHK(ch, PRF_NOGOSS);
    break;
  case SCMD_NOGRATZ:
    result = PRF_TOG_CHK(ch, PRF_NOGRATZ);
    break;
  case SCMD_NOWIZ:
    result = PRF_TOG_CHK(ch, PRF_NOWIZ);
    break;
  case SCMD_QUEST:
    result = PRF_TOG_CHK(ch, PRF_QUEST);
    break;
  case SCMD_SHOWVNUMS:
    result = PRF_TOG_CHK(ch, PRF_SHOWVNUMS);
    break;
  case SCMD_NOREPEAT:
    result = PRF_TOG_CHK(ch, PRF_NOREPEAT);
    break;
  case SCMD_HOLYLIGHT:
    result = PRF_TOG_CHK(ch, PRF_HOLYLIGHT);
    break;
  case SCMD_AUTOEXIT:
    result = PRF_TOG_CHK(ch, PRF_AUTOEXIT);
    break;
  case SCMD_CLS:
    result = PRF_TOG_CHK(ch, PRF_CLS);
    break;
  case SCMD_BUILDWALK:
    if (GET_LEVEL(ch) < LVL_BUILDER) {
      send_to_char(ch, "Builders only, sorry.\r\n");
      return;
    }
    result = PRF_TOG_CHK(ch, PRF_BUILDWALK);
    if (PRF_FLAGGED(ch, PRF_BUILDWALK))
      mudlog(CMP, GET_LEVEL(ch), TRUE,
             "OLC: %s turned buildwalk on. Allowed zone %d", GET_NAME(ch), GET_OLC_ZONE(ch));
    else
      mudlog(CMP, GET_LEVEL(ch), TRUE,
             "OLC: %s turned buildwalk off. Allowed zone %d", GET_NAME(ch), GET_OLC_ZONE(ch));
    break;
  case SCMD_AFK:
    result = PRF_TOG_CHK(ch, PRF_AFK);
    if (PRF_FLAGGED(ch, PRF_AFK))
      act("$n has gone AFK.", TRUE, ch, 0, 0, TO_ROOM);
    else {
      act("$n has come back from AFK.", TRUE, ch, 0, 0, TO_ROOM);
      if (has_mail(GET_IDNUM(ch)))
        send_to_char(ch, "You have mail waiting.\r\n");
    }
    break;
  case SCMD_AUTOLOOT:
    result = PRF_TOG_CHK(ch, PRF_AUTOLOOT);
    break;
  case SCMD_AUTOGOLD:
    result = PRF_TOG_CHK(ch, PRF_AUTOGOLD);
    break;
  case SCMD_AUTOSPLIT:
    result = PRF_TOG_CHK(ch, PRF_AUTOSPLIT);
    break;
  case SCMD_AUTOSAC:
    result = PRF_TOG_CHK(ch, PRF_AUTOSAC);
    break;
  case SCMD_AUTOASSIST:
    result = PRF_TOG_CHK(ch, PRF_AUTOASSIST);
    break;
  case SCMD_AUTOMAP:
    result = PRF_TOG_CHK(ch, PRF_AUTOMAP);
    break;
  case SCMD_AUTOKEY:
    result = PRF_TOG_CHK(ch, PRF_AUTOKEY);
    break;
  case SCMD_AUTODOOR:
    result = PRF_TOG_CHK(ch, PRF_AUTODOOR);
    break;
  case SCMD_AUTODIAG:
    result = PRF_TOG_CHK(ch, PRF_AUTODIAG);
    break;
  case SCMD_NO_AFF:
    result = PRF_TOG_CHK(ch, PRF_NO_AFF);
    break;
  case SCMD_ANON:
    result = PRF_TOG_CHK(ch, PRF_ANON);
    break;
  case SCMD_WRLDWRITE:
    result = PRF_TOG_CHK(ch, PRF_WORLD_WRITE);
    break;
  case SCMD_NOWHO:
    result = PRF_TOG_CHK(ch, PRF_NOWHO);
    break;
	
  default:
    log("SYSERR: Unknown subcmd %d in do_gen_toggle.", subcmd);
    return;
  }

  if (result)
    send_to_char(ch, "%s", tog_messages[subcmd][TOG_ON]);
  else
    send_to_char(ch, "%s", tog_messages[subcmd][TOG_OFF]);

  return;
}
