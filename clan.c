/*
    clan.c
    ascii pfile system
*/


#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "interpreter.h"
#include "handler.h"
#include "screen.h"

void do_clan_who(struct char_data *ch);
void do_clan_say(struct char_data *ch, char *msg);
void do_clan_resign(struct char_data *ch);
void do_clan_dismiss(struct char_data *ch, struct char_data *victim);
void do_clan_enroll(struct char_data *ch, struct char_data *victim);
void do_clan_promote(struct char_data *ch, struct char_data *victim);
void do_clan_demote(struct char_data *ch, struct char_data *victim);
void do_clan_members(struct char_data *ch);

char *get_rank_name(int clan, int rank);
char *get_clan_name(int clan);

extern struct descriptor_data *descriptor_list;
extern char *class_abbrevs[];
extern struct zone_data *zone_table;
extern struct room_data *world;


/* to set up a new clan, implmentor needs to:

    assign  

*/

ACMD(do_clan)
{
    struct char_data *victim = NULL;
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];

    if (GET_CLAN(ch) == 0)
    {
        send_to_char(ch, "You do not belong to any clan.\r\n");
        return;
    }

    half_chop(argument, arg1, arg2);

    if (!*arg1)
    {
      send_to_char(ch, "Usage: clan [ who | say | resign | members]\r\n");

      if (GET_CLAN_RANK(ch) == CLAN_RANK_LEADER)
            send_to_char(ch, "Also:  clan [ enroll | dismiss | promote | demote ] victim\r\n");

      if (GET_LEVEL(ch) == LVL_IMPL)
            send_to_char(ch, "Assign leaders: assign <clan number> name \r\n");

      return;
    }

    /* perform general level functions first */

    if (is_abbrev(arg1, "who"))
    {
        do_clan_who(ch);
        return ;
    }

    if (is_abbrev(arg1, "say"))
    {
        do_clan_say(ch, arg2);
        return ;
    }

    if (is_abbrev(arg1, "resign"))
    {
        do_clan_resign(ch);
        return ;
    }
	
	if (is_abbrev(arg1, "members"))
    {
        do_clan_members(ch);
        return ;
    }

    /* now perform privileged functions */

    if (GET_CLAN_RANK(ch) != CLAN_RANK_LEADER)
    {
        send_to_char(ch, "You do not have the authority to reorganize the clan.\r\n");
        return;
    }

    if (!(victim = get_char_vis(ch, arg2, NULL, FIND_CHAR_ROOM)))
    {
        send_to_char(ch, "That person isn't here.\r\n");
        return;
    }

/* Why can't imms not join clans
    if (GET_LEVEL(victim) > (LVL_IMMORT - 1))
   {
        send_to_char(ch, "Immortals cannot join clans!\r\n");
        return;
    } */

    if (is_abbrev(arg1, "enroll"))
    {
        do_clan_enroll(ch, victim);
        return ;
    }

    if (GET_CLAN(victim) !=  GET_CLAN(ch))  
    {
        send_to_char(ch, "That person is not in your clan!\r\n");
        return;
    }

    if (is_abbrev(arg1, "dismiss"))
    {
        do_clan_dismiss(ch, victim);
        return ;
    }

    if (is_abbrev(arg1, "promote"))
    {
        do_clan_promote(ch, victim);
        return ;
    }

    if (is_abbrev(arg1, "demote"))
    {
        do_clan_demote(ch, victim);
        return ;
    }

    else { 
     send_to_char(ch, "That is not a clan function.\r\n");
     return;
     }
}

void do_clan_who(struct char_data *ch)
{
 char buf[MAX_STRING_LENGTH] = "";
 char bufa[MAX_STRING_LENGTH] = "";
 char bufb[MAX_STRING_LENGTH] = "";
 char bufc[MAX_STRING_LENGTH] = "";
 char bufd[MAX_STRING_LENGTH] = "";
 char bufe[MAX_STRING_LENGTH] = "";
 char buff[MAX_STRING_LENGTH] = "";
 char bufg[MAX_STRING_LENGTH] = "";
 char bufh[MAX_STRING_LENGTH] = "";
 char bufi[MAX_STRING_LENGTH] = "";
 char bufj[MAX_STRING_LENGTH] = "";

 struct descriptor_data *d;

    sprintf(buf, "Members of clan %s%s%s online:\r\n"
                  "%s--------------------------------------------------------%s\r\n",
     CCCYN(ch, C_NRM), get_clan_name(GET_CLAN(ch)), CCNRM(ch, C_NRM), 
     CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
    send_to_char(ch, "%s", buf);

    for (d = descriptor_list; d; d = d->next)
    {
        if (d->connected)
        {
            continue;
        }

        if (GET_CLAN(d->character) ==  GET_CLAN(ch)) {
         if (CAN_SEE(ch, d->character)) {		
        
         if (GET_CLAN_RANK(d->character) == 10) {
          sprintf(bufa + strlen(bufa), "%s[%s%2d %s%s] %s%s the %s %s-- %s%s%s\r\n", 
           CBGRN(ch, C_NRM), CCNRM(ch, C_NRM), GET_LEVEL(d->character),
           CLASS_ABBR(d->character), CBGRN(ch, C_NRM), CCCYN(ch, C_NRM), GET_NAME(d->character),  
           get_rank_name(GET_CLAN(d->character), GET_CLAN_RANK(d->character)),
           CBWHT(ch, C_NRM), CBBLK(ch, C_NRM),
	   (world[IN_ROOM(d->character)].zone == world[IN_ROOM(ch)].zone ?
           world[IN_ROOM(d->character)].name : 
           zone_table[world[IN_ROOM(d->character)].zone].name), CCNRM(ch, C_NRM)); 
	   }

         if (GET_CLAN_RANK(d->character) == 9) {
          sprintf(bufc + strlen(bufc), "%s[%s%2d %s%s]%s %s the %s %s-- %s%s%s\r\n", 
           CBGRN(ch, C_NRM), CCNRM(ch, C_NRM), GET_LEVEL(d->character),
           CLASS_ABBR(d->character), CBGRN(ch, C_NRM), CCNRM(ch, C_NRM), GET_NAME(d->character),  
           get_rank_name(GET_CLAN(d->character), GET_CLAN_RANK(d->character)),
		   CBWHT(ch, C_NRM), CBBLK(ch, C_NRM),
           (world[IN_ROOM(d->character)].zone == world[IN_ROOM(ch)].zone ?
           world[IN_ROOM(d->character)].name : 
           zone_table[world[IN_ROOM(d->character)].zone].name), CCNRM(ch, C_NRM)); 
          }

         if (GET_CLAN_RANK(d->character) == 8) {
          sprintf(bufc + strlen(bufc), "%s[%s%2d %s%s]%s %s the %s %s-- %s%s%s\r\n", 
           CBGRN(ch, C_NRM), CCNRM(ch, C_NRM), GET_LEVEL(d->character),
           CLASS_ABBR(d->character), CBGRN(ch, C_NRM), CCNRM(ch, C_NRM), GET_NAME(d->character),  
           get_rank_name(GET_CLAN(d->character), GET_CLAN_RANK(d->character)),
		   CBWHT(ch, C_NRM), CBBLK(ch, C_NRM),
           (world[IN_ROOM(d->character)].zone == world[IN_ROOM(ch)].zone ?
           world[IN_ROOM(d->character)].name : 
           zone_table[world[IN_ROOM(d->character)].zone].name), CCNRM(ch, C_NRM)); 
          }

         if (GET_CLAN_RANK(d->character) == 7) {
          sprintf(bufc + strlen(bufc), "%s[%s%2d %s%s]%s %s the %s %s-- %s%s%s\r\n", 
           CBGRN(ch, C_NRM), CCNRM(ch, C_NRM), GET_LEVEL(d->character),
           CLASS_ABBR(d->character), CBGRN(ch, C_NRM), CCNRM(ch, C_NRM), GET_NAME(d->character),  
           get_rank_name(GET_CLAN(d->character), GET_CLAN_RANK(d->character)),
		   CBWHT(ch, C_NRM), CBBLK(ch, C_NRM),
           (world[IN_ROOM(d->character)].zone == world[IN_ROOM(ch)].zone ?
           world[IN_ROOM(d->character)].name : 
           zone_table[world[IN_ROOM(d->character)].zone].name), CCNRM(ch, C_NRM)); 
          }

         if (GET_CLAN_RANK(d->character) == 6) {
          sprintf(bufc + strlen(bufc), "%s[%s%2d %s%s]%s %s the %s %s-- %s%s%s\r\n", 
           CBGRN(ch, C_NRM), CCNRM(ch, C_NRM), GET_LEVEL(d->character),
           CLASS_ABBR(d->character), CBGRN(ch, C_NRM), CCNRM(ch, C_NRM), GET_NAME(d->character),  
           get_rank_name(GET_CLAN(d->character), GET_CLAN_RANK(d->character)),
		   CBWHT(ch, C_NRM), CBBLK(ch, C_NRM),
           (world[IN_ROOM(d->character)].zone == world[IN_ROOM(ch)].zone ?
           world[IN_ROOM(d->character)].name : 
           zone_table[world[IN_ROOM(d->character)].zone].name), CCNRM(ch, C_NRM)); 
          }

         if (GET_CLAN_RANK(d->character) == 5) {
          sprintf(bufc + strlen(bufc), "%s[%s%2d %s%s]%s %s the %s %s-- %s%s%s\r\n", 
           CBGRN(ch, C_NRM), CCNRM(ch, C_NRM), GET_LEVEL(d->character),
           CLASS_ABBR(d->character), CBGRN(ch, C_NRM), CCNRM(ch, C_NRM), GET_NAME(d->character),  
           get_rank_name(GET_CLAN(d->character), GET_CLAN_RANK(d->character)),
		   CBWHT(ch, C_NRM), CBBLK(ch, C_NRM),
           (world[IN_ROOM(d->character)].zone == world[IN_ROOM(ch)].zone ?
           world[IN_ROOM(d->character)].name : 
           zone_table[world[IN_ROOM(d->character)].zone].name), CCNRM(ch, C_NRM)); 
          }

         if (GET_CLAN_RANK(d->character) == 4) {
          sprintf(bufc + strlen(bufc), "%s[%s%2d %s%s]%s %s the %s %s-- %s%s%s\r\n", 
           CBGRN(ch, C_NRM), CCNRM(ch, C_NRM), GET_LEVEL(d->character),
           CLASS_ABBR(d->character), CBGRN(ch, C_NRM), CCNRM(ch, C_NRM), GET_NAME(d->character),  
           get_rank_name(GET_CLAN(d->character), GET_CLAN_RANK(d->character)),
		   CBWHT(ch, C_NRM), CBBLK(ch, C_NRM),
           (world[IN_ROOM(d->character)].zone == world[IN_ROOM(ch)].zone ?
           world[IN_ROOM(d->character)].name : 
           zone_table[world[IN_ROOM(d->character)].zone].name), CCNRM(ch, C_NRM));  
          }

         if (GET_CLAN_RANK(d->character) == 3) {
          sprintf(bufc + strlen(bufc), "%s[%s%2d %s%s]%s %s the %s %s-- %s%s%s\r\n", 
           CBGRN(ch, C_NRM), CCNRM(ch, C_NRM), GET_LEVEL(d->character),
           CLASS_ABBR(d->character), CBGRN(ch, C_NRM), CCNRM(ch, C_NRM), GET_NAME(d->character),  
           get_rank_name(GET_CLAN(d->character), GET_CLAN_RANK(d->character)),
		   CBWHT(ch, C_NRM), CBBLK(ch, C_NRM),
           (world[IN_ROOM(d->character)].zone == world[IN_ROOM(ch)].zone ?
           world[IN_ROOM(d->character)].name : 
           zone_table[world[IN_ROOM(d->character)].zone].name), CCNRM(ch, C_NRM)); 
          }

         if (GET_CLAN_RANK(d->character) == 2) {
          sprintf(bufc + strlen(bufc), "%s[%s%2d %s%s]%s %s the %s %s-- %s%s%s\r\n", 
           CBGRN(ch, C_NRM), CCNRM(ch, C_NRM), GET_LEVEL(d->character),
           CLASS_ABBR(d->character), CBGRN(ch, C_NRM), CCNRM(ch, C_NRM), GET_NAME(d->character),  
           get_rank_name(GET_CLAN(d->character), GET_CLAN_RANK(d->character)),
		   CBWHT(ch, C_NRM), CBBLK(ch, C_NRM),
           (world[IN_ROOM(d->character)].zone == world[IN_ROOM(ch)].zone ?
           world[IN_ROOM(d->character)].name : 
           zone_table[world[IN_ROOM(d->character)].zone].name), CCNRM(ch, C_NRM)); 
          }

         if (GET_CLAN_RANK(d->character) == 1) {
          sprintf(bufc + strlen(bufc), "%s[%s%2d %s%s]%s %s the %s %s-- %s%s%s\r\n", 
           CBGRN(ch, C_NRM), CCNRM(ch, C_NRM), GET_LEVEL(d->character),
           CLASS_ABBR(d->character), CBGRN(ch, C_NRM), CCNRM(ch, C_NRM), GET_NAME(d->character),  
           get_rank_name(GET_CLAN(d->character), GET_CLAN_RANK(d->character)),
		   CBWHT(ch, C_NRM), CBBLK(ch, C_NRM),
           (world[IN_ROOM(d->character)].zone == world[IN_ROOM(ch)].zone ?
           world[IN_ROOM(d->character)].name : 
           zone_table[world[IN_ROOM(d->character)].zone].name), CCNRM(ch, C_NRM)); 
          }
       }
     }
	}

         strcat(bufa, bufb);
         strcat(bufa, bufc);
         strcat(bufa, bufd);
         strcat(bufa, bufe);
         strcat(bufa, buff);
         strcat(bufa, bufg);
         strcat(bufa, bufh);
         strcat(bufa, bufi);
         strcat(bufa, bufj);

         send_to_char(ch, "%s", bufa);

    return;
}

void do_clan_members(struct char_data *ch)
{
 int i, k = 0;
 char buf[MAX_STRING_LENGTH] = "";

 send_to_char(ch, "Members of clan %s: \r\n", get_clan_name(GET_CLAN(ch)));

  for (i = 0; i <= top_of_p_table; i++ ) {
   if ((player_table + i)->clan_name ==  GET_CLAN(ch)) {

 send_to_char(ch, "%s  [%s%s%s] %-15.15s", buf, ((player_table + i)->clan_rank == 10 ?
  CCCYN(ch, C_NRM) : CCNRM(ch, C_NRM)), get_rank_name((player_table + i)->clan_name,
  (player_table + i)->clan_rank), CCNRM(ch, C_NRM), (player_table + i)->name);

    k++;
    if (k == 3) {
      k = 0;
      send_to_char(ch, "\r\n");
    }
  }
 }
 send_to_char(ch, "\r\n");
}

void do_clan_say(struct char_data *ch, char *msg)
{
    struct descriptor_data *d;
	char buf[MAX_STRING_LENGTH] = "";

    if (GET_CLAN_RANK(ch) < 1)
    {
        send_to_char(ch, "You need to be an initiate or higher in a clan.\r\n");
        return;
    }

    if (!*msg)
      send_to_char(ch, "Yes, but WHAT do you want to say to the clan?\r\n");
    else {

	skip_spaces(&msg);

    for (d = descriptor_list; d; d = d->next)
    {
        if (d->connected)
        {
            continue;
        }

        if (GET_CLAN(d->character) == GET_CLAN(ch)) 
        {
            *buf = '\0';

            if (GET_NAME(d->character) == GET_NAME(ch))
            {
             send_to_char(d->character, "%sYou clan-say, '%s'%s\r\n",
                    CCCYN(ch, C_NRM), msg, CCNRM(ch, C_NRM));
            }
            else {
		     send_to_char(d->character, "%s%s clan-says, '%s'%s\r\n",
                    CCCYN(ch, C_NRM), CAN_SEE(d->character, ch) ? GET_NAME(ch) : "Someone", 
					msg, CCNRM(ch, C_NRM));
            }
        }
    }
 }
    return;

}

void do_clan_enroll(struct char_data *ch, struct char_data *victim)
{
    struct descriptor_data *d;

    if (GET_LEVEL(victim) < 10) {
     send_to_char(ch, "A character must be at least level 10 to join a clan.\r\n"); 
     return; }

    if (!GET_CLAN_RANK(victim))
    {
        GET_CLAN(victim) = GET_CLAN(ch);
        GET_CLAN_RANK(victim) = CLAN_RANK_INITIATE;
		
       mudlog(NRM, MAX(LVL_BUILDER, GET_INVIS_LEV(ch)), TRUE, 
	    "(GC) %s has enrolled %s into clan %s.", GET_NAME(ch),
        GET_NAME(victim), get_clan_name(GET_CLAN(ch)));

    for (d = descriptor_list; d; d = d->next)
    {
        if (d->connected)
        {
            continue;
        }

        if (GET_CLAN(d->character) == GET_CLAN(ch)) { 

         if (GET_NAME(d->character) == GET_NAME(victim)) {
          send_to_char(d->character, "%s- %s has enrolled you into %s! -%s\r\n",
           CCCYN(ch, C_NRM), GET_NAME(ch), get_clan_name(GET_CLAN(ch)), CCNRM(ch, C_NRM));
         }

         else if (GET_NAME(d->character) == GET_NAME(ch)) {
          send_to_char(d->character, "%s- You have enrolled %s into %s! -%s\r\n",
           CCCYN(ch, C_NRM), GET_NAME(victim), get_clan_name(GET_CLAN(ch)), CCNRM(ch, C_NRM));
         }

        else {
         send_to_char(d->character, "%s- %s has enrolled %s into %s! -%s\r\n",
          CCCYN(ch, C_NRM), GET_NAME(ch), GET_NAME(victim), 
          get_clan_name(GET_CLAN(ch)), CCNRM(ch, C_NRM));
       }
      }
     }
    }
    else
    {
        send_to_char(ch, "That person already belongs to a clan..\r\n");
    }
    return;
}

void do_clan_dismiss(struct char_data *ch, struct char_data *victim)
{
    struct descriptor_data *d;

    for (d = descriptor_list; d; d = d->next)
    {
        if (d->connected)
        {
            continue;
        }

        if (GET_CLAN(d->character) == GET_CLAN(ch)) { 

         if (GET_NAME(d->character) == GET_NAME(victim)) {
          send_to_char(d->character, "%s- %s has dismissed you from %s! -%s\r\n",
           CCCYN(ch, C_NRM), GET_NAME(ch), get_clan_name(GET_CLAN(ch)), CCNRM(ch, C_NRM));
         }

         else if (GET_NAME(d->character) == GET_NAME(ch)) {
          send_to_char(d->character, "%s- You have dismissed %s from %s! -%s\r\n",
           CCCYN(ch, C_NRM), GET_NAME(victim), get_clan_name(GET_CLAN(ch)), CCNRM(ch, C_NRM));
         }

        else {
         send_to_char(d->character, "%s- %s has dismissed %s from %s! -%s\r\n",
          CCCYN(ch, C_NRM), GET_NAME(ch), GET_NAME(victim), get_clan_name(GET_CLAN(ch)), CCNRM(ch, C_NRM));
       }
      }
     }

	mudlog(NRM, MAX(LVL_BUILDER, GET_INVIS_LEV(ch)), TRUE, 
	   "(GC) %s has dismissed %s from clan %s.", GET_NAME(ch),
       GET_NAME(victim), get_clan_name(GET_CLAN(ch))); 
	 
    GET_CLAN(victim) = 0;
    GET_CLAN_RANK(victim) = 0;

    return;
}

void do_clan_resign(struct char_data *ch)
{
    struct descriptor_data *d;

    for (d = descriptor_list; d; d = d->next)
    {
        if (d->connected)
        {
            continue;
        }

        if (GET_CLAN(d->character) == GET_CLAN(ch)) { 

         if (GET_NAME(d->character) == GET_NAME(ch)) {
          send_to_char(d->character, "%s- You have resigned from %s! -%s\r\n",
           CCCYN(ch, C_NRM), get_clan_name(GET_CLAN(ch)), CCNRM(ch, C_NRM));
         }

        else {
         send_to_char(d->character, "%s- %s has resigned from %s! -%s\r\n",
          CCCYN(ch, C_NRM), GET_NAME(ch), get_clan_name(GET_CLAN(ch)), CCNRM(ch, C_NRM));
       }
      }
     }
	
	  mudlog(NRM, MAX(LVL_BUILDER, GET_INVIS_LEV(ch)), TRUE, "(GC) %s has resigned from clan %s.", GET_NAME(ch),
      get_clan_name(GET_CLAN(ch)));
	  
    GET_CLAN(ch) = 0;
    GET_CLAN_RANK(ch) = 0;
	
    return;
}

void do_clan_promote(struct char_data *ch, struct char_data *victim)
{
    struct descriptor_data *d;

    if (GET_CLAN_RANK(victim) < CLAN_RANK_MASTER)
    {
        GET_CLAN_RANK(victim)++;

    for (d = descriptor_list; d; d = d->next)
    {
        if (d->connected)
        {
            continue;
        }

        if (GET_CLAN(d->character) == GET_CLAN(ch)) { 

         if (GET_NAME(d->character) == GET_NAME(victim)) {
          send_to_char(d->character, "%s- %s has promoted you to %s! -%s\r\n",
           CCCYN(ch, C_NRM), GET_NAME(ch),  
           get_rank_name(GET_CLAN(victim),GET_CLAN_RANK(victim)), CCNRM(ch, C_NRM));
         }

         else if (GET_NAME(d->character) == GET_NAME(ch)) {
          send_to_char(d->character, "%s- You have promoted %s to %s! -%s\r\n",
           CCCYN(ch, C_NRM), GET_NAME(victim),  
           get_rank_name(GET_CLAN(victim), GET_CLAN_RANK(victim)), CCNRM(ch, C_NRM));
         }

        else {
         send_to_char(d->character, "%s- %s has promoted %s to %s! -%s\r\n",
          CCCYN(ch, C_NRM), GET_NAME(ch), GET_NAME(victim), 
          get_rank_name(GET_CLAN(victim), GET_CLAN_RANK(victim)), CCNRM(ch, C_NRM));
       }
      }  
     }
    }
    else
    {
        send_to_char(ch, "You can not promote this person any further.\r\n");
    }
    return;
}

void do_clan_demote(struct char_data *ch, struct char_data *victim)
{
    struct descriptor_data *d;

    if (GET_CLAN_RANK(victim) > CLAN_RANK_INITIATE) 
    {
        GET_CLAN_RANK(victim)--;

    for (d = descriptor_list; d; d = d->next)
    {
        if (d->connected)
        {
            continue;
        }

        if (GET_CLAN(d->character) == GET_CLAN(ch)) { 

         if (GET_NAME(d->character) == GET_NAME(victim)) {
          send_to_char(d->character, "%s- %s has demoted you to %s! -%s\r\n",
           CCCYN(ch, C_NRM), GET_NAME(ch),  
           get_rank_name(GET_CLAN(victim), GET_CLAN_RANK(victim)), CCNRM(ch, C_NRM));
         }

         else if (GET_NAME(d->character) == GET_NAME(ch)) {
          send_to_char(d->character, "%s- You have demoted %s to %s! -%s\r\n",
           CCCYN(ch, C_NRM), GET_NAME(victim),  
           get_rank_name(GET_CLAN(victim), GET_CLAN_RANK(victim)), CCNRM(ch, C_NRM));
         }

        else {
         send_to_char(d->character, "%s- %s has demoted %s to %s! -%s\r\n",
          CCCYN(ch, C_NRM), GET_NAME(ch), GET_NAME(victim), 
          get_rank_name(GET_CLAN(victim), GET_CLAN_RANK(victim)), CCNRM(ch, C_NRM));
       }
      }
     }
    }
    else
    {
        send_to_char(ch, "You can not demote this person any further!\r\n");
    }
   
    return;
}

/* reserved for the implementor only */
ACMD(do_assign)
{
    struct char_data *victim = NULL;
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    int clan_number;

    half_chop(argument, arg1, arg2);

    if (!*arg1 || !*arg2)
    {
        send_to_char(ch, "Usage: assign <clan number> <leader>\r\n");
        return;
    }

    if (!(victim = get_char_vis(ch, arg2, NULL, FIND_CHAR_ROOM)))
    {
        send_to_char(ch, "That person is not available.\r\n");
        return;
    }

    if (GET_CLAN_RANK(victim))
    {
        send_to_char(ch, "Can not designate someone already in a clan!\r\n");
        return;
    }

    if (GET_LEVEL(victim) < 10) {
     send_to_char(ch, "A character must be at least level 10 to join a clan.\r\n"); 
     return; } 


    clan_number = atoi(arg1);

      GET_CLAN(victim) = clan_number;
      GET_CLAN_RANK(victim) = 10;

      send_to_char(ch, "%s", CONFIG_OK);
	  
	  mudlog(NRM, MAX(LVL_BUILDER, GET_INVIS_LEV(ch)), TRUE, 
	   "(GC) %s has assigned clan %s to %s.", GET_NAME(ch),
       get_clan_name(GET_CLAN(victim)), GET_NAME(victim));

      send_to_char(victim, "You have been assigned leadership of clan %s%s%s.\r\n",
       CCCYN(ch, C_NRM), get_clan_name(GET_CLAN(victim)), CCNRM(ch, C_NRM));
}

char *get_clan_name(int clan)
{
  switch(clan)
   {
    case 1: return "<*-LEGION-*>"; break;
    case 2: return "<===TesT}=o"; break;
    default : return "none"; break;
   }
}

char *get_rank_name(int clan, int rank)
{
 if (clan == 1) {
    switch (rank)
    {
        case 1: return "xInitiate"; break;
        case 2: return "xRecruit"; break;
        case 3: return "xApprentice"; break;
        case 4: return "xMember"; break;
        case 5: return "xClannie"; break;
        case 6: return "xBrother"; break;
        case 7: return "xHighMember"; break;
        case 8: return "xCommander"; break;
        case 9: return "xMaster"; break;
        case 10: return "xLeader"; break;
        default: return "Uninitiated"; break;
    }
 }

 else {
    switch (rank)
    {
        case 1: return "Initiate"; break;
        case 2: return "Recruit"; break;
        case 3: return "Apprentice"; break;
        case 4: return "Member"; break;
        case 5: return "Clannie"; break;
        case 6: return "Brother"; break;
        case 7: return "HighMember"; break;
        case 8: return "Commander"; break;
        case 9: return "Master"; break;
        case 10: return "Leader"; break;
        default: return "Uninitiated"; break;
    }
 }
}
