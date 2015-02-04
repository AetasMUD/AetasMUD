/**
* @file class.h
* Header file for class specific functions and variables.
* 
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*                                                                        
* All rights reserved.  See license for complete information.                                                                
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University 
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               
*
*/
#ifndef _CLASS_H_
#define _CLASS_H_

/* Functions available through class.c */
int backstab_mult(int level);
void do_start(struct char_data *ch);
bitvector_t find_class_bitvector(const char *arg);
int invalid_class(struct char_data *ch, struct obj_data *obj);
int invalid_needed_class(struct char_data *ch, struct obj_data *obj);
int invalid_race(struct char_data *ch, struct obj_data *obj);
int level_exp(int chclass, int level);
int parse_class(char arg);
int parse_class_by_abbrev(char *arg);
int parse_race(char arg);
int parse_race_by_abbrev(char *arg);
int parse_mage_race(char *arg);
int parse_necro_race(char *arg);
int parse_monk_race(char *arg);
int parse_knig_race(char *arg);
int parse_cler_race(char *arg);
int parse_barb_race(char *arg);
int parse_druid_race(char *arg);
int parse_psio_race(char *arg);
void roll_real_abils(struct char_data *ch);
byte saving_throws(int class_num, int type, int level);
int thaco(int class_num, int level);
const char *title_female(int chclass, int level);
const char *title_male(int chclass, int level);

/* Global variables */

#ifndef __CLASS_C__  

extern const char *class_abbrevs[];
extern const char *class_types[];
extern const char *classplural_types[];
extern const char *race_types[];
extern const char *raceplural_types[];
extern const char *racepos_types[];
extern const char *class_menu;
extern const char *race_menu;
extern const char *race_mage_menu;
extern const char *race_necro_menu;
extern const char *race_monk_menu;
extern const char *race_knig_menu;
extern const char *race_cler_menu;
extern const char *race_barb_menu;
extern const char *race_druid_menu;
extern const char *race_psio_menu;
extern int prac_params[][NUM_CLASSES];
extern struct guild_info_type guild_info[];
extern const char *get_clan_name(int clan);
extern const char *get_rank_name(int clan, int rank);

#endif /* __CLASS_C__ */

#endif /* _CLASS_H_*/
