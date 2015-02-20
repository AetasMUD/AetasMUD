/**************************************************************************
*  File: class.c                                           Part of tbaMUD *
*  Usage: Source file for class-specific code.                            *
*                                                                         *
*  All rights reserved.  See license for complete information.            *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
**************************************************************************/

/** Help buffer the global variable definitions */
#define __CLASS_C__

/* This file attempts to concentrate most of the code which must be changed
 * in order for new classes to be added.  If you're adding a new class, you 
 * should go through this entire file from beginning to end and add the 
 * appropriate new special cases for your new class. */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "spells.h"
#include "interpreter.h"
#include "constants.h"
#include "act.h"


/* Names first */
const char *class_abbrevs[] = {
  "Knig",
  "Cler",
  "Barb",
  "Rang",
  "Mage",
  "Monk",
  "Necr",
  "Drui",
  "Thie",
  "Psio",
  "Unsd",   /*  10 */
  "Unsd", "Unsd", "Unsd", "Unsd", "Unsd", 
  "Unsd", "Unsd", "Unsd", "Unsd", "Unsd",   /*  20 */
  "Unsd", "Unsd", "Unsd", "Unsd", "Unsd", 
  "Unsd", "Unsd", "Unsd", "Unsd", "Unsd",   /*  30 */
  "Unsd", "Unsd", "Unsd", "Unsd", "Unsd", 
  "Unsd", "Unsd", "Unsd", "Unsd", "Unsd",   /*  40 */
  "Unsd", "Unsd", "Unsd", "Unsd", "Unsd", 
  "Unsd", "Unsd", "Unsd", "Unsd",
  "None",   /* 50 Start Mob Classes */
  "Mobl",
  "\n"
};

const char *class_types[] = {
  "Knight",     /*   0 */
  "Cleric",
  "Barbarian",
  "Ranger",
  "Mage",
  "Monk",
  "Necromancer",
  "Druid",
  "Thief",
  "Psionicist",
  "Unused",     /*  10 */
  "Unused", "Unused", "Unused", "Unused", "Unused",
  "Unused", "Unused", "Unused", "Unused", "Unused", /*  20 */
  "Unused", "Unused", "Unused", "Unused", "Unused",
  "Unused", "Unused", "Unused", "Unused", "Unused", /*  30 */
  "Unused", "Unused", "Unused", "Unused", "Unused",
  "Unused", "Unused", "Unused", "Unused", "Unused", /*  40 */
  "Unused", "Unused", "Unused", "Unused", "Unused",
  "Unused", "Unused", "Unused", "Unused",
  "None",       /*  50 Start Mob Classes */
  "Mobile",
  "\n"
};

const char *classplural_types[] = {
  "Knights",
  "Clerics",
  "Barbarians",
  "Rangers",
  "Mages",
  "Monks",
  "Necromancers",
  "Druids",
  "Thieves",
  "Psionicists",
  "Unused",     /*  10 */
  "Unused", "Unused", "Unused", "Unused", "Unused",
  "Unused", "Unused", "Unused", "Unused", "Unused", /*  20 */
  "Unused", "Unused", "Unused", "Unused", "Unused",
  "Unused", "Unused", "Unused", "Unused", "Unused", /*  30 */
  "Unused", "Unused", "Unused", "Unused", "Unused",
  "Unused", "Unused", "Unused", "Unused", "Unused", /*  40 */
  "Unused", "Unused", "Unused", "Unused", "Unused",
  "Unused", "Unused", "Unused", "Unused",
  "Classless",  /*  50 Start Mob Classes */
  "Mobiles",
  "\n"
};

const char *race_abbrevs[] = {
  "Elf",
  "Dwf",
  "Hum",
  "Kdr",
  "HOr",
  "HOg",
  "Bak",
  "Mob",
  "Orc",
  "Ogr",
  "Drg",
  "UnD",
  "Cae",
  "Ani",
  "Dem",
  "\n"
};

const char *race_types[] = {
  "Elf",
  "Dwarf",
  "Human",
  "Kender",
  "Half-Orc",
  "Half-Ogre",
  "Bakali",
  "Mobile",
  "Orc",
  "Ogre",
  "Dragon",
  "Undead",
  "Caervuni",
  "Animal",
  "Demon",
  "\n"
};

const char *raceplural_types[] = {
  "Elves",
  "Dwarves",
  "Humans",
  "Kenders",
  "Half-Orcs",
  "Half-Ogres",
  "Bakali",
  "Mobiles",
  "Orcs",
  "Ogres",
  "Dragons",
  "Undead",
  "Caervuni",
  "Animals",
  "Demons",
  "\n"
};

const char *racepos_types[] = {
  "Elven",
  "Dwarven",
  "Human",
  "Kender",
  "Half-Orc",
  "Half-Ogre",
  "Bakali",
  "Mobile",
  "Orc",
  "Ogre",
  "Dragonic",
  "Undead",
  "Caervuni",
  "Animal",
  "Demon",
  "\n"
};


/* The menu for choosing a class in interpreter.c: */
const char *class_menu =
"\r\n\r\n"
"\tcSelecting a class is perhaps the most important choice that you will\r\n"
"make for a character. Character class determines which spells and skills\r\n"
"the character can use, as well as which equipment they can use and which\r\n"
"alignment they must remain at.\tn\r\n"
"\r\n\tGSelect a class:\tn\r\n\r\n"
"     \tcKnight      \tw-- \tnWarrior devoted to a deity.\r\n"
"     \tcCleric      \tw-- \tnOne who is devoted to and dependant on a deity.\r\n"
"     \tcBarbarian   \tw-- \tnWarrior relying on brute strength.\r\n"
"     \tcRanger      \tw-- \tnWanderer of lands and roamer of the forests.\r\n" 
"     \tcMage        \tw-- \tnOne who wields arcane powers.\r\n"
"     \tcMonk        \tw-- \tnWarrior relying on personal discipline and balance.\r\n"
"     \tcNecromancer \tw-- \tnUser of magic of the netherworld.\r\n"
"     \tcDruid       \tw-- \tnOne devoted to the earth and its protection.\r\n"
"     \tcThief       \tw-- \tnBackstabber and pickpocket.\r\n"
"     \tcPsionicist  \tw-- \tnOne who can alter the perceptions of the mind.\r\n";


/* The menu for choosing a race in interpreter.c: */
const char *race_menu =
"\r\n"
"\tcA character's race determines what they look like, how they react\r\n"
"in some situations, their physical and mental capabilities, and to\r\n"
"some extent which equipment they can use.\tn\r\n\r\n"
"\tGSelect a race:\tn\r\n\r\n"
"  \tcHuman        \tc--\tn Humans have average attributes.\r\n"
"  \tcElf          \tc--\tn Elves are usually shorter and lighter than humans\r\n"
"                    with pointy ears and almond eyes. They receive bonuses to\r\n"
"                    intelligence and dexterity, but have reduced strength and\r\n"
"                    constitution.\r\n"
"  \tcDwarf        \tc--\tn Dwarves are usually quite shorter than humans,\r\n"
"                    are rather stocky, and normally wear beards. They receive\r\n"
"                    bonuses to strength and constitution, but have reduced\r\n"
"                    dexterity and intelligence.\r\n"
"  \tcKender       \tc--\tn Kender are often mistaken for teenage humans, even\r\n"
"                    with their pointy ears. They receive bonuses for dexterity,\r\n"
"                    but have reduced intelligence and strength.\r\n" 
"  Half-\tcOrc     \tc--\tn Half-orcs are usually shorter than humans, with a face\r\n"
"                    that resembles a pig, and skin that is green and brown.\r\n"
"                    They receive bonuses to strength, but reductions to\r\n"
"                    intelligence and charisma.\r\n"
"  Half-\tcOgre    \tc--\tn Half-ogres usually stand a couple of feet taller than\r\n"
"                    humans, and have been described as over-sized neanderthals.\r\n"
"                    They receive large bonuses to strength, but suffer with\r\n"
"                    reduced wisdom, intelligence and charisma.\r\n"
"  \tcBakali       \tc--\tn Bakali are a race of lizard type humanoids which\r\n"
"                    stand a bit taller than humans, and weigh more. They receive\r\n"
"                    bonuses to strength and constitution, but have reduced\r\n"
"                    wisdom and charisma.\r\n";

/* added class/race menus */

const char *race_mage_menu =
"\r\n\r\n"
"\tcThe following races can be Mages. Mages may choose to be any alignment.\tn\r\n"
"\r\n"
"\tGSelect a race:\r\n\r\n"
"  \tcHuman        \tc--\tn Humans have average attributes.\r\n"
"  \tcElf          \tc--\tn Elves are usually shorter and lighter than humans\r\n"
"                    with pointy ears and almond eyes. They receive bonuses to\r\n"
"                    intelligence and dexterity, but have reduced strength and\r\n"
"                    constitution.\r\n"
"  \tcKender       \tc--\tn Kender are often mistaken for teenage humans, even\r\n"
"                    with their pointy ears. They receive bonuses for dexterity,\r\n"
"                    but have reduced intelligence and strength.\r\n" 
"  Half-\tcOrc     \tc--\tn Half-orcs are usually shorter than humans, with a face\r\n"
"                    that resembles a pig, and skin that is green and brown.\r\n"
"                    They receive bonuses to strength, but reductions to\r\n"
"                    intelligence and charisma.\r\n"
"  \tcBakali       \tc--\tn Bakali are a race of lizard type humanoids which\r\n"
"                    stand a bit taller than humans, and weigh more. They receive\r\n"
"                    bonuses to strength and constitution, but have reduced\r\n"
"                    wisdom and charisma.\r\n";


const char *race_necro_menu =
"\r\n\r\n"
"\tcThe following races can be Necromancers. Necromancers may choose to be\r\n"
"any alignment.\tn\r\n"
"\r\n"
"\tGSelect a race:\tn\r\n\r\n"
"  \tcHuman        \tc--\tn Humans have average attributes.\r\n"
"  \tcElf          \tc--\tn Elves are usually shorter and lighter than humans\r\n"
"                    with pointy ears and almond eyes. They receive bonuses to\r\n"
"                    intelligence and dexterity, but have reduced strength and\r\n"
"                    constitution.\r\n"
"  Half-\tcOrc     \tc--\tn Half-orcs are usually shorter than humans, with a face\r\n"
"                    that resembles a pig, and skin that is green and brown.\r\n"
"                    They receive bonuses to strength, but reductions to\r\n"
"                    intelligence and charisma.\r\n"
"  \tcBakali       \tc--\tn Bakali are a race of lizard type humanoids which\r\n"
"                    stand a bit taller than humans, and weigh more. They receive\r\n"
"                    bonuses to strength and constitution, but have reduced\r\n"
"                    wisdom and charisma.\r\n";

const char *race_monk_menu =
"\r\n"
"\tcThe following races are able to be Monks. Monks need to be at a neutral\r\n"
"alignment in order to remain in balance with the forces that give them\r\n"
"their abilities. Therefore, your alignment will automatically be set to\r\n"
"neutral after choosing your race.\tn\r\n"
"\r\n"
"\tGSelect a race:\tn\r\n\r\n"
"  \tcHuman        \tc--\tn Humans have average attributes.\r\n"
"  \tcElf          \tc--\tn Elves are usually shorter and lighter than humans\r\n"
"                    with pointy ears and almond eyes. They receive bonuses to\r\n"
"                    intelligence and dexterity, but have reduced strength and\r\n"
"                    constitution.\r\n"
"  \tcKender       \tc--\tn Kender are often mistaken for teenage humans, even\r\n"
"                    with their pointy ears. They receive bonuses for dexterity,\r\n"
"                    but have reduced intelligence and strength.\r\n" 
"  \tcBakali       \tc--\tn Bakali are a race of lizard type humanoids which\r\n"
"                    stand a bit taller than humans, and weigh more. They receive\r\n"
"                    bonuses to strength and constitution, but have reduced\r\n"
"                    wisdom and charisma.\r\n";

const char *race_knig_menu =
"\r\n\r\n"
"\tcThe following races are able to be Knights. Knights must have alignments\r\n"
"of either good or evil in order to keep the favor of their chosen deity.\tn\r\n"
"\r\n"
"\tGSelect a race:\tn\r\n\r\n"
"  \tcHuman        \tc--\tn Humans have average attributes.\r\n"
"  \tcElf          \tc--\tn Elves are usually shorter and lighter than humans\r\n"
"                    with pointy ears and almond eyes. They receive bonuses to\r\n"
"                    intelligence and dexterity, but have reduced strength and\r\n"
"                    constitution.\r\n"
"  \tcDwarf        \tc--\tn Dwarves are usually quite shorter than humans,\r\n"
"                    are rather stocky, and normally wear beards. They receive\r\n"
"                    bonuses to strength and constitution, but have reduced\r\n"
"                    dexterity and intelligence.\r\n";

const char *race_cler_menu =
"\r\n\r\n"
"\tcThe following races are able to be Clerics. Clerics must have alignments\r\n"
"of either good or evil in order to keep the favor of their chosen deity.\tn\r\n"
"\r\n"
"\tGSelect a race:\tn\r\n\r\n"
"  \tcHuman        \tc--\tn Humans have average attributes.\r\n"
"  \tcElf          \tc--\tn Elves are usually shorter and lighter than humans\r\n"
"                    with pointy ears and almond eyes. They receive bonuses to\r\n"
"                    intelligence and dexterity, but have reduced strength and\r\n"
"                    constitution.\r\n"
"  \tcDwarf        \tc--\tn Dwarves are usually quite shorter than humans,\r\n"
"                    are rather stocky, and normally wear beards. They receive\r\n"
"                    bonuses to strength and constitution, but have reduced\r\n"
"                    dexterity and intelligence.\r\n";

const char *race_barb_menu =
"\r\n\r\n"
"\tcThe following races can be Barbarians. Barbarians may choose to be any alignment.\tn\r\n"
"\r\n"
"\tGSelect a race:\tn\r\n\r\n"
"  \tcHuman        \tc--\tn Humans have average attributes.\r\n"
"  \tcDwarf        \tc--\tn Dwarves are usually quite shorter than humans,\r\n"
"                    are rather stocky, and normally wear beards. They receive\r\n"
"                    bonuses to strength and constitution, but have reduced\r\n"
"                    dexterity and intelligence.\r\n"
"  Half-\tcOrc     \tc--\tn Half-orcs are usually shorter than humans, with a face\r\n"
"                    that resembles a pig, and skin that is green and brown.\r\n"
"                    They receive bonuses to strength, but reductions to\r\n"
"                    intelligence and charisma.\r\n"
"  Half-\tcOgre    \tc--\tn Half-ogres usually stand a couple of feet taller than\r\n"
"                    humans, and have been described as over-sized neanderthals.\r\n"
"                    They receive large bonuses to strength, but suffer with\r\n"
"                    reduced wisdom, intelligence and charisma.\r\n"
"  \tcBakali       \tc--\tn Bakali are a race of lizard type humanoids which\r\n"
"                    stand a bit taller than humans, and weigh more. They receive\r\n"
"                    bonuses to strength and constitution, but have reduced\r\n"
"                    wisdom and charisma.\r\n";

const char *race_druid_menu =
"\r\n"
"\tcThe following races are able to be Druids. Druids need to be at a neutral\r\n"
"alignment in order to remain in balance with the forces of nature that\r\n"
"they rely on in order to use their unique abilities. Therefore, your\r\n"
"alignment will automatically be set to neutral after choosing your race.\tn\r\n"
"\r\n"
"\tGSelect a race:\tn\r\n\r\n"
"  \tcHuman        \tc--\tn Humans have average attributes.\r\n"
"  \tcElf          \tc--\tn Elves are usually shorter and lighter than humans\r\n"
"                    with pointy ears and almond eyes. They receive bonuses to\r\n"
"                    intelligence and dexterity, but have reduced strength and\r\n"
"                    constitution.\r\n"
"  \tcDwarf        \tc--\tn Dwarves are usually quite shorter than humans,\r\n"
"                    are rather stocky, and normally wear beards. They receive\r\n"
"                    bonuses to strength and constitution, but have reduced\r\n"
"                    dexterity and intelligence.\r\n"
"  \tcBakali       \tc--\tn Bakali are a race of lizard type humanoids which\r\n"
"                    stand a bit taller than humans, and weigh more. They receive\r\n"
"                    bonuses to strength and constitution, but have reduced\r\n"
"                    wisdom and charisma.\r\n";

const char *race_psio_menu =
"\r\n\r\n"
"\tcThe following races can be Psionicists. Psionicists may choose to be\r\n"
"any alignment.\tn\r\n"
"\r\n"
"\tGSelect a race:\tn\r\n\r\n"
"  \tcHuman        \tc--\tn Humans have average attributes.\r\n"
"  \tcElf          \tc--\tn Elves are usually shorter and lighter than humans\r\n"
"                    with pointy ears and almond eyes. They receive bonuses to\r\n"
"                    intelligence and dexterity, but have reduced strength and\r\n"
"                    constitution.\r\n"
"  \tcKender       \tc--\tn Kender are often mistaken for teenage humans, even\r\n"
"                    with their pointy ears. They receive bonuses for dexterity,\r\n"
"                    but have reduced intelligence and strength.\r\n" 
"  \tcBakali       \tc--\tn Bakali are a race of lizard type humanoids which\r\n"
"                    stand a bit taller than humans, and weigh more. They receive\r\n"
"                    bonuses to strength and constitution, but have reduced\r\n"
"                    wisdom and charisma.\r\n";


/* The code to interpret a class letter -- used in interpreter.c when a new 
 * character is selecting a class and by 'set class' in act.wizard.c. */
int parse_class_by_abbrev(char *arg)
{
  skip_spaces(&arg);
  
  if (is_abbrev(arg, "knight"))
    return CLASS_KNIGHT;
  else if (is_abbrev(arg, "cleric"))
    return CLASS_CLERIC;
  else if (is_abbrev(arg, "barbarian"))
    return CLASS_BARBARIAN;
  else if (is_abbrev(arg, "ranger"))
    return CLASS_RANGER;
  else if (is_abbrev(arg, "magic user") || is_abbrev(arg, "mage"))
    return CLASS_MAGE;
  else if (is_abbrev(arg, "monk"))
    return CLASS_MONK;
  else if (is_abbrev(arg, "necromancer"))
    return CLASS_NECROMANCER;
  else if (is_abbrev(arg, "druid"))
    return CLASS_DRUID;
  else if (is_abbrev(arg, "thief"))
    return CLASS_THIEF;
  else if (is_abbrev(arg, "psionicist"))
    return CLASS_PSIONICIST;

  return CLASS_UNDEFINED;
}

/* The code to interpret a class letter -- used in interpreter.c when a new 
 * character is selecting a class and by 'set class' in act.wizard.c. */
// interpreter now uses parse_class_by_abbrev
int parse_class(char arg)
{
  arg = LOWER(arg);
  
  switch (arg) {
  case 'k': return CLASS_KNIGHT;
  case 'c': return CLASS_CLERIC;
  case 'b': return CLASS_BARBARIAN;
  case 'r': return CLASS_RANGER;
  case 'm': return CLASS_MAGE;
  case 'o': return CLASS_MONK;
  case 'n': return CLASS_NECROMANCER;
  case 'd': return CLASS_DRUID;
  case 't': return CLASS_THIEF;
  case 'p': return CLASS_PSIONICIST;
  default:  return CLASS_UNDEFINED;
  }
}

int parse_race(char arg)
{
  arg = LOWER(arg);

  switch (arg) {
  case 'e': return RACE_ELF;
  case 'd': return RACE_DWARF;
  case 'h': return RACE_HUMAN;
  case 'k': return RACE_KENDER;
  case 'o': return RACE_HALF_ORC;
  case 'g': return RACE_HALF_OGRE;
  case 'b': return RACE_BAKALI;
  default:  return RACE_UNDEFINED;
  }
}

// code to choose race from menu by full word not letter
int parse_race_by_abbrev(char *arg)
{
  skip_spaces(&arg);
  
  if (is_abbrev(arg, "elf"))
    return RACE_ELF;
  else if (is_abbrev(arg, "dwarf"))
    return RACE_DWARF;
  else if (is_abbrev(arg, "human"))
    return RACE_HUMAN;
  else if (is_abbrev(arg, "kender"))
    return RACE_KENDER;
  else if (is_abbrev(arg, "half-orc") || is_abbrev(arg, "half orc") || is_abbrev(arg, "halforc") || is_abbrev(arg, "orc"))
    return RACE_HALF_ORC;
  else if (is_abbrev(arg, "half-ogre") || is_abbrev(arg, "half ogre") || is_abbrev(arg, "halfogre") || is_abbrev(arg, "ogre"))
    return RACE_HALF_OGRE;
  else if (is_abbrev(arg, "bakali"))
    return RACE_BAKALI;

  return RACE_UNDEFINED;
}

int parse_mage_race(char *arg)
{
  skip_spaces(&arg);
  
  if (is_abbrev(arg, "elf"))
    return RACE_ELF;
  else if (is_abbrev(arg, "human"))
    return RACE_HUMAN;
  else if (is_abbrev(arg, "kender"))
    return RACE_KENDER;
  else if (is_abbrev(arg, "half-orc") || is_abbrev(arg, "half orc") || is_abbrev(arg, "halforc") || is_abbrev(arg, "orc"))
    return RACE_HALF_ORC;
  else if (is_abbrev(arg, "bakali"))
    return RACE_BAKALI;

  return RACE_UNDEFINED;
}

int parse_necro_race(char *arg)
{
  skip_spaces(&arg);
  
  if (is_abbrev(arg, "elf"))
    return RACE_ELF;
  else if (is_abbrev(arg, "human"))
    return RACE_HUMAN;
  else if (is_abbrev(arg, "half-orc") || is_abbrev(arg, "half orc") || is_abbrev(arg, "halforc") || is_abbrev(arg, "orc"))
    return RACE_HALF_ORC;
  else if (is_abbrev(arg, "bakali"))
    return RACE_BAKALI;

  return RACE_UNDEFINED;
}

int parse_monk_race(char *arg)
{
  skip_spaces(&arg);
  
  if (is_abbrev(arg, "elf"))
    return RACE_ELF;
  else if (is_abbrev(arg, "human"))
    return RACE_HUMAN;
  else if (is_abbrev(arg, "kender"))
    return RACE_KENDER;
  else if (is_abbrev(arg, "bakali"))
    return RACE_BAKALI;

  return RACE_UNDEFINED;
}

int parse_knig_race(char *arg)
{
  skip_spaces(&arg);
  
  if (is_abbrev(arg, "elf"))
    return RACE_ELF;
  else if (is_abbrev(arg, "dwarf"))
    return RACE_DWARF;
  else if (is_abbrev(arg, "human"))
    return RACE_HUMAN;

  return RACE_UNDEFINED;
}

int parse_cler_race(char *arg)
{
  skip_spaces(&arg);
  
  if (is_abbrev(arg, "elf"))
    return RACE_ELF;
  else if (is_abbrev(arg, "dwarf"))
    return RACE_DWARF;
  else if (is_abbrev(arg, "human"))
    return RACE_HUMAN;

  return RACE_UNDEFINED;
}

int parse_barb_race(char *arg)
{
  skip_spaces(&arg);
  
  if (is_abbrev(arg, "dwarf"))
    return RACE_DWARF;
  else if (is_abbrev(arg, "human"))
    return RACE_HUMAN;
  else if (is_abbrev(arg, "half-orc") || is_abbrev(arg, "half orc") || is_abbrev(arg, "halforc") || is_abbrev(arg, "orc"))
    return RACE_HALF_ORC;
  else if (is_abbrev(arg, "half-ogre") || is_abbrev(arg, "half ogre") || is_abbrev(arg, "halfogre") || is_abbrev(arg, "ogre"))
    return RACE_HALF_OGRE;
  else if (is_abbrev(arg, "bakali"))
    return RACE_BAKALI;

  return RACE_UNDEFINED;
}

int parse_druid_race(char *arg)
{
  skip_spaces(&arg);
  
  if (is_abbrev(arg, "elf"))
    return RACE_ELF;
  else if (is_abbrev(arg, "dwarf"))
    return RACE_DWARF;
  else if (is_abbrev(arg, "human"))
    return RACE_HUMAN;
  else if (is_abbrev(arg, "bakali"))
    return RACE_BAKALI;

  return RACE_UNDEFINED;
}

int parse_psio_race(char *arg)
{
  skip_spaces(&arg);
  
  if (is_abbrev(arg, "elf"))
    return RACE_ELF;
  else if (is_abbrev(arg, "dwarf"))
    return RACE_DWARF;
  else if (is_abbrev(arg, "human"))
    return RACE_HUMAN;
  else if (is_abbrev(arg, "kender"))
    return RACE_KENDER;
  else if (is_abbrev(arg, "bakali"))
    return RACE_BAKALI;

  return RACE_UNDEFINED;
}

/* bitvectors (i.e., powers of two) for each class, mainly for use in do_who 
 * and do_users.  Add new classes at the end so that all classes use sequential
 * powers of two (1 << 0, 1 << 1, 1 << 2, 1 << 3, 1 << 4, 1 << 5, etc.) up to 
 * the limit of your bitvector_t, typically 0-31. */
bitvector_t find_class_bitvector(const char *arg)
{
  size_t rpos, ret = 0;

  for (rpos = 0; rpos < strlen(arg); rpos++)
    ret |= (1 << parse_class(arg[rpos]));

  return (ret);
}

/* These are definitions which control the guildmasters for each class. 
 * The  first field (top line) controls the highest percentage skill level a 
 * character of the class is allowed to attain in any skill.  (After this 
 * level, attempts to practice will say "You are already learned in this area."
 *
 * The second line controls the maximum percent gain in learnedness a character
 * is allowed per practice -- in other words, if the random die throw comes out
 * higher than this number, the gain will only be this number instead.
 *
 * The third line controls the minimu percent gain in learnedness a character 
 * is allowed per practice -- in other words, if the random die throw comes 
 * out below this number, the gain will be set up to this number.
 *
 * The fourth line simply sets whether the character knows 'spells' or 'skills'.
 * This does not affect anything except the message given to the character when
 * trying to practice (i.e. "You know of the following spells" vs. "You know of
 * the following skills" */

#define SPELL	0
#define SKILL	1

/* #define LEARNED_LEVEL	0  % known which is considered "learned" */
/* #define MAX_PER_PRAC		1  max percent gain in skill per practice */
/* #define MIN_PER_PRAC		2  min percent gain in skill per practice */
/* #define PRAC_TYPE		3  should it say 'spell' or 'skill'?	*/

int prac_params[4][NUM_CLASSES] = {
 // Kni  CLE	 Bar    Ran    Mag    Mon    nec    dru    THE	  psi 
  {90,	  95,	 85,	90,    95,    90,    95,    95,    85,    95},		
  {30,	  50,	 25,	30,    50,    30,    50,    50,    25,    50},		
  {10,	  25,	 10,	10,    25,    10,    25,    25,    10,    25},	
  {SKILL, SPELL, SKILL,	SKILL, SPELL, SKILL, SPELL, SPELL, SKILL, SPELL}
};


/* The appropriate rooms for each guildmaster/guildguard; controls which types
 * of people the various guildguards let through.  i.e., the first line shows 
 * that from room 3017, only MAGES are allowed to go south. Don't forget 
 * to visit spec_assign.c if you create any new mobiles that should be a guild
 * master or guard so they can act appropriately. If you "recycle" the 
 * existing mobs that are used in other guilds for your new guild, then you 
 * don't have to change that file, only here. Guildguards are now implemented
 * via triggers. This code remains as an example. */
struct guild_info_type guild_info[] = {

/* Midgaard */
 { CLASS_MAGE,    3017,    SOUTH   }, 
 { CLASS_CLERIC,        3004,    NORTH   }, 
 { CLASS_THIEF,         3027,    EAST   }, 
 { CLASS_BARBARIAN,       3021,    EAST   }, 

/* Brass Dragon */
  { -999 /* all */ ,	5065,	WEST	},

/* this must go last -- add new guards above! */
  { -1, NOWHERE, -1}
};

/* Saving throws for : MCTW : PARA, ROD, PETRI, BREATH, SPELL. Levels 0-40. Do 
 * not forget to change extern declaration in magic.c if you add to this. */
byte saving_throws(int class_num, int type, int level)
{
  switch (class_num) {

  case CLASS_KNIGHT:
    switch (type) {
    case SAVING_PARA:	/* Paralyzation */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 69;
      case  3: return 68;
      case  4: return 67;
      case  5: return 67;
      case  6: return 66;
      case  7: return 66;
      case  8: return 65;
      case  9: return 65;
      case 10: return 64;
      case 11: return 64;
      case 12: return 63;
      case 13: return 63;
      case 14: return 62;
      case 15: return 62;
      case 16: return 61;
      case 17: return 61;
      case 18: return 60;
      case 19: return 60;
      case 20: return 59;
      case 21: return 59;
      case 22: return 58;
      case 23: return 58;
      case 24: return 57;
      case 25: return 57;
      case 26: return 56;
      case 27: return 56;
      case 28: return 55;
      case 29: return 55;
      case 30: return 54;
      case 31: return 54;
      case 32: return 53;
      case 33: return 53;
      case 34: return 52;
      case 35: return 52;
      case 36: return 51;
      case 37: return 51;
      case 38: return 50;
      case 39: return 50;
      case 40: return 49;
      case 41: return 48;
      case 42: return 47;
      case 43: return 46;
      case 44: return 45;
      case 45: return 44;
      case 46: return 43;
      case 47: return 41;
      case 48: return 39;
      case 49: return 37;
      case 50: return 35;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for Knight paralyzation saving throw.");
	break;
      }
    case SAVING_ROD:	/* Rods */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 69;
      case  3: return 68;
      case  4: return 67;
      case  5: return 67;
      case  6: return 66;
      case  7: return 66;
      case  8: return 65;
      case  9: return 65;
      case 10: return 64;
      case 11: return 64;
      case 12: return 63;
      case 13: return 63;
      case 14: return 62;
      case 15: return 62;
      case 16: return 61;
      case 17: return 61;
      case 18: return 60;
      case 19: return 60;
      case 20: return 59;
      case 21: return 59;
      case 22: return 58;
      case 23: return 58;
      case 24: return 57;
      case 25: return 57;
      case 26: return 56;
      case 27: return 56;
      case 28: return 55;
      case 29: return 55;
      case 30: return 54;
      case 31: return 54;
      case 32: return 53;
      case 33: return 53;
      case 34: return 52;
      case 35: return 52;
      case 36: return 51;
      case 37: return 51;
      case 38: return 50;
      case 39: return 50;
      case 40: return 49;
      case 41: return 48;
      case 42: return 47;
      case 43: return 46;
      case 44: return 45;
      case 45: return 44;
      case 46: return 43;
      case 47: return 41;
      case 48: return 39;
      case 49: return 37;
      case 50: return 35;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for Knight rod saving throw.");
	break;
      }
    case SAVING_PETRI:	/* Petrification */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 69;
      case  3: return 68;
      case  4: return 67;
      case  5: return 67;
      case  6: return 66;
      case  7: return 66;
      case  8: return 65;
      case  9: return 65;
      case 10: return 64;
      case 11: return 64;
      case 12: return 63;
      case 13: return 63;
      case 14: return 62;
      case 15: return 62;
      case 16: return 61;
      case 17: return 61;
      case 18: return 60;
      case 19: return 60;
      case 20: return 59;
      case 21: return 59;
      case 22: return 58;
      case 23: return 58;
      case 24: return 57;
      case 25: return 57;
      case 26: return 56;
      case 27: return 56;
      case 28: return 55;
      case 29: return 55;
      case 30: return 54;
      case 31: return 54;
      case 32: return 53;
      case 33: return 53;
      case 34: return 52;
      case 35: return 52;
      case 36: return 51;
      case 37: return 51;
      case 38: return 50;
      case 39: return 50;
      case 40: return 49;
      case 41: return 48;
      case 42: return 47;
      case 43: return 46;
      case 44: return 45;
      case 45: return 44;
      case 46: return 43;
      case 47: return 41;
      case 48: return 39;
      case 49: return 37;
      case 50: return 35;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for Knight petrification saving throw.");
	break;
      }
    case SAVING_BREATH:	/* Breath weapons */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 69;
      case  3: return 68;
      case  4: return 67;
      case  5: return 67;
      case  6: return 66;
      case  7: return 66;
      case  8: return 65;
      case  9: return 65;
      case 10: return 64;
      case 11: return 64;
      case 12: return 63;
      case 13: return 63;
      case 14: return 62;
      case 15: return 62;
      case 16: return 61;
      case 17: return 61;
      case 18: return 60;
      case 19: return 60;
      case 20: return 59;
      case 21: return 59;
      case 22: return 58;
      case 23: return 58;
      case 24: return 57;
      case 25: return 57;
      case 26: return 56;
      case 27: return 56;
      case 28: return 55;
      case 29: return 55;
      case 30: return 54;
      case 31: return 54;
      case 32: return 53;
      case 33: return 53;
      case 34: return 52;
      case 35: return 52;
      case 36: return 51;
      case 37: return 51;
      case 38: return 50;
      case 39: return 50;
      case 40: return 49;
      case 41: return 48;
      case 42: return 47;
      case 43: return 46;
      case 44: return 45;
      case 45: return 44;
      case 46: return 43;
      case 47: return 41;
      case 48: return 39;
      case 49: return 37;
      case 50: return 35;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for Knight breath saving throw.");
	break;
      }
    case SAVING_SPELL:	/* Generic spells */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 69;
      case  3: return 68;
      case  4: return 67;
      case  5: return 67;
      case  6: return 66;
      case  7: return 66;
      case  8: return 65;
      case  9: return 65;
      case 10: return 64;
      case 11: return 64;
      case 12: return 63;
      case 13: return 63;
      case 14: return 62;
      case 15: return 62;
      case 16: return 61;
      case 17: return 61;
      case 18: return 60;
      case 19: return 60;
      case 20: return 59;
      case 21: return 59;
      case 22: return 58;
      case 23: return 58;
      case 24: return 57;
      case 25: return 57;
      case 26: return 56;
      case 27: return 56;
      case 28: return 55;
      case 29: return 55;
      case 30: return 54;
      case 31: return 54;
      case 32: return 53;
      case 33: return 53;
      case 34: return 52;
      case 35: return 52;
      case 36: return 51;
      case 37: return 51;
      case 38: return 50;
      case 39: return 50;
      case 40: return 49;
      case 41: return 48;
      case 42: return 47;
      case 43: return 46;
      case 44: return 45;
      case 45: return 44;
      case 46: return 43;
      case 47: return 41;
      case 48: return 39;
      case 49: return 37;
      case 50: return 35;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for Knight spell saving throw.");
	break;
      }
    default:
      log("SYSERR: Invalid saving throw type.");
      break;
    }

  case CLASS_CLERIC:
    switch (type) {
    case SAVING_PARA:	/* Paralyzation */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 68;
      case  3: return 67;
      case  4: return 66;
      case  5: return 65;
      case  6: return 64;
      case  7: return 63;
      case  8: return 62;
      case  9: return 61;
      case 10: return 60;
      case 11: return 59;
      case 12: return 58;
      case 13: return 58;
      case 14: return 57;
      case 15: return 57;
      case 16: return 56;
      case 17: return 56;
      case 18: return 55;
      case 19: return 55;
      case 20: return 54;
      case 21: return 54;
      case 22: return 53;
      case 23: return 53;
      case 24: return 52;
      case 25: return 52;
      case 26: return 51;
      case 27: return 51;
      case 28: return 50;
      case 29: return 50;
      case 30: return 49;
      case 31: return 49;
      case 32: return 48;
      case 33: return 48;
      case 34: return 47;
      case 35: return 47;
      case 36: return 46;
      case 37: return 46;
      case 38: return 45;
      case 39: return 45;
      case 40: return 44;
      case 41: return 43;
      case 42: return 42;
      case 43: return 41;
      case 44: return 40;
      case 45: return 39;
      case 46: return 38;
      case 47: return 36;
      case 48: return 34;
      case 49: return 32;
      case 50: return 30;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for Cleric paralyzation saving throw.");
	break;
      }
    case SAVING_ROD:	/* Rods */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 68;
      case  3: return 67;
      case  4: return 66;
      case  5: return 65;
      case  6: return 64;
      case  7: return 63;
      case  8: return 62;
      case  9: return 61;
      case 10: return 60;
      case 11: return 59;
      case 12: return 58;
      case 13: return 58;
      case 14: return 57;
      case 15: return 57;
      case 16: return 56;
      case 17: return 56;
      case 18: return 55;
      case 19: return 55;
      case 20: return 54;
      case 21: return 54;
      case 22: return 53;
      case 23: return 53;
      case 24: return 52;
      case 25: return 52;
      case 26: return 51;
      case 27: return 51;
      case 28: return 50;
      case 29: return 50;
      case 30: return 49;
      case 31: return 49;
      case 32: return 48;
      case 33: return 48;
      case 34: return 47;
      case 35: return 47;
      case 36: return 46;
      case 37: return 46;
      case 38: return 45;
      case 39: return 45;
      case 40: return 44;
      case 41: return 43;
      case 42: return 42;
      case 43: return 41;
      case 44: return 40;
      case 45: return 39;
      case 46: return 38;
      case 47: return 36;
      case 48: return 34;
      case 49: return 32;
      case 50: return 30;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for Cleric rod saving throw.");
	break;
      }
    case SAVING_PETRI:	/* Petrification */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 68;
      case  3: return 67;
      case  4: return 66;
      case  5: return 65;
      case  6: return 64;
      case  7: return 63;
      case  8: return 62;
      case  9: return 61;
      case 10: return 60;
      case 11: return 59;
      case 12: return 58;
      case 13: return 58;
      case 14: return 57;
      case 15: return 57;
      case 16: return 56;
      case 17: return 56;
      case 18: return 55;
      case 19: return 55;
      case 20: return 54;
      case 21: return 54;
      case 22: return 53;
      case 23: return 53;
      case 24: return 52;
      case 25: return 52;
      case 26: return 51;
      case 27: return 51;
      case 28: return 50;
      case 29: return 50;
      case 30: return 49;
      case 31: return 49;
      case 32: return 48;
      case 33: return 48;
      case 34: return 47;
      case 35: return 47;
      case 36: return 46;
      case 37: return 46;
      case 38: return 45;
      case 39: return 45;
      case 40: return 44;
      case 41: return 43;
      case 42: return 42;
      case 43: return 41;
      case 44: return 40;
      case 45: return 39;
      case 46: return 38;
      case 47: return 36;
      case 48: return 34;
      case 49: return 32;
      case 50: return 30;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for Cleric petrification saving throw.");
	break;
      }
    case SAVING_BREATH:	/* Breath weapons */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 68;
      case  3: return 67;
      case  4: return 66;
      case  5: return 65;
      case  6: return 64;
      case  7: return 63;
      case  8: return 62;
      case  9: return 61;
      case 10: return 60;
      case 11: return 59;
      case 12: return 58;
      case 13: return 58;
      case 14: return 57;
      case 15: return 57;
      case 16: return 56;
      case 17: return 56;
      case 18: return 55;
      case 19: return 55;
      case 20: return 54;
      case 21: return 54;
      case 22: return 53;
      case 23: return 53;
      case 24: return 52;
      case 25: return 52;
      case 26: return 51;
      case 27: return 51;
      case 28: return 50;
      case 29: return 50;
      case 30: return 49;
      case 31: return 49;
      case 32: return 48;
      case 33: return 48;
      case 34: return 47;
      case 35: return 47;
      case 36: return 46;
      case 37: return 46;
      case 38: return 45;
      case 39: return 45;
      case 40: return 44;
      case 41: return 43;
      case 42: return 42;
      case 43: return 41;
      case 44: return 40;
      case 45: return 39;
      case 46: return 38;
      case 47: return 36;
      case 48: return 34;
      case 49: return 32;
      case 50: return 30;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for Cleric breath saving throw.");
	break;
      }
    case SAVING_SPELL:	/* Generic spells */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 68;
      case  3: return 67;
      case  4: return 66;
      case  5: return 65;
      case  6: return 64;
      case  7: return 63;
      case  8: return 62;
      case  9: return 61;
      case 10: return 60;
      case 11: return 59;
      case 12: return 58;
      case 13: return 58;
      case 14: return 57;
      case 15: return 57;
      case 16: return 56;
      case 17: return 56;
      case 18: return 55;
      case 19: return 55;
      case 20: return 54;
      case 21: return 54;
      case 22: return 53;
      case 23: return 53;
      case 24: return 52;
      case 25: return 52;
      case 26: return 51;
      case 27: return 51;
      case 28: return 50;
      case 29: return 50;
      case 30: return 49;
      case 31: return 49;
      case 32: return 48;
      case 33: return 48;
      case 34: return 47;
      case 35: return 47;
      case 36: return 46;
      case 37: return 46;
      case 38: return 45;
      case 39: return 45;
      case 40: return 44;
      case 41: return 43;
      case 42: return 42;
      case 43: return 41;
      case 44: return 40;
      case 45: return 39;
      case 46: return 38;
      case 47: return 36;
      case 48: return 34;
      case 49: return 32;
      case 50: return 30;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for Cleric spell saving throw.");
	break;
      }
    default:
      log("SYSERR: Invalid saving throw type.");
      break;
    }

  case CLASS_BARBARIAN:
    switch (type) {
    case SAVING_PARA:	/* Paralyzation */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 69;
      case  3: return 68;
      case  4: return 68;
      case  5: return 67;
      case  6: return 67;
      case  7: return 66;
      case  8: return 66;
      case  9: return 65;
      case 10: return 65;
      case 11: return 64;
      case 12: return 64;
      case 13: return 63;
      case 14: return 63;
      case 15: return 62;
      case 16: return 62;
      case 17: return 61;
      case 18: return 61;
      case 19: return 60;
      case 20: return 60;
      case 21: return 59;
      case 22: return 59;
      case 23: return 58;
      case 24: return 58;
      case 25: return 57;
      case 26: return 57;
      case 27: return 56;
      case 28: return 56;
      case 29: return 55;
      case 30: return 55;
      case 31: return 54;
      case 32: return 54;
      case 33: return 53;
      case 34: return 53;
      case 35: return 52;
      case 36: return 52;
      case 37: return 51;
      case 38: return 51;
      case 39: return 50;
      case 40: return 50;
      case 41: return 49;
      case 42: return 48;
      case 43: return 47;
      case 44: return 46;
      case 45: return 45;
      case 46: return 44;
      case 47: return 43;
      case 48: return 42;
      case 49: return 41;
      case 50: return 40;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for Barbarian paralyzation saving throw.");
	break;
      }
    case SAVING_ROD:	/* Rods */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 69;
      case  3: return 68;
      case  4: return 68;
      case  5: return 67;
      case  6: return 67;
      case  7: return 66;
      case  8: return 66;
      case  9: return 65;
      case 10: return 65;
      case 11: return 64;
      case 12: return 64;
      case 13: return 63;
      case 14: return 63;
      case 15: return 62;
      case 16: return 62;
      case 17: return 61;
      case 18: return 61;
      case 19: return 60;
      case 20: return 60;
      case 21: return 59;
      case 22: return 59;
      case 23: return 58;
      case 24: return 58;
      case 25: return 57;
      case 26: return 57;
      case 27: return 56;
      case 28: return 56;
      case 29: return 55;
      case 30: return 55;
      case 31: return 54;
      case 32: return 54;
      case 33: return 53;
      case 34: return 53;
      case 35: return 52;
      case 36: return 52;
      case 37: return 51;
      case 38: return 51;
      case 39: return 50;
      case 40: return 50;
      case 41: return 49;
      case 42: return 48;
      case 43: return 47;
      case 44: return 46;
      case 45: return 45;
      case 46: return 44;
      case 47: return 43;
      case 48: return 42;
      case 49: return 41;
      case 50: return 40;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for Barbarian rod saving throw.");
	break;
      }
    case SAVING_PETRI:	/* Petrification */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 69;
      case  3: return 68;
      case  4: return 68;
      case  5: return 67;
      case  6: return 67;
      case  7: return 66;
      case  8: return 66;
      case  9: return 65;
      case 10: return 65;
      case 11: return 64;
      case 12: return 64;
      case 13: return 63;
      case 14: return 63;
      case 15: return 62;
      case 16: return 62;
      case 17: return 61;
      case 18: return 61;
      case 19: return 60;
      case 20: return 60;
      case 21: return 59;
      case 22: return 59;
      case 23: return 58;
      case 24: return 58;
      case 25: return 57;
      case 26: return 57;
      case 27: return 56;
      case 28: return 56;
      case 29: return 55;
      case 30: return 55;
      case 31: return 54;
      case 32: return 54;
      case 33: return 53;
      case 34: return 53;
      case 35: return 52;
      case 36: return 52;
      case 37: return 51;
      case 38: return 51;
      case 39: return 50;
      case 40: return 50;
      case 41: return 49;
      case 42: return 48;
      case 43: return 47;
      case 44: return 46;
      case 45: return 45;
      case 46: return 44;
      case 47: return 43;
      case 48: return 42;
      case 49: return 41;
      case 50: return 40;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for Barbarian petrification saving throw.");
	break;
      }
    case SAVING_BREATH:	/* Breath weapons */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 69;
      case  3: return 68;
      case  4: return 68;
      case  5: return 67;
      case  6: return 67;
      case  7: return 66;
      case  8: return 66;
      case  9: return 65;
      case 10: return 65;
      case 11: return 64;
      case 12: return 64;
      case 13: return 63;
      case 14: return 63;
      case 15: return 62;
      case 16: return 62;
      case 17: return 61;
      case 18: return 61;
      case 19: return 60;
      case 20: return 60;
      case 21: return 59;
      case 22: return 59;
      case 23: return 58;
      case 24: return 58;
      case 25: return 57;
      case 26: return 57;
      case 27: return 56;
      case 28: return 56;
      case 29: return 55;
      case 30: return 55;
      case 31: return 54;
      case 32: return 54;
      case 33: return 53;
      case 34: return 53;
      case 35: return 52;
      case 36: return 52;
      case 37: return 51;
      case 38: return 51;
      case 39: return 50;
      case 40: return 50;
      case 41: return 49;
      case 42: return 48;
      case 43: return 47;
      case 44: return 46;
      case 45: return 45;
      case 46: return 44;
      case 47: return 43;
      case 48: return 42;
      case 49: return 41;
      case 50: return 40;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for Barbarian breath saving throw.");
	break;
      }
    case SAVING_SPELL:	/* Generic spells */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 69;
      case  3: return 68;
      case  4: return 68;
      case  5: return 67;
      case  6: return 67;
      case  7: return 66;
      case  8: return 66;
      case  9: return 65;
      case 10: return 65;
      case 11: return 64;
      case 12: return 64;
      case 13: return 63;
      case 14: return 63;
      case 15: return 62;
      case 16: return 62;
      case 17: return 61;
      case 18: return 61;
      case 19: return 60;
      case 20: return 60;
      case 21: return 59;
      case 22: return 59;
      case 23: return 58;
      case 24: return 58;
      case 25: return 57;
      case 26: return 57;
      case 27: return 56;
      case 28: return 56;
      case 29: return 55;
      case 30: return 55;
      case 31: return 54;
      case 32: return 54;
      case 33: return 53;
      case 34: return 53;
      case 35: return 52;
      case 36: return 52;
      case 37: return 51;
      case 38: return 51;
      case 39: return 50;
      case 40: return 50;
      case 41: return 49;
      case 42: return 48;
      case 43: return 47;
      case 44: return 46;
      case 45: return 45;
      case 46: return 44;
      case 47: return 43;
      case 48: return 42;
      case 49: return 41;
      case 50: return 40;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for Barbarian spell saving throw.");
	break;
      }
    default:
      log("SYSERR: Invalid saving throw type.");
      break;
    }

  case CLASS_RANGER:
    switch (type) {
    case SAVING_PARA:	/* Paralyzation */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 69;
      case  3: return 69;
      case  4: return 69;
      case  5: return 68;
      case  6: return 68;
      case  7: return 67;
      case  8: return 67;
      case  9: return 66;
      case 10: return 66;
      case 11: return 65;
      case 12: return 65;
      case 13: return 64;
      case 14: return 64;
      case 15: return 63;
      case 16: return 63;
      case 17: return 62;
      case 18: return 62;
      case 19: return 61;
      case 20: return 61;
      case 21: return 60;
      case 22: return 60;
      case 23: return 59;
      case 24: return 59;
      case 25: return 58;
      case 26: return 58;
      case 27: return 57;
      case 28: return 57;
      case 29: return 56;
      case 30: return 56;
      case 31: return 55;
      case 32: return 55;
      case 33: return 54;
      case 34: return 54;
      case 35: return 53;
      case 36: return 53;
      case 37: return 52;
      case 38: return 52;
      case 39: return 51;
      case 40: return 51;
      case 41: return 50;
      case 42: return 49;
      case 43: return 48;
      case 44: return 47;
      case 45: return 46;
      case 46: return 45;
      case 47: return 43;
      case 48: return 41;
      case 49: return 39;
      case 50: return 37;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for Ranger paralyzation saving throw.");
	break;
      }
    case SAVING_ROD:	/* Rods */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 69;
      case  3: return 69;
      case  4: return 69;
      case  5: return 68;
      case  6: return 68;
      case  7: return 67;
      case  8: return 67;
      case  9: return 66;
      case 10: return 66;
      case 11: return 65;
      case 12: return 65;
      case 13: return 64;
      case 14: return 64;
      case 15: return 63;
      case 16: return 63;
      case 17: return 62;
      case 18: return 62;
      case 19: return 61;
      case 20: return 61;
      case 21: return 60;
      case 22: return 60;
      case 23: return 59;
      case 24: return 59;
      case 25: return 58;
      case 26: return 58;
      case 27: return 57;
      case 28: return 57;
      case 29: return 56;
      case 30: return 56;
      case 31: return 55;
      case 32: return 55;
      case 33: return 54;
      case 34: return 54;
      case 35: return 53;
      case 36: return 53;
      case 37: return 52;
      case 38: return 52;
      case 39: return 51;
      case 40: return 51;
      case 41: return 50;
      case 42: return 49;
      case 43: return 48;
      case 44: return 47;
      case 45: return 46;
      case 46: return 45;
      case 47: return 43;
      case 48: return 41;
      case 49: return 39;
      case 50: return 37;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for Ranger rod saving throw.");
	break;
      }
    case SAVING_PETRI:	/* Petrification */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 69;
      case  3: return 69;
      case  4: return 69;
      case  5: return 68;
      case  6: return 68;
      case  7: return 67;
      case  8: return 67;
      case  9: return 66;
      case 10: return 66;
      case 11: return 65;
      case 12: return 65;
      case 13: return 64;
      case 14: return 64;
      case 15: return 63;
      case 16: return 63;
      case 17: return 62;
      case 18: return 62;
      case 19: return 61;
      case 20: return 61;
      case 21: return 60;
      case 22: return 60;
      case 23: return 59;
      case 24: return 59;
      case 25: return 58;
      case 26: return 58;
      case 27: return 57;
      case 28: return 57;
      case 29: return 56;
      case 30: return 56;
      case 31: return 55;
      case 32: return 55;
      case 33: return 54;
      case 34: return 54;
      case 35: return 53;
      case 36: return 53;
      case 37: return 52;
      case 38: return 52;
      case 39: return 51;
      case 40: return 51;
      case 41: return 50;
      case 42: return 49;
      case 43: return 48;
      case 44: return 47;
      case 45: return 46;
      case 46: return 45;
      case 47: return 43;
      case 48: return 41;
      case 49: return 39;
      case 50: return 37;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for Ranger petrification saving throw.");
	break;
      }
    case SAVING_BREATH:	/* Breath weapons */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 69;
      case  3: return 69;
      case  4: return 69;
      case  5: return 68;
      case  6: return 68;
      case  7: return 67;
      case  8: return 67;
      case  9: return 66;
      case 10: return 66;
      case 11: return 65;
      case 12: return 65;
      case 13: return 64;
      case 14: return 64;
      case 15: return 63;
      case 16: return 63;
      case 17: return 62;
      case 18: return 62;
      case 19: return 61;
      case 20: return 61;
      case 21: return 60;
      case 22: return 60;
      case 23: return 59;
      case 24: return 59;
      case 25: return 58;
      case 26: return 58;
      case 27: return 57;
      case 28: return 57;
      case 29: return 56;
      case 30: return 56;
      case 31: return 55;
      case 32: return 55;
      case 33: return 54;
      case 34: return 54;
      case 35: return 53;
      case 36: return 53;
      case 37: return 52;
      case 38: return 52;
      case 39: return 51;
      case 40: return 51;
      case 41: return 50;
      case 42: return 49;
      case 43: return 48;
      case 44: return 47;
      case 45: return 46;
      case 46: return 45;
      case 47: return 43;
      case 48: return 41;
      case 49: return 39;
      case 50: return 37;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for Ranger breath saving throw.");
	break;
      }
    case SAVING_SPELL:	/* Generic spells */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 69;
      case  3: return 69;
      case  4: return 69;
      case  5: return 68;
      case  6: return 68;
      case  7: return 67;
      case  8: return 67;
      case  9: return 66;
      case 10: return 66;
      case 11: return 65;
      case 12: return 65;
      case 13: return 64;
      case 14: return 64;
      case 15: return 63;
      case 16: return 63;
      case 17: return 62;
      case 18: return 62;
      case 19: return 61;
      case 20: return 61;
      case 21: return 60;
      case 22: return 60;
      case 23: return 59;
      case 24: return 59;
      case 25: return 58;
      case 26: return 58;
      case 27: return 57;
      case 28: return 57;
      case 29: return 56;
      case 30: return 56;
      case 31: return 55;
      case 32: return 55;
      case 33: return 54;
      case 34: return 54;
      case 35: return 53;
      case 36: return 53;
      case 37: return 52;
      case 38: return 52;
      case 39: return 51;
      case 40: return 51;
      case 41: return 50;
      case 42: return 49;
      case 43: return 48;
      case 44: return 47;
      case 45: return 46;
      case 46: return 45;
      case 47: return 43;
      case 48: return 41;
      case 49: return 39;
      case 50: return 37;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for Ranger spell saving throw.");
	break;
      }
    default:
      log("SYSERR: Invalid saving throw type.");
      break;
    }

  case CLASS_MAGE:
    switch (type) {
    case SAVING_PARA:	/* Paralyzation */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 68;
      case  3: return 66;
      case  4: return 65;
      case  5: return 64;
      case  6: return 63;
      case  7: return 62;
      case  8: return 61;
      case  9: return 60;
      case 10: return 59;
      case 11: return 58;
      case 12: return 57;
      case 13: return 57;
      case 14: return 56;
      case 15: return 56;
      case 16: return 55;
      case 17: return 55;
      case 18: return 54;
      case 19: return 54;
      case 20: return 53;
      case 21: return 53;
      case 22: return 52;
      case 23: return 52;
      case 24: return 51;
      case 25: return 51;
      case 26: return 50;
      case 27: return 50;
      case 28: return 49;
      case 29: return 49;
      case 30: return 48;
      case 31: return 48;
      case 32: return 47;
      case 33: return 47;
      case 34: return 46;
      case 35: return 45;
      case 36: return 44;
      case 37: return 43;
      case 38: return 42;
      case 39: return 41;
      case 40: return 40;
      case 41: return 39;
      case 42: return 38;
      case 43: return 37;
      case 44: return 36;
      case 45: return 35;
      case 46: return 34;
      case 47: return 32;
      case 48: return 30;
      case 49: return 28;
      case 50: return 26;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for mage paralyzation saving throw.");
	break;
      }
    case SAVING_ROD:	/* Rods */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 68;
      case  3: return 66;
      case  4: return 65;
      case  5: return 64;
      case  6: return 63;
      case  7: return 62;
      case  8: return 61;
      case  9: return 60;
      case 10: return 59;
      case 11: return 58;
      case 12: return 57;
      case 13: return 57;
      case 14: return 56;
      case 15: return 56;
      case 16: return 55;
      case 17: return 55;
      case 18: return 54;
      case 19: return 54;
      case 20: return 53;
      case 21: return 53;
      case 22: return 52;
      case 23: return 52;
      case 24: return 51;
      case 25: return 51;
      case 26: return 50;
      case 27: return 50;
      case 28: return 49;
      case 29: return 49;
      case 30: return 48;
      case 31: return 48;
      case 32: return 47;
      case 33: return 47;
      case 34: return 46;
      case 35: return 45;
      case 36: return 44;
      case 37: return 43;
      case 38: return 42;
      case 39: return 41;
      case 40: return 40;
      case 41: return 39;
      case 42: return 38;
      case 43: return 37;
      case 44: return 36;
      case 45: return 35;
      case 46: return 34;
      case 47: return 32;
      case 48: return 30;
      case 49: return 28;
      case 50: return 26;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for mage rod saving throw.");
	break;
      }
    case SAVING_PETRI:	/* Petrification */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 68;
      case  3: return 66;
      case  4: return 65;
      case  5: return 64;
      case  6: return 63;
      case  7: return 62;
      case  8: return 61;
      case  9: return 60;
      case 10: return 59;
      case 11: return 58;
      case 12: return 57;
      case 13: return 57;
      case 14: return 56;
      case 15: return 56;
      case 16: return 55;
      case 17: return 55;
      case 18: return 54;
      case 19: return 54;
      case 20: return 53;
      case 21: return 53;
      case 22: return 52;
      case 23: return 52;
      case 24: return 51;
      case 25: return 51;
      case 26: return 50;
      case 27: return 50;
      case 28: return 49;
      case 29: return 49;
      case 30: return 48;
      case 31: return 48;
      case 32: return 47;
      case 33: return 47;
      case 34: return 46;
      case 35: return 45;
      case 36: return 44;
      case 37: return 43;
      case 38: return 42;
      case 39: return 41;
      case 40: return 40;
      case 41: return 39;
      case 42: return 38;
      case 43: return 37;
      case 44: return 36;
      case 45: return 35;
      case 46: return 34;
      case 47: return 32;
      case 48: return 30;
      case 49: return 28;
      case 50: return 26;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for mage petrification saving throw.");
	break;
      }
    case SAVING_BREATH:	/* Breath weapons */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 68;
      case  3: return 66;
      case  4: return 65;
      case  5: return 64;
      case  6: return 63;
      case  7: return 62;
      case  8: return 61;
      case  9: return 60;
      case 10: return 59;
      case 11: return 58;
      case 12: return 57;
      case 13: return 57;
      case 14: return 56;
      case 15: return 56;
      case 16: return 55;
      case 17: return 55;
      case 18: return 54;
      case 19: return 54;
      case 20: return 53;
      case 21: return 53;
      case 22: return 52;
      case 23: return 52;
      case 24: return 51;
      case 25: return 51;
      case 26: return 50;
      case 27: return 50;
      case 28: return 49;
      case 29: return 49;
      case 30: return 48;
      case 31: return 48;
      case 32: return 47;
      case 33: return 47;
      case 34: return 46;
      case 35: return 45;
      case 36: return 44;
      case 37: return 43;
      case 38: return 42;
      case 39: return 41;
      case 40: return 40;
      case 41: return 39;
      case 42: return 38;
      case 43: return 37;
      case 44: return 36;
      case 45: return 35;
      case 46: return 34;
      case 47: return 32;
      case 48: return 30;
      case 49: return 28;
      case 50: return 26;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for mage breath saving throw.");
	break;
      }
    case SAVING_SPELL:	/* Generic spells */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 68;
      case  3: return 66;
      case  4: return 65;
      case  5: return 64;
      case  6: return 63;
      case  7: return 62;
      case  8: return 61;
      case  9: return 60;
      case 10: return 59;
      case 11: return 58;
      case 12: return 57;
      case 13: return 57;
      case 14: return 56;
      case 15: return 56;
      case 16: return 55;
      case 17: return 55;
      case 18: return 54;
      case 19: return 54;
      case 20: return 53;
      case 21: return 53;
      case 22: return 52;
      case 23: return 52;
      case 24: return 51;
      case 25: return 51;
      case 26: return 50;
      case 27: return 50;
      case 28: return 49;
      case 29: return 49;
      case 30: return 48;
      case 31: return 48;
      case 32: return 47;
      case 33: return 47;
      case 34: return 46;
      case 35: return 45;
      case 36: return 44;
      case 37: return 43;
      case 38: return 42;
      case 39: return 41;
      case 40: return 40;
      case 41: return 39;
      case 42: return 38;
      case 43: return 37;
      case 44: return 36;
      case 45: return 35;
      case 46: return 34;
      case 47: return 32;
      case 48: return 30;
      case 49: return 28;
      case 50: return 26;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for mage spell saving throw.");
	break;
      }
    default:
      log("SYSERR: Invalid saving throw type.");
      break;
    }

  case CLASS_MONK:
    switch (type) {
    case SAVING_PARA:	/* Paralyzation */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 69;
      case  3: return 69;
      case  4: return 69;
      case  5: return 68;
      case  6: return 68;
      case  7: return 67;
      case  8: return 67;
      case  9: return 66;
      case 10: return 66;
      case 11: return 65;
      case 12: return 65;
      case 13: return 64;
      case 14: return 64;
      case 15: return 63;
      case 16: return 63;
      case 17: return 62;
      case 18: return 62;
      case 19: return 61;
      case 20: return 61;
      case 21: return 60;
      case 22: return 60;
      case 23: return 59;
      case 24: return 59;
      case 25: return 58;
      case 26: return 58;
      case 27: return 57;
      case 28: return 57;
      case 29: return 56;
      case 30: return 56;
      case 31: return 55;
      case 32: return 55;
      case 33: return 54;
      case 34: return 54;
      case 35: return 53;
      case 36: return 53;
      case 37: return 52;
      case 38: return 52;
      case 39: return 51;
      case 40: return 51;
      case 41: return 50;
      case 42: return 49;
      case 43: return 48;
      case 44: return 47;
      case 45: return 46;
      case 46: return 45;
      case 47: return 43;
      case 48: return 41;
      case 49: return 39;
      case 50: return 37;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for Monk paralyzation saving throw.");
	break;
      }
    case SAVING_ROD:	/* Rods */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 69;
      case  3: return 69;
      case  4: return 69;
      case  5: return 68;
      case  6: return 68;
      case  7: return 67;
      case  8: return 67;
      case  9: return 66;
      case 10: return 66;
      case 11: return 65;
      case 12: return 65;
      case 13: return 64;
      case 14: return 64;
      case 15: return 63;
      case 16: return 63;
      case 17: return 62;
      case 18: return 62;
      case 19: return 61;
      case 20: return 61;
      case 21: return 60;
      case 22: return 60;
      case 23: return 59;
      case 24: return 59;
      case 25: return 58;
      case 26: return 58;
      case 27: return 57;
      case 28: return 57;
      case 29: return 56;
      case 30: return 56;
      case 31: return 55;
      case 32: return 55;
      case 33: return 54;
      case 34: return 54;
      case 35: return 53;
      case 36: return 53;
      case 37: return 52;
      case 38: return 52;
      case 39: return 51;
      case 40: return 51;
      case 41: return 50;
      case 42: return 49;
      case 43: return 48;
      case 44: return 47;
      case 45: return 46;
      case 46: return 45;
      case 47: return 43;
      case 48: return 41;
      case 49: return 39;
      case 50: return 37;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for Monk rod saving throw.");
	break;
      }
    case SAVING_PETRI:	/* Petrification */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 69;
      case  3: return 69;
      case  4: return 69;
      case  5: return 68;
      case  6: return 68;
      case  7: return 67;
      case  8: return 67;
      case  9: return 66;
      case 10: return 66;
      case 11: return 65;
      case 12: return 65;
      case 13: return 64;
      case 14: return 64;
      case 15: return 63;
      case 16: return 63;
      case 17: return 62;
      case 18: return 62;
      case 19: return 61;
      case 20: return 61;
      case 21: return 60;
      case 22: return 60;
      case 23: return 59;
      case 24: return 59;
      case 25: return 58;
      case 26: return 58;
      case 27: return 57;
      case 28: return 57;
      case 29: return 56;
      case 30: return 56;
      case 31: return 55;
      case 32: return 55;
      case 33: return 54;
      case 34: return 54;
      case 35: return 53;
      case 36: return 53;
      case 37: return 52;
      case 38: return 52;
      case 39: return 51;
      case 40: return 51;
      case 41: return 50;
      case 42: return 49;
      case 43: return 48;
      case 44: return 47;
      case 45: return 46;
      case 46: return 45;
      case 47: return 43;
      case 48: return 41;
      case 49: return 39;
      case 50: return 37;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for Monk petrification saving throw.");
	break;
      }
    case SAVING_BREATH:	/* Breath weapons */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 69;
      case  3: return 69;
      case  4: return 69;
      case  5: return 68;
      case  6: return 68;
      case  7: return 67;
      case  8: return 67;
      case  9: return 66;
      case 10: return 66;
      case 11: return 65;
      case 12: return 65;
      case 13: return 64;
      case 14: return 64;
      case 15: return 63;
      case 16: return 63;
      case 17: return 62;
      case 18: return 62;
      case 19: return 61;
      case 20: return 61;
      case 21: return 60;
      case 22: return 60;
      case 23: return 59;
      case 24: return 59;
      case 25: return 58;
      case 26: return 58;
      case 27: return 57;
      case 28: return 57;
      case 29: return 56;
      case 30: return 56;
      case 31: return 55;
      case 32: return 55;
      case 33: return 54;
      case 34: return 54;
      case 35: return 53;
      case 36: return 53;
      case 37: return 52;
      case 38: return 52;
      case 39: return 51;
      case 40: return 51;
      case 41: return 50;
      case 42: return 49;
      case 43: return 48;
      case 44: return 47;
      case 45: return 46;
      case 46: return 45;
      case 47: return 43;
      case 48: return 41;
      case 49: return 39;
      case 50: return 37;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for Monk breath saving throw.");
	break;
      }
    case SAVING_SPELL:	/* Generic spells */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 69;
      case  3: return 69;
      case  4: return 69;
      case  5: return 68;
      case  6: return 68;
      case  7: return 67;
      case  8: return 67;
      case  9: return 66;
      case 10: return 66;
      case 11: return 65;
      case 12: return 65;
      case 13: return 64;
      case 14: return 64;
      case 15: return 63;
      case 16: return 63;
      case 17: return 62;
      case 18: return 62;
      case 19: return 61;
      case 20: return 61;
      case 21: return 60;
      case 22: return 60;
      case 23: return 59;
      case 24: return 59;
      case 25: return 58;
      case 26: return 58;
      case 27: return 57;
      case 28: return 57;
      case 29: return 56;
      case 30: return 56;
      case 31: return 55;
      case 32: return 55;
      case 33: return 54;
      case 34: return 54;
      case 35: return 53;
      case 36: return 53;
      case 37: return 52;
      case 38: return 52;
      case 39: return 51;
      case 40: return 51;
      case 41: return 50;
      case 42: return 49;
      case 43: return 48;
      case 44: return 47;
      case 45: return 46;
      case 46: return 45;
      case 47: return 43;
      case 48: return 41;
      case 49: return 39;
      case 50: return 37;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for Monk spell saving throw.");
	break;
      }
    default:
      log("SYSERR: Invalid saving throw type.");
      break;
    }

  case CLASS_NECROMANCER:
    switch (type) {
    case SAVING_PARA:	/* Paralyzation */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 68;
      case  3: return 66;
      case  4: return 65;
      case  5: return 64;
      case  6: return 63;
      case  7: return 62;
      case  8: return 61;
      case  9: return 60;
      case 10: return 59;
      case 11: return 58;
      case 12: return 57;
      case 13: return 57;
      case 14: return 56;
      case 15: return 56;
      case 16: return 55;
      case 17: return 55;
      case 18: return 54;
      case 19: return 54;
      case 20: return 53;
      case 21: return 53;
      case 22: return 52;
      case 23: return 52;
      case 24: return 51;
      case 25: return 51;
      case 26: return 50;
      case 27: return 50;
      case 28: return 49;
      case 29: return 49;
      case 30: return 48;
      case 31: return 48;
      case 32: return 47;
      case 33: return 47;
      case 34: return 46;
      case 35: return 45;
      case 36: return 44;
      case 37: return 43;
      case 38: return 42;
      case 39: return 41;
      case 40: return 40;
      case 41: return 39;
      case 42: return 38;
      case 43: return 37;
      case 44: return 36;
      case 45: return 35;
      case 46: return 34;
      case 47: return 32;
      case 48: return 30;
      case 49: return 28;
      case 50: return 26;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for Necromancer paralyzation saving throw.");
	break;
      }
    case SAVING_ROD:	/* Rods */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 68;
      case  3: return 66;
      case  4: return 65;
      case  5: return 64;
      case  6: return 63;
      case  7: return 62;
      case  8: return 61;
      case  9: return 60;
      case 10: return 59;
      case 11: return 58;
      case 12: return 57;
      case 13: return 57;
      case 14: return 56;
      case 15: return 56;
      case 16: return 55;
      case 17: return 55;
      case 18: return 54;
      case 19: return 54;
      case 20: return 53;
      case 21: return 53;
      case 22: return 52;
      case 23: return 52;
      case 24: return 51;
      case 25: return 51;
      case 26: return 50;
      case 27: return 50;
      case 28: return 49;
      case 29: return 49;
      case 30: return 48;
      case 31: return 48;
      case 32: return 47;
      case 33: return 47;
      case 34: return 46;
      case 35: return 45;
      case 36: return 44;
      case 37: return 43;
      case 38: return 42;
      case 39: return 41;
      case 40: return 40;
      case 41: return 39;
      case 42: return 38;
      case 43: return 37;
      case 44: return 36;
      case 45: return 35;
      case 46: return 34;
      case 47: return 32;
      case 48: return 30;
      case 49: return 28;
      case 50: return 26;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for Necromancer rod saving throw.");
	break;
      }
    case SAVING_PETRI:	/* Petrification */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 68;
      case  3: return 66;
      case  4: return 65;
      case  5: return 64;
      case  6: return 63;
      case  7: return 62;
      case  8: return 61;
      case  9: return 60;
      case 10: return 59;
      case 11: return 58;
      case 12: return 57;
      case 13: return 57;
      case 14: return 56;
      case 15: return 56;
      case 16: return 55;
      case 17: return 55;
      case 18: return 54;
      case 19: return 54;
      case 20: return 53;
      case 21: return 53;
      case 22: return 52;
      case 23: return 52;
      case 24: return 51;
      case 25: return 51;
      case 26: return 50;
      case 27: return 50;
      case 28: return 49;
      case 29: return 49;
      case 30: return 48;
      case 31: return 48;
      case 32: return 47;
      case 33: return 47;
      case 34: return 46;
      case 35: return 45;
      case 36: return 44;
      case 37: return 43;
      case 38: return 42;
      case 39: return 41;
      case 40: return 40;
      case 41: return 39;
      case 42: return 38;
      case 43: return 37;
      case 44: return 36;
      case 45: return 35;
      case 46: return 34;
      case 47: return 32;
      case 48: return 30;
      case 49: return 28;
      case 50: return 26;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for Necromancer Petri saving throw.");
	break;
      }
    case SAVING_BREATH:	/* Breath weapons */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 68;
      case  3: return 66;
      case  4: return 65;
      case  5: return 64;
      case  6: return 63;
      case  7: return 62;
      case  8: return 61;
      case  9: return 60;
      case 10: return 59;
      case 11: return 58;
      case 12: return 57;
      case 13: return 57;
      case 14: return 56;
      case 15: return 56;
      case 16: return 55;
      case 17: return 55;
      case 18: return 54;
      case 19: return 54;
      case 20: return 53;
      case 21: return 53;
      case 22: return 52;
      case 23: return 52;
      case 24: return 51;
      case 25: return 51;
      case 26: return 50;
      case 27: return 50;
      case 28: return 49;
      case 29: return 49;
      case 30: return 48;
      case 31: return 48;
      case 32: return 47;
      case 33: return 47;
      case 34: return 46;
      case 35: return 45;
      case 36: return 44;
      case 37: return 43;
      case 38: return 42;
      case 39: return 41;
      case 40: return 40;
      case 41: return 39;
      case 42: return 38;
      case 43: return 37;
      case 44: return 36;
      case 45: return 35;
      case 46: return 34;
      case 47: return 32;
      case 48: return 30;
      case 49: return 28;
      case 50: return 26;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for Necromancer breath saving throw.");
	break;
      }
    case SAVING_SPELL:	/* Generic spells */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 68;
      case  3: return 66;
      case  4: return 65;
      case  5: return 64;
      case  6: return 63;
      case  7: return 62;
      case  8: return 61;
      case  9: return 60;
      case 10: return 59;
      case 11: return 58;
      case 12: return 57;
      case 13: return 57;
      case 14: return 56;
      case 15: return 56;
      case 16: return 55;
      case 17: return 55;
      case 18: return 54;
      case 19: return 54;
      case 20: return 53;
      case 21: return 53;
      case 22: return 52;
      case 23: return 52;
      case 24: return 51;
      case 25: return 51;
      case 26: return 50;
      case 27: return 50;
      case 28: return 49;
      case 29: return 49;
      case 30: return 48;
      case 31: return 48;
      case 32: return 47;
      case 33: return 47;
      case 34: return 46;
      case 35: return 45;
      case 36: return 44;
      case 37: return 43;
      case 38: return 42;
      case 39: return 41;
      case 40: return 40;
      case 41: return 39;
      case 42: return 38;
      case 43: return 37;
      case 44: return 36;
      case 45: return 35;
      case 46: return 34;
      case 47: return 32;
      case 48: return 30;
      case 49: return 28;
      case 50: return 26;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for Necromancer spell saving throw.");
	break;
      }
    default:
      log("SYSERR: Invalid saving throw type.");
      break;
    }

  case CLASS_DRUID:
    switch (type) {
    case SAVING_PARA:	/* Paralyzation */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 68;
      case  3: return 67;
      case  4: return 66;
      case  5: return 65;
      case  6: return 64;
      case  7: return 63;
      case  8: return 62;
      case  9: return 61;
      case 10: return 60;
      case 11: return 59;
      case 12: return 58;
      case 13: return 58;
      case 14: return 57;
      case 15: return 57;
      case 16: return 56;
      case 17: return 56;
      case 18: return 55;
      case 19: return 55;
      case 20: return 54;
      case 21: return 54;
      case 22: return 53;
      case 23: return 53;
      case 24: return 52;
      case 25: return 52;
      case 26: return 51;
      case 27: return 51;
      case 28: return 50;
      case 29: return 50;
      case 30: return 49;
      case 31: return 49;
      case 32: return 48;
      case 33: return 48;
      case 34: return 47;
      case 35: return 47;
      case 36: return 46;
      case 37: return 46;
      case 38: return 45;
      case 39: return 45;
      case 40: return 44;
      case 41: return 43;
      case 42: return 42;
      case 43: return 41;
      case 44: return 40;
      case 45: return 39;
      case 46: return 38;
      case 47: return 36;
      case 48: return 34;
      case 49: return 32;
      case 50: return 30;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for Druid paralyzation saving throw.");
	break;
      }
    case SAVING_ROD:	/* Rods */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 68;
      case  3: return 67;
      case  4: return 66;
      case  5: return 65;
      case  6: return 64;
      case  7: return 63;
      case  8: return 62;
      case  9: return 61;
      case 10: return 60;
      case 11: return 59;
      case 12: return 58;
      case 13: return 58;
      case 14: return 57;
      case 15: return 57;
      case 16: return 56;
      case 17: return 56;
      case 18: return 55;
      case 19: return 55;
      case 20: return 54;
      case 21: return 54;
      case 22: return 53;
      case 23: return 53;
      case 24: return 52;
      case 25: return 52;
      case 26: return 51;
      case 27: return 51;
      case 28: return 50;
      case 29: return 50;
      case 30: return 49;
      case 31: return 49;
      case 32: return 48;
      case 33: return 48;
      case 34: return 47;
      case 35: return 47;
      case 36: return 46;
      case 37: return 46;
      case 38: return 45;
      case 39: return 45;
      case 40: return 44;
      case 41: return 43;
      case 42: return 42;
      case 43: return 41;
      case 44: return 40;
      case 45: return 39;
      case 46: return 38;
      case 47: return 36;
      case 48: return 34;
      case 49: return 32;
      case 50: return 30;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for Druid rod saving throw.");
	break;
      }
    case SAVING_PETRI:	/* Petrification */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 68;
      case  3: return 67;
      case  4: return 66;
      case  5: return 65;
      case  6: return 64;
      case  7: return 63;
      case  8: return 62;
      case  9: return 61;
      case 10: return 60;
      case 11: return 59;
      case 12: return 58;
      case 13: return 58;
      case 14: return 57;
      case 15: return 57;
      case 16: return 56;
      case 17: return 56;
      case 18: return 55;
      case 19: return 55;
      case 20: return 54;
      case 21: return 54;
      case 22: return 53;
      case 23: return 53;
      case 24: return 52;
      case 25: return 52;
      case 26: return 51;
      case 27: return 51;
      case 28: return 50;
      case 29: return 50;
      case 30: return 49;
      case 31: return 49;
      case 32: return 48;
      case 33: return 48;
      case 34: return 47;
      case 35: return 47;
      case 36: return 46;
      case 37: return 46;
      case 38: return 45;
      case 39: return 45;
      case 40: return 44;
      case 41: return 43;
      case 42: return 42;
      case 43: return 41;
      case 44: return 40;
      case 45: return 39;
      case 46: return 38;
      case 47: return 36;
      case 48: return 34;
      case 49: return 32;
      case 50: return 30;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for Druid petrification saving throw.");
	break;
      }
    case SAVING_BREATH:	/* Breath weapons */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 68;
      case  3: return 67;
      case  4: return 66;
      case  5: return 65;
      case  6: return 64;
      case  7: return 63;
      case  8: return 62;
      case  9: return 61;
      case 10: return 60;
      case 11: return 59;
      case 12: return 58;
      case 13: return 58;
      case 14: return 57;
      case 15: return 57;
      case 16: return 56;
      case 17: return 56;
      case 18: return 55;
      case 19: return 55;
      case 20: return 54;
      case 21: return 54;
      case 22: return 53;
      case 23: return 53;
      case 24: return 52;
      case 25: return 52;
      case 26: return 51;
      case 27: return 51;
      case 28: return 50;
      case 29: return 50;
      case 30: return 49;
      case 31: return 49;
      case 32: return 48;
      case 33: return 48;
      case 34: return 47;
      case 35: return 47;
      case 36: return 46;
      case 37: return 46;
      case 38: return 45;
      case 39: return 45;
      case 40: return 44;
      case 41: return 43;
      case 42: return 42;
      case 43: return 41;
      case 44: return 40;
      case 45: return 39;
      case 46: return 38;
      case 47: return 36;
      case 48: return 34;
      case 49: return 32;
      case 50: return 30;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for Druid breath saving throw.");
	break;
      }
    case SAVING_SPELL:	/* Generic spells */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 68;
      case  3: return 67;
      case  4: return 66;
      case  5: return 65;
      case  6: return 64;
      case  7: return 63;
      case  8: return 62;
      case  9: return 61;
      case 10: return 60;
      case 11: return 59;
      case 12: return 58;
      case 13: return 58;
      case 14: return 57;
      case 15: return 57;
      case 16: return 56;
      case 17: return 56;
      case 18: return 55;
      case 19: return 55;
      case 20: return 54;
      case 21: return 54;
      case 22: return 53;
      case 23: return 53;
      case 24: return 52;
      case 25: return 52;
      case 26: return 51;
      case 27: return 51;
      case 28: return 50;
      case 29: return 50;
      case 30: return 49;
      case 31: return 49;
      case 32: return 48;
      case 33: return 48;
      case 34: return 47;
      case 35: return 47;
      case 36: return 46;
      case 37: return 46;
      case 38: return 45;
      case 39: return 45;
      case 40: return 44;
      case 41: return 43;
      case 42: return 42;
      case 43: return 41;
      case 44: return 40;
      case 45: return 39;
      case 46: return 38;
      case 47: return 36;
      case 48: return 34;
      case 49: return 32;
      case 50: return 30;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for Druid spell saving throw.");
	break;
      }
    default:
      log("SYSERR: Invalid saving throw type.");
      break;
    }

  case CLASS_THIEF:
    switch (type) {
    case SAVING_PARA:	/* Paralyzation */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 69;
      case  3: return 68;
      case  4: return 68;
      case  5: return 67;
      case  6: return 67;
      case  7: return 66;
      case  8: return 66;
      case  9: return 65;
      case 10: return 65;
      case 11: return 64;
      case 12: return 64;
      case 13: return 63;
      case 14: return 63;
      case 15: return 62;
      case 16: return 62;
      case 17: return 61;
      case 18: return 61;
      case 19: return 60;
      case 20: return 60;
      case 21: return 59;
      case 22: return 59;
      case 23: return 58;
      case 24: return 58;
      case 25: return 57;
      case 26: return 57;
      case 27: return 56;
      case 28: return 56;
      case 29: return 55;
      case 30: return 55;
      case 31: return 54;
      case 32: return 54;
      case 33: return 53;
      case 34: return 53;
      case 35: return 52;
      case 36: return 52;
      case 37: return 51;
      case 38: return 51;
      case 39: return 50;
      case 40: return 50;
      case 41: return 49;
      case 42: return 48;
      case 43: return 47;
      case 44: return 46;
      case 45: return 45;
      case 46: return 44;
      case 47: return 43;
      case 48: return 42;
      case 49: return 41;
      case 50: return 40;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for Thief paralyzation saving throw.");
	break;
      }
    case SAVING_ROD:	/* Rods */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 69;
      case  3: return 68;
      case  4: return 68;
      case  5: return 67;
      case  6: return 67;
      case  7: return 66;
      case  8: return 66;
      case  9: return 65;
      case 10: return 65;
      case 11: return 64;
      case 12: return 64;
      case 13: return 63;
      case 14: return 63;
      case 15: return 62;
      case 16: return 62;
      case 17: return 61;
      case 18: return 61;
      case 19: return 60;
      case 20: return 60;
      case 21: return 59;
      case 22: return 59;
      case 23: return 58;
      case 24: return 58;
      case 25: return 57;
      case 26: return 57;
      case 27: return 56;
      case 28: return 56;
      case 29: return 55;
      case 30: return 55;
      case 31: return 54;
      case 32: return 54;
      case 33: return 53;
      case 34: return 53;
      case 35: return 52;
      case 36: return 52;
      case 37: return 51;
      case 38: return 51;
      case 39: return 50;
      case 40: return 50;
      case 41: return 49;
      case 42: return 48;
      case 43: return 47;
      case 44: return 46;
      case 45: return 45;
      case 46: return 44;
      case 47: return 43;
      case 48: return 42;
      case 49: return 41;
      case 50: return 40;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for Thief rod saving throw.");
	break;
      }
    case SAVING_PETRI:	/* Petrification */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 69;
      case  3: return 68;
      case  4: return 68;
      case  5: return 67;
      case  6: return 67;
      case  7: return 66;
      case  8: return 66;
      case  9: return 65;
      case 10: return 65;
      case 11: return 64;
      case 12: return 64;
      case 13: return 63;
      case 14: return 63;
      case 15: return 62;
      case 16: return 62;
      case 17: return 61;
      case 18: return 61;
      case 19: return 60;
      case 20: return 60;
      case 21: return 59;
      case 22: return 59;
      case 23: return 58;
      case 24: return 58;
      case 25: return 57;
      case 26: return 57;
      case 27: return 56;
      case 28: return 56;
      case 29: return 55;
      case 30: return 55;
      case 31: return 54;
      case 32: return 54;
      case 33: return 53;
      case 34: return 53;
      case 35: return 52;
      case 36: return 52;
      case 37: return 51;
      case 38: return 51;
      case 39: return 50;
      case 40: return 50;
      case 41: return 49;
      case 42: return 48;
      case 43: return 47;
      case 44: return 46;
      case 45: return 45;
      case 46: return 44;
      case 47: return 43;
      case 48: return 42;
      case 49: return 41;
      case 50: return 40;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for Thief petrification saving throw.");
	break;
      }
    case SAVING_BREATH:	/* Breath weapons */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 69;
      case  3: return 68;
      case  4: return 68;
      case  5: return 67;
      case  6: return 67;
      case  7: return 66;
      case  8: return 66;
      case  9: return 65;
      case 10: return 65;
      case 11: return 64;
      case 12: return 64;
      case 13: return 63;
      case 14: return 63;
      case 15: return 62;
      case 16: return 62;
      case 17: return 61;
      case 18: return 61;
      case 19: return 60;
      case 20: return 60;
      case 21: return 59;
      case 22: return 59;
      case 23: return 58;
      case 24: return 58;
      case 25: return 57;
      case 26: return 57;
      case 27: return 56;
      case 28: return 56;
      case 29: return 55;
      case 30: return 55;
      case 31: return 54;
      case 32: return 54;
      case 33: return 53;
      case 34: return 53;
      case 35: return 52;
      case 36: return 52;
      case 37: return 51;
      case 38: return 51;
      case 39: return 50;
      case 40: return 50;
      case 41: return 49;
      case 42: return 48;
      case 43: return 47;
      case 44: return 46;
      case 45: return 45;
      case 46: return 44;
      case 47: return 43;
      case 48: return 42;
      case 49: return 41;
      case 50: return 40;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for Thief breath saving throw.");
	break;
      }
    case SAVING_SPELL:	/* Generic spells */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 69;
      case  3: return 68;
      case  4: return 68;
      case  5: return 67;
      case  6: return 67;
      case  7: return 66;
      case  8: return 66;
      case  9: return 65;
      case 10: return 65;
      case 11: return 64;
      case 12: return 64;
      case 13: return 63;
      case 14: return 63;
      case 15: return 62;
      case 16: return 62;
      case 17: return 61;
      case 18: return 61;
      case 19: return 60;
      case 20: return 60;
      case 21: return 59;
      case 22: return 59;
      case 23: return 58;
      case 24: return 58;
      case 25: return 57;
      case 26: return 57;
      case 27: return 56;
      case 28: return 56;
      case 29: return 55;
      case 30: return 55;
      case 31: return 54;
      case 32: return 54;
      case 33: return 53;
      case 34: return 53;
      case 35: return 52;
      case 36: return 52;
      case 37: return 51;
      case 38: return 51;
      case 39: return 50;
      case 40: return 50;
      case 41: return 49;
      case 42: return 48;
      case 43: return 47;
      case 44: return 46;
      case 45: return 45;
      case 46: return 44;
      case 47: return 43;
      case 48: return 42;
      case 49: return 41;
      case 50: return 40;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for Thief spell saving throw.");
	break;
      }
    default:
      log("SYSERR: Invalid saving throw type.");
      break;
    }

  case CLASS_PSIONICIST:
    switch (type) {
    case SAVING_PARA:	/* Paralyzation */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 69;
      case  3: return 68;
      case  4: return 67;
      case  5: return 67;
      case  6: return 66;
      case  7: return 66;
      case  8: return 65;
      case  9: return 65;
      case 10: return 64;
      case 11: return 64;
      case 12: return 63;
      case 13: return 63;
      case 14: return 62;
      case 15: return 62;
      case 16: return 61;
      case 17: return 61;
      case 18: return 60;
      case 19: return 60;
      case 20: return 59;
      case 21: return 59;
      case 22: return 58;
      case 23: return 58;
      case 24: return 57;
      case 25: return 57;
      case 26: return 56;
      case 27: return 56;
      case 28: return 55;
      case 29: return 55;
      case 30: return 54;
      case 31: return 54;
      case 32: return 53;
      case 33: return 53;
      case 34: return 52;
      case 35: return 52;
      case 36: return 51;
      case 37: return 51;
      case 38: return 50;
      case 39: return 50;
      case 40: return 49;
      case 41: return 48;
      case 42: return 47;
      case 43: return 46;
      case 44: return 45;
      case 45: return 44;
      case 46: return 43;
      case 47: return 41;
      case 48: return 39;
      case 49: return 37;
      case 50: return 35;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for Psion paralyzation saving throw.");
	break;
      }
    case SAVING_ROD:	/* Rods */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 69;
      case  3: return 68;
      case  4: return 67;
      case  5: return 67;
      case  6: return 66;
      case  7: return 66;
      case  8: return 65;
      case  9: return 65;
      case 10: return 64;
      case 11: return 64;
      case 12: return 63;
      case 13: return 63;
      case 14: return 62;
      case 15: return 62;
      case 16: return 61;
      case 17: return 61;
      case 18: return 60;
      case 19: return 60;
      case 20: return 59;
      case 21: return 59;
      case 22: return 58;
      case 23: return 58;
      case 24: return 57;
      case 25: return 57;
      case 26: return 56;
      case 27: return 56;
      case 28: return 55;
      case 29: return 55;
      case 30: return 54;
      case 31: return 54;
      case 32: return 53;
      case 33: return 53;
      case 34: return 52;
      case 35: return 52;
      case 36: return 51;
      case 37: return 51;
      case 38: return 50;
      case 39: return 50;
      case 40: return 49;
      case 41: return 48;
      case 42: return 47;
      case 43: return 46;
      case 44: return 45;
      case 45: return 44;
      case 46: return 43;
      case 47: return 41;
      case 48: return 39;
      case 49: return 37;
      case 50: return 35;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for Psion rod saving throw.");
	break;
      }
    case SAVING_PETRI:	/* Petrification */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 69;
      case  3: return 68;
      case  4: return 67;
      case  5: return 67;
      case  6: return 66;
      case  7: return 66;
      case  8: return 65;
      case  9: return 65;
      case 10: return 64;
      case 11: return 64;
      case 12: return 63;
      case 13: return 63;
      case 14: return 62;
      case 15: return 62;
      case 16: return 61;
      case 17: return 61;
      case 18: return 60;
      case 19: return 60;
      case 20: return 59;
      case 21: return 59;
      case 22: return 58;
      case 23: return 58;
      case 24: return 57;
      case 25: return 57;
      case 26: return 56;
      case 27: return 56;
      case 28: return 55;
      case 29: return 55;
      case 30: return 54;
      case 31: return 54;
      case 32: return 53;
      case 33: return 53;
      case 34: return 52;
      case 35: return 52;
      case 36: return 51;
      case 37: return 51;
      case 38: return 50;
      case 39: return 50;
      case 40: return 49;
      case 41: return 48;
      case 42: return 47;
      case 43: return 46;
      case 44: return 45;
      case 45: return 44;
      case 46: return 43;
      case 47: return 41;
      case 48: return 39;
      case 49: return 37;
      case 50: return 35;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for Psion petrification saving throw.");
	break;
      }
    case SAVING_BREATH:	/* Breath weapons */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 69;
      case  3: return 68;
      case  4: return 67;
      case  5: return 67;
      case  6: return 66;
      case  7: return 66;
      case  8: return 65;
      case  9: return 65;
      case 10: return 64;
      case 11: return 64;
      case 12: return 63;
      case 13: return 63;
      case 14: return 62;
      case 15: return 62;
      case 16: return 61;
      case 17: return 61;
      case 18: return 60;
      case 19: return 60;
      case 20: return 59;
      case 21: return 59;
      case 22: return 58;
      case 23: return 58;
      case 24: return 57;
      case 25: return 57;
      case 26: return 56;
      case 27: return 56;
      case 28: return 55;
      case 29: return 55;
      case 30: return 54;
      case 31: return 54;
      case 32: return 53;
      case 33: return 53;
      case 34: return 52;
      case 35: return 52;
      case 36: return 51;
      case 37: return 51;
      case 38: return 50;
      case 39: return 50;
      case 40: return 49;
      case 41: return 48;
      case 42: return 47;
      case 43: return 46;
      case 44: return 45;
      case 45: return 44;
      case 46: return 43;
      case 47: return 41;
      case 48: return 39;
      case 49: return 37;
      case 50: return 35;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for Psion breath saving throw.");
	break;
      }
    case SAVING_SPELL:	/* Generic spells */
      switch (level) {
      case  0: return 90;
      case  1: return 70;
      case  2: return 69;
      case  3: return 68;
      case  4: return 67;
      case  5: return 67;
      case  6: return 66;
      case  7: return 66;
      case  8: return 65;
      case  9: return 65;
      case 10: return 64;
      case 11: return 64;
      case 12: return 63;
      case 13: return 63;
      case 14: return 62;
      case 15: return 62;
      case 16: return 61;
      case 17: return 61;
      case 18: return 60;
      case 19: return 60;
      case 20: return 59;
      case 21: return 59;
      case 22: return 58;
      case 23: return 58;
      case 24: return 57;
      case 25: return 57;
      case 26: return 56;
      case 27: return 56;
      case 28: return 55;
      case 29: return 55;
      case 30: return 54;
      case 31: return 54;
      case 32: return 53;
      case 33: return 53;
      case 34: return 52;
      case 35: return 52;
      case 36: return 51;
      case 37: return 51;
      case 38: return 50;
      case 39: return 50;
      case 40: return 49;
      case 41: return 48;
      case 42: return 47;
      case 43: return 46;
      case 44: return 45;
      case 45: return 44;
      case 46: return 43;
      case 47: return 41;
      case 48: return 39;
      case 49: return 37;
      case 50: return 35;
      case 51: return  0;
      case 52: return  0;
      case 53: return  0;
      case 54: return  0;
      case 55: return  0;
      case 56: return  0;
      case 57: return  0;
      case 58: return  0;
      case 59: return  0;
      case 60: return  0;
      case 61: return  0;
      case 62: return  0;
      case 63: return  0;
      case 64: return  0;
      case 65: return  0;
      default:
	log("SYSERR: Missing level for Psion spell saving throw.");
	break;
      }
    default:
      log("SYSERR: Invalid saving throw type.");
      break;
    }

  default:
    log("SYSERR: Invalid class saving throw.");
    break;
  }

  /* Should not get here unless something is wrong. */
  return 100;
}


/* THAC0 for classes and levels.  (To Hit Armor Class 0) */
int thaco(int class_num, int level)
{
  switch (class_num) {

  case CLASS_KNIGHT:
    switch (level) {
    case  0: return 100;
    case  1: return  20;
    case  2: return  20;
    case  3: return  20;
    case  4: return  19;
    case  5: return  19;
    case  6: return  19;
    case  7: return  18;
    case  8: return  18;
    case  9: return  18;
    case 10: return  17;
    case 11: return  17;
    case 12: return  17;
    case 13: return  16;
    case 14: return  16;
    case 15: return  16;
    case 16: return  15;
    case 17: return  15;
    case 18: return  15;
    case 19: return  14;
    case 20: return  14;
    case 21: return  14;
    case 22: return  13;
    case 23: return  13;
    case 24: return  13;
    case 25: return  12;
    case 26: return  12;
    case 27: return  12;
    case 28: return  11;
    case 29: return  11;
    case 30: return  11;
    case 31: return  10;
    case 32: return  10;
    case 33: return  10;
    case 34: return   9;
    case 35: return   9;
    case 36: return   9;
    case 37: return   8;
    case 38: return   8;
    case 39: return   8; 
    case 40: return   7;
    case 41: return   7;
    case 42: return   7; 
    case 43: return   6;
    case 44: return   6;
    case 45: return   5;
    case 46: return   5;
    case 47: return   4;
    case 48: return   3;
    case 49: return   2;
    case 50: return   1;
    case 51: return   1;
    case 52: return   1;
    case 53: return   1;
    case 54: return   1;
    case 55: return   1;
    case 56: return   1;
    case 57: return   1;
    case 58: return   1;
    case 59: return   1;
    case 60: return   1;
    case 61: return   1;
    case 62: return   1;
    case 63: return   1;
    case 64: return   1;
    case 65: return   1;
    default:
      log("SYSERR: Missing level for Knight thac0.");
    }

  case CLASS_CLERIC:
    switch (level) {
    case  0: return 100;
    case  1: return  20;
    case  2: return  20;
    case  3: return  20;
    case  4: return  19;
    case  5: return  19;
    case  6: return  19;
    case  7: return  18;
    case  8: return  18;
    case  9: return  18;
    case 10: return  17;
    case 11: return  17;
    case 12: return  17;
    case 13: return  16;
    case 14: return  16;
    case 15: return  16;
    case 16: return  15;
    case 17: return  15;
    case 18: return  15;
    case 19: return  14;
    case 20: return  14;
    case 21: return  14;
    case 22: return  13;
    case 23: return  13;
    case 24: return  13;
    case 25: return  12;
    case 26: return  12;
    case 27: return  12;
    case 28: return  11;
    case 29: return  11;
    case 30: return  11;
    case 31: return  10;
    case 32: return  10;
    case 33: return  10;
    case 34: return   9;
    case 35: return   9;
    case 36: return   9;
    case 37: return   8;
    case 38: return   8;
    case 39: return   8; 
    case 40: return   7;
    case 41: return   7;
    case 42: return   7; 
    case 43: return   6;
    case 44: return   6;
    case 45: return   6;
    case 46: return   5;
    case 47: return   5;
    case 48: return   4;
    case 49: return   4;
    case 50: return   3;
    case 51: return   1;
    case 52: return   1;
    case 53: return   1;
    case 54: return   1;
    case 55: return   1;
    case 56: return   1;
    case 57: return   1;
    case 58: return   1;
    case 59: return   1;
    case 60: return   1;
    case 61: return   1;
    case 62: return   1;
    case 63: return   1;
    case 64: return   1;
    case 65: return   1;
    default:
      log("SYSERR: Missing level for cleric thac0.");
    }

  case CLASS_BARBARIAN:
    switch (level) {
    case  0: return 100;
    case  1: return  20;
    case  2: return  20;
    case  3: return  20;
    case  4: return  19;
    case  5: return  19;
    case  6: return  19;
    case  7: return  18;
    case  8: return  18;
    case  9: return  18;
    case 10: return  17;
    case 11: return  17;
    case 12: return  17;
    case 13: return  16;
    case 14: return  16;
    case 15: return  16;
    case 16: return  15;
    case 17: return  15;
    case 18: return  15;
    case 19: return  14;
    case 20: return  14;
    case 21: return  14;
    case 22: return  13;
    case 23: return  13;
    case 24: return  13;
    case 25: return  12;
    case 26: return  12;
    case 27: return  12;
    case 28: return  11;
    case 29: return  11;
    case 30: return  11;
    case 31: return  10;
    case 32: return  10;
    case 33: return  10;
    case 34: return   9;
    case 35: return   9;
    case 36: return   9;
    case 37: return   8;
    case 38: return   8;
    case 39: return   7; 
    case 40: return   7;
    case 41: return   6;
    case 42: return   6; 
    case 43: return   5;
    case 44: return   5;
    case 45: return   4;
    case 46: return   3;
    case 47: return   2;
    case 48: return   1;
    case 49: return   1;
    case 50: return   1;
    case 51: return   1;
    case 52: return   1;
    case 53: return   1;
    case 54: return   1;
    case 55: return   1;
    case 56: return   1;
    case 57: return   1;
    case 58: return   1;
    case 59: return   1;
    case 60: return   1;
    case 61: return   1;
    case 62: return   1;
    case 63: return   1;
    case 64: return   1;
    case 65: return   1;
    default:
      log("SYSERR: Missing level for Barbarian thac0.");
    }

  case CLASS_RANGER:
    switch (level) {
    case  0: return 100;
    case  1: return  20;
    case  2: return  20;
    case  3: return  20;
    case  4: return  19;
    case  5: return  19;
    case  6: return  19;
    case  7: return  18;
    case  8: return  18;
    case  9: return  18;
    case 10: return  17;
    case 11: return  17;
    case 12: return  17;
    case 13: return  16;
    case 14: return  16;
    case 15: return  16;
    case 16: return  15;
    case 17: return  15;
    case 18: return  15;
    case 19: return  14;
    case 20: return  14;
    case 21: return  14;
    case 22: return  13;
    case 23: return  13;
    case 24: return  13;
    case 25: return  12;
    case 26: return  12;
    case 27: return  12;
    case 28: return  11;
    case 29: return  11;
    case 30: return  11;
    case 31: return  10;
    case 32: return  10;
    case 33: return  10;
    case 34: return   9;
    case 35: return   9;
    case 36: return   9;
    case 37: return   8;
    case 38: return   8;
    case 39: return   8; 
    case 40: return   7;
    case 41: return   7;
    case 42: return   7; 
    case 43: return   6;
    case 44: return   6;
    case 45: return   5;
    case 46: return   5;
    case 47: return   4;
    case 48: return   3;
    case 49: return   2;
    case 50: return   1;
    case 51: return   1;
    case 52: return   1;
    case 53: return   1;
    case 54: return   1;
    case 55: return   1;
    case 56: return   1;
    case 57: return   1;
    case 58: return   1;
    case 59: return   1;
    case 60: return   1;
    case 61: return   1;
    case 62: return   1;
    case 63: return   1;
    case 64: return   1;
    case 65: return   1;

    default:
      log("SYSERR: Missing level for Ranger thac0.");
    }

  case CLASS_MAGE:
    switch (level) {
    case  0: return 100;
    case  1: return  20;
    case  2: return  20;
    case  3: return  20;
    case  4: return  20;
    case  5: return  20;
    case  6: return  20;
    case  7: return  19;
    case  8: return  19;
    case  9: return  19;
    case 10: return  19;
    case 11: return  19;
    case 12: return  19;
    case 13: return  18;
    case 14: return  18;
    case 15: return  18;
    case 16: return  18;
    case 17: return  18;
    case 18: return  18;
    case 19: return  17;
    case 20: return  17;
    case 21: return  17;
    case 22: return  17;
    case 23: return  17;
    case 24: return  16;
    case 25: return  16;
    case 26: return  16;
    case 27: return  16;
    case 28: return  16;
    case 29: return  15;
    case 30: return  15;
    case 31: return  15;
    case 32: return  15;
    case 33: return  15;
    case 34: return  14;
    case 35: return  14;
    case 36: return  14;
    case 37: return  14;
    case 38: return  14;
    case 39: return  13; 
    case 40: return  13;
    case 41: return  13;
    case 42: return  13; 
    case 43: return  13;
    case 44: return  12;
    case 45: return  12;
    case 46: return  12;
    case 47: return  12;
    case 48: return  11;
    case 49: return  11;
    case 50: return  11;
    case 51: return   1;
    case 52: return   1;
    case 53: return   1;
    case 54: return   1;
    case 55: return   1;
    case 56: return   1;
    case 57: return   1;
    case 58: return   1;
    case 59: return   1;
    case 60: return   1;
    case 61: return   1;
    case 62: return   1;
    case 63: return   1;
    case 64: return   1;
    case 65: return   1;
    default:
      log("SYSERR: Missing level for mage thac0.");
    }

  case CLASS_MONK:
    switch (level) {
    case  0: return 100;
    case  1: return  20;
    case  2: return  20;
    case  3: return  20;
    case  4: return  19;
    case  5: return  19;
    case  6: return  19;
    case  7: return  18;
    case  8: return  18;
    case  9: return  18;
    case 10: return  17;
    case 11: return  17;
    case 12: return  17;
    case 13: return  16;
    case 14: return  16;
    case 15: return  16;
    case 16: return  15;
    case 17: return  15;
    case 18: return  15;
    case 19: return  14;
    case 20: return  14;
    case 21: return  14;
    case 22: return  13;
    case 23: return  13;
    case 24: return  13;
    case 25: return  12;
    case 26: return  12;
    case 27: return  12;
    case 28: return  11;
    case 29: return  11;
    case 30: return  11;
    case 31: return  10;
    case 32: return  10;
    case 33: return  10;
    case 34: return   9;
    case 35: return   9;
    case 36: return   9;
    case 37: return   8;
    case 38: return   8;
    case 39: return   7; 
    case 40: return   7;
    case 41: return   6;
    case 42: return   6; 
    case 43: return   5;
    case 44: return   5;
    case 45: return   4;
    case 46: return   3;
    case 47: return   2;
    case 48: return   1;
    case 49: return   1;
    case 50: return   1;
    case 51: return   1;
    case 52: return   1;
    case 53: return   1;
    case 54: return   1;
    case 55: return   1;
    case 56: return   1;
    case 57: return   1;
    case 58: return   1;
    case 59: return   1;
    case 60: return   1;
    case 61: return   1;
    case 62: return   1;
    case 63: return   1;
    case 64: return   1;
    case 65: return   1;
    default:
      log("SYSERR: Missing level for Monk thac0.");
    }

  case CLASS_NECROMANCER:
    switch (level) {
    case  0: return 100;
    case  1: return  20;
    case  2: return  20;
    case  3: return  20;
    case  4: return  20;
    case  5: return  20;
    case  6: return  20;
    case  7: return  19;
    case  8: return  19;
    case  9: return  19;
    case 10: return  19;
    case 11: return  19;
    case 12: return  19;
    case 13: return  18;
    case 14: return  18;
    case 15: return  18;
    case 16: return  18;
    case 17: return  18;
    case 18: return  18;
    case 19: return  17;
    case 20: return  17;
    case 21: return  17;
    case 22: return  17;
    case 23: return  17;
    case 24: return  16;
    case 25: return  16;
    case 26: return  16;
    case 27: return  16;
    case 28: return  16;
    case 29: return  15;
    case 30: return  15;
    case 31: return  15;
    case 32: return  15;
    case 33: return  15;
    case 34: return  14;
    case 35: return  14;
    case 36: return  14;
    case 37: return  14;
    case 38: return  14;
    case 39: return  13; 
    case 40: return  13;
    case 41: return  13;
    case 42: return  13; 
    case 43: return  13;
    case 44: return  12;
    case 45: return  12;
    case 46: return  12;
    case 47: return  12;
    case 48: return  11;
    case 49: return  11;
    case 50: return  11;
    case 51: return   1;
    case 52: return   1;
    case 53: return   1;
    case 54: return   1;
    case 55: return   1;
    case 56: return   1;
    case 57: return   1;
    case 58: return   1;
    case 59: return   1;
    case 60: return   1;
    case 61: return   1;
    case 62: return   1;
    case 63: return   1;
    case 64: return   1;
    case 65: return   1;
    default:
      log("SYSERR: Missing level for Necromancer thac0.");
    }

  case CLASS_DRUID:
    switch (level) {
    case  0: return 100;
    case  1: return  20;
    case  2: return  20;
    case  3: return  20;
    case  4: return  19;
    case  5: return  19;
    case  6: return  19;
    case  7: return  18;
    case  8: return  18;
    case  9: return  18;
    case 10: return  17;
    case 11: return  17;
    case 12: return  17;
    case 13: return  16;
    case 14: return  16;
    case 15: return  16;
    case 16: return  15;
    case 17: return  15;
    case 18: return  15;
    case 19: return  14;
    case 20: return  14;
    case 21: return  14;
    case 22: return  13;
    case 23: return  13;
    case 24: return  13;
    case 25: return  12;
    case 26: return  12;
    case 27: return  12;
    case 28: return  11;
    case 29: return  11;
    case 30: return  11;
    case 31: return  10;
    case 32: return  10;
    case 33: return  10;
    case 34: return   9;
    case 35: return   9;
    case 36: return   9;
    case 37: return   8;
    case 38: return   8;
    case 39: return   8; 
    case 40: return   7;
    case 41: return   7;
    case 42: return   7; 
    case 43: return   6;
    case 44: return   6;
    case 45: return   6;
    case 46: return   5;
    case 47: return   5;
    case 48: return   4;
    case 49: return   4;
    case 50: return   3;
    case 51: return   1;
    case 52: return   1;
    case 53: return   1;
    case 54: return   1;
    case 55: return   1;
    case 56: return   1;
    case 57: return   1;
    case 58: return   1;
    case 59: return   1;
    case 60: return   1;
    case 61: return   1;
    case 62: return   1;
    case 63: return   1;
    case 64: return   1;
    case 65: return   1;
    default:
      log("SYSERR: Missing level for Druid thac0.");
    }
  case CLASS_THIEF:
    switch (level) {
    case  0: return 100;
    case  1: return  20;
    case  2: return  20;
    case  3: return  20;
    case  4: return  20;
    case  5: return  20;
    case  6: return  19;
    case  7: return  19;
    case  8: return  19;
    case  9: return  19;
    case 10: return  18;
    case 11: return  18;
    case 12: return  18;
    case 13: return  18;
    case 14: return  17;
    case 15: return  17;
    case 16: return  17;
    case 17: return  17;
    case 18: return  16;
    case 19: return  16;
    case 20: return  16;
    case 21: return  16;
    case 22: return  15;
    case 23: return  15;
    case 24: return  15;
    case 25: return  15;
    case 26: return  14;
    case 27: return  14;
    case 28: return  14;
    case 29: return  14;
    case 30: return  13;
    case 31: return  13;
    case 32: return  13;
    case 33: return  13;
    case 34: return  12;
    case 35: return  12;
    case 36: return  12;
    case 37: return  11;
    case 38: return  11;
    case 39: return  11; 
    case 40: return  10;
    case 41: return  10;
    case 42: return  10; 
    case 43: return   9;
    case 44: return   9;
    case 45: return   8;
    case 46: return   8;
    case 47: return   7;
    case 48: return   7;
    case 49: return   6;
    case 50: return   6;
    case 51: return   1;
    case 52: return   1;
    case 53: return   1;
    case 54: return   1;
    case 55: return   1;
    case 56: return   1;
    case 57: return   1;
    case 58: return   1;
    case 59: return   1;
    case 60: return   1;
    case 61: return   1;
    case 62: return   1;
    case 63: return   1;
    case 64: return   1;
    case 65: return   1;
    default:
      log("SYSERR: Missing level for thief thac0.");
    }

  case CLASS_PSIONICIST:
    switch (level) {
    case  0: return 100;
    case  1: return  20;
    case  2: return  20;
    case  3: return  20;
    case  4: return  20;
    case  5: return  20;
    case  6: return  20;
    case  7: return  19;
    case  8: return  19;
    case  9: return  19;
    case 10: return  19;
    case 11: return  19;
    case 12: return  19;
    case 13: return  18;
    case 14: return  18;
    case 15: return  18;
    case 16: return  18;
    case 17: return  18;
    case 18: return  18;
    case 19: return  17;
    case 20: return  17;
    case 21: return  17;
    case 22: return  17;
    case 23: return  17;
    case 24: return  16;
    case 25: return  16;
    case 26: return  16;
    case 27: return  16;
    case 28: return  16;
    case 29: return  15;
    case 30: return  15;
    case 31: return  15;
    case 32: return  15;
    case 33: return  15;
    case 34: return  14;
    case 35: return  14;
    case 36: return  14;
    case 37: return  14;
    case 38: return  14;
    case 39: return  13; 
    case 40: return  13;
    case 41: return  13;
    case 42: return  13; 
    case 43: return  13;
    case 44: return  12;
    case 45: return  12;
    case 46: return  12;
    case 47: return  12;
    case 48: return  11;
    case 49: return  11;
    case 50: return  11;
    case 51: return   1;
    case 52: return   1;
    case 53: return   1;
    case 54: return   1;
    case 55: return   1;
    case 56: return   1;
    case 57: return   1;
    case 58: return   1;
    case 59: return   1;
    case 60: return   1;
    case 61: return   1;
    case 62: return   1;
    case 63: return   1;
    case 64: return   1;
    case 65: return   1;
    default:
      log("SYSERR: Missing level for Psionicist thac0.");
    }

  default:
    log("SYSERR: Unknown class in thac0 chart.");
  }

  /* Will not get there unless something is wrong. */
  return 100;
}



/* Roll the 6 stats for a character... each stat is made of the sum of the best
 * 3 out of 4 rolls of a 6-sided die.  Each class then decides which priority 
 * will be given for the best to worst stats. */
void roll_real_abils(struct char_data *ch)
{
  int i, j, k, temp;
  ubyte table[6];
  ubyte rolls[4];

  for (i = 0; i < 6; i++)
    table[i] = 0;

  for (i = 0; i < 6; i++) {

    for (j = 0; j < 4; j++)
      rolls[j] = rand_number(1, 6);

    temp = rolls[0] + rolls[1] + rolls[2] + rolls[3] -
      MIN(rolls[0], MIN(rolls[1], MIN(rolls[2], rolls[3])));

    for (k = 0; k < 6; k++)
      if (table[k] < temp) {
	temp ^= table[k];
	table[k] ^= temp;
	temp ^= table[k];
      }
  }

  /*  ch->real_abils.str_add = 0; */

  switch (GET_CLASS(ch)) {

  case CLASS_KNIGHT:
    ch->real_abils.str = table[0];
    ch->real_abils.dex = table[1];
    ch->real_abils.con = table[2];
    ch->real_abils.wis = table[3];
    ch->real_abils.intel = table[4];
    ch->real_abils.cha = table[5];
/*    if (ch->real_abils.str == 18)
      ch->real_abils.str_add = rand_number(0, 100); */
    break;

  case CLASS_CLERIC:
    ch->real_abils.wis = table[0];
    ch->real_abils.intel = table[1];
    ch->real_abils.str = table[2];
    ch->real_abils.dex = table[3];
    ch->real_abils.con = table[4];
    ch->real_abils.cha = table[5];
    break;

  case CLASS_BARBARIAN:
    ch->real_abils.str = table[0];
    ch->real_abils.dex = table[1];
    ch->real_abils.con = table[2];
    ch->real_abils.wis = table[3];
    ch->real_abils.intel = table[4];
    ch->real_abils.cha = table[5];
/*    if (ch->real_abils.str == 18)
      ch->real_abils.str_add = rand_number(0, 100); */
    break;

  case CLASS_RANGER:
    ch->real_abils.str = table[0];
    ch->real_abils.dex = table[1];
    ch->real_abils.con = table[2];
    ch->real_abils.wis = table[3];
    ch->real_abils.intel = table[4];
    ch->real_abils.cha = table[5];
/*    if (ch->real_abils.str == 18)
      ch->real_abils.str_add = rand_number(0, 100); */
    break;

  case CLASS_MAGE:
    ch->real_abils.intel = table[0];
    ch->real_abils.wis = table[1];
    ch->real_abils.dex = table[2];
    ch->real_abils.str = table[3];
    ch->real_abils.con = table[4];
    ch->real_abils.cha = table[5];
    break;

  case CLASS_MONK:
    ch->real_abils.str = table[0];
    ch->real_abils.dex = table[1];
    ch->real_abils.con = table[2];
    ch->real_abils.wis = table[3];
    ch->real_abils.intel = table[4];
    ch->real_abils.cha = table[5];
/*    if (ch->real_abils.str == 18)
      ch->real_abils.str_add = rand_number(0, 100); */
    break;

  case CLASS_NECROMANCER:
    ch->real_abils.intel = table[0];
    ch->real_abils.wis = table[1];
    ch->real_abils.dex = table[2];
    ch->real_abils.str = table[3];
    ch->real_abils.con = table[4];
    ch->real_abils.cha = table[5];
    break;

  case CLASS_DRUID:
    ch->real_abils.wis = table[0];
    ch->real_abils.intel = table[1];
    ch->real_abils.str = table[2];
    ch->real_abils.dex = table[3];
    ch->real_abils.con = table[4];
    ch->real_abils.cha = table[5];
    break;

  case CLASS_THIEF:
    ch->real_abils.dex = table[0];
    ch->real_abils.str = table[1];
    ch->real_abils.con = table[2];
    ch->real_abils.intel = table[3];
    ch->real_abils.wis = table[4];
    ch->real_abils.cha = table[5];
    break;

  case CLASS_PSIONICIST:
    ch->real_abils.intel = table[0];
    ch->real_abils.wis = table[1];
    ch->real_abils.dex = table[2];
    ch->real_abils.str = table[3];
    ch->real_abils.con = table[4];
    ch->real_abils.cha = table[5];
    break;

 } 
 
 switch (GET_RACE(ch)) {

	case RACE_ELF:
		++ch->real_abils.intel;
		++ch->real_abils.dex;
		--ch->real_abils.str;
		--ch->real_abils.con;
	break;

	case RACE_DWARF:
		++ch->real_abils.str;
		ch->real_abils.con+=2;
		--ch->real_abils.intel;
		--ch->real_abils.wis;
		--ch->real_abils.dex;
	break;

	case RACE_HUMAN:
	break;

	case RACE_KENDER:
		ch->real_abils.dex+=2;
		--ch->real_abils.str;
		--ch->real_abils.intel;
	break;

	case RACE_HALF_ORC:
		++ch->real_abils.str;
		--ch->real_abils.intel;
		ch->real_abils.cha-=2;
	break;

	case RACE_HALF_OGRE:
		ch->real_abils.str+=3; 
		ch->real_abils.intel-=2;
		--ch->real_abils.wis;
		ch->real_abils.cha-=2;
	break;
	
	case RACE_BAKALI:
		ch->real_abils.str+=2;
		++ch->real_abils.con;
		--ch->real_abils.intel;
		ch->real_abils.wis-=2;
		ch->real_abils.cha-=2;
	break;
 }

  ch->aff_abils = ch->real_abils;
}

/* Some initializations for characters, including initial skills */
void do_start(struct char_data *ch)
{
  GET_LEVEL(ch) = 1;
  GET_EXP(ch) = 1;

  /*set_title(ch, NULL);*/
  /* roll_real_abils(ch); */

  GET_MAX_HIT(ch)  = 10;
  GET_MAX_MANA(ch) = 100;
  GET_MAX_MOVE(ch) = 82;

  switch (GET_CLASS(ch)) {

  case CLASS_KNIGHT:
    break;

  case CLASS_CLERIC:
    break;

  case CLASS_BARBARIAN:
    break;

  case CLASS_RANGER:
    break;

  case CLASS_MAGE:
    break;

  case CLASS_MONK:
    break;

  case CLASS_NECROMANCER:
    break;

  case CLASS_DRUID:
    break;

  case CLASS_THIEF:
    SET_SKILL(ch, SKILL_SNEAK, 10);
    SET_SKILL(ch, SKILL_HIDE, 5);
    SET_SKILL(ch, SKILL_STEAL, 15);
    SET_SKILL(ch, SKILL_BACKSTAB, 10);
    SET_SKILL(ch, SKILL_PICK_LOCK, 10);
    SET_SKILL(ch, SKILL_TRACK, 10);
    break;

  case CLASS_PSIONICIST:
    break;
  }


  advance_level(ch);

  GET_HIT(ch) = GET_MAX_HIT(ch);
  GET_MANA(ch) = GET_MAX_MANA(ch);
  GET_MOVE(ch) = GET_MAX_MOVE(ch);

  GET_COND(ch, THIRST) = 24;
  GET_COND(ch, HUNGER) = 24;
  GET_COND(ch, DRUNK) = 0; 

  if (CONFIG_SITEOK_ALL)
    SET_BIT_AR(PLR_FLAGS(ch), PLR_SITEOK);
}

/* This function controls the change to maxmove, maxmana, and maxhp for each 
 * class every time they gain a level. */
void advance_level(struct char_data *ch)
{
  int add_hp, add_mana = 0, add_move = 0, i;
  int remort_hp = 0, remort_mana = 0, remort_move = 0;

  add_hp = con_app[GET_CON(ch)].hitp;

  switch (GET_CLASS(ch)) {

  case CLASS_KNIGHT:
    add_hp += rand_number(15, 20);
    add_mana += rand_number(2, 7); 
    add_move += rand_number(7, 11);
    break;

  case CLASS_CLERIC:
    add_hp += rand_number(6, 11);
    add_mana += rand_number(11, 16); 
    add_move += rand_number(5, 10);
    break;

  case CLASS_BARBARIAN:
    add_hp += rand_number(25, 30);
    add_mana += 0;
    add_move += rand_number(15, 20);
    break;

  case CLASS_RANGER:
    add_hp += rand_number(19, 24);
    add_mana += rand_number(2, 7); 
    add_move += rand_number(10, 15);
    break;

  case CLASS_MAGE:
    add_hp += rand_number(4, 9);
    add_mana += rand_number(20, 25); 
    add_move += rand_number(3, 8);
    break;

  case CLASS_MONK:
    add_hp += rand_number(8, 13);
    add_mana += rand_number(2, 7);
    add_move += rand_number(10, 15);
    break;

  case CLASS_NECROMANCER:
    add_hp += rand_number(5, 10);
    add_mana += rand_number(11, 16); 
    add_move += rand_number(3, 8);
    break;

  case CLASS_DRUID:
    add_hp += rand_number(6, 11);
    add_mana += rand_number(2, 7); 
    add_move += rand_number(5, 10);
    break;

  case CLASS_THIEF:
    add_hp += rand_number(5, 10);
    add_mana += 0;
    add_move += rand_number(5, 10);
    break;

  case CLASS_PSIONICIST:
    add_hp += rand_number(4, 9);
    add_mana += rand_number(6, 11);
    add_move += rand_number(3, 8);
    break;

  }
  
  if (GET_GEN(ch) >= 1) {
  
  switch (GET_REMORT(ch)) {

  case CLASS_KNIGHT:
    remort_hp += rand_number(15, 20);
    remort_mana += rand_number(2, 7); 
    remort_move += rand_number(7, 11);
    break;

  case CLASS_CLERIC:
    remort_hp += rand_number(6, 11);
    remort_mana += rand_number(11, 16); 
    remort_move += rand_number(5, 10);
    break;

  case CLASS_BARBARIAN:
    remort_hp += rand_number(25, 30);
    remort_mana += 0;
    remort_move += rand_number(15, 20);
    break;

  case CLASS_RANGER:
    remort_hp += rand_number(19, 24);
    remort_mana += rand_number(2, 7); 
    remort_move += rand_number(10, 15);
    break;

  case CLASS_MAGE:
    remort_hp += rand_number(4, 9);
    remort_mana += rand_number(20, 25); 
    remort_move += rand_number(3, 8);
    break;

  case CLASS_MONK:
    remort_hp += rand_number(8, 13);
    remort_mana += rand_number(2, 7);
    remort_move += rand_number(10, 15);
    break;

  case CLASS_NECROMANCER:
    remort_hp += rand_number(5, 10);
    remort_mana += rand_number(11, 16); 
    remort_move += rand_number(3, 8);
    break;

  case CLASS_DRUID:
    remort_hp += rand_number(6, 11);
    remort_mana += rand_number(2, 7); 
    remort_move += rand_number(5, 10);
    break;

  case CLASS_THIEF:
    remort_hp += rand_number(5, 10);
    remort_mana += 0;
    remort_move += rand_number(5, 10);
    break;

  case CLASS_PSIONICIST:
    remort_hp += rand_number(4, 9);
    remort_mana += rand_number(6, 11);
    remort_move += rand_number(3, 8);
    break;

  }
 }

// Bonus based on race 
  if (GET_RACE(ch) == RACE_HALF_OGRE) {
   add_hp += rand_number(8, 12);
   add_move += MIN(add_move, rand_number(3, 5));
   }

// Bonus based on remort class gen 1 thru 3 
  if ((GET_GEN(ch) >= 1) && (GET_GEN(ch) <= 3)) {
   add_mana += MAX(0, (remort_mana / 8));
   add_move += MAX(0, (remort_move / 8));
   add_hp += MAX(0, (remort_move / 8));
  }

// Bonus based on remort class gen 4 thru 6 
  if ((GET_GEN(ch) >= 4) && (GET_GEN(ch) <= 6)) {
   add_mana += MAX(0, (remort_mana / 4));
   add_move += MAX(0, (remort_move / 4));
   add_hp += MAX(0, (remort_move / 4));
  }

// Bonus based on remort class gen 7 thru 9 
  if ((GET_GEN(ch) >= 7) && (GET_GEN(ch) <= 9)) {
   add_mana += MAX(0, (remort_mana / 2));
   add_move += MAX(0, (remort_move / 2));
   add_hp += MAX(0, (remort_move / 2));
  }

// Bonus based on remort class gen 10 
  if (GET_GEN(ch) == 10) {
   add_mana += remort_mana;
   add_move += remort_move;
   add_hp += remort_move;
  }
  
// Bonus to mana based on class and intelligence 
  if ( IS_MAGE(ch) || IS_CLERIC(ch) || IS_NECROMANCER(ch) || IS_PSIONICIST(ch) )
   add_mana += MAX(0, (GET_INT(ch) / 4));
  
  if (IS_KNIGHT(ch) || IS_DRUID(ch))
   add_mana += MAX(0, (GET_INT(ch) / 6));
  
  if (IS_BARBARIAN(ch) || IS_RANGER(ch) || IS_MONK(ch) || IS_THIEF(ch))
   add_mana += MAX(0, (GET_INT(ch) / 10));
  
// Bonus based on generation  
  if (GET_GEN(ch) >= 1) {
   add_mana += MAX(0, (GET_GEN(ch) / 2));
   add_hp += MAX(0, (GET_GEN(ch) / 2));
   add_move += MAX(0, (GET_GEN(ch) /2));
  }

  ch->points.max_hit += MAX(1, add_hp);
  ch->points.max_move += MAX(1, add_move);

  if (GET_LEVEL(ch) > 1)
    ch->points.max_mana += add_mana;

  // Practice points per level is based solely on intelligence
  GET_PRACTICES(ch) += MAX(1, (int_app[GET_INT(ch)].learn / 12));
	
  GET_SOUL_POINTS(ch) += MAX(0, (GET_LEVEL(ch) * GET_CON(ch) / 125)) ;

  if (GET_LEVEL(ch) >= LVL_IMMORT) {
    for (i = 0; i < 3; i++)
      GET_COND(ch, i) = (char) -1;
    SET_BIT_AR(PRF_FLAGS(ch), PRF_HOLYLIGHT);
  }

  snoop_check(ch);
  save_char(ch);
}

/* This simply calculates the backstab multiplier based on a character's level.
 * This used to be an array, but was changed to be a function so that it would 
 * be easier to add more levels to your MUD.  This doesn't really create a big 
 * performance hit because it's not used very often. */
int backstab_mult(int level)
{
  if (level <= 7)
    return 2;	  /* level 1 - 7 */
  else if (level <= 13)
    return 3;	  /* level 8 - 13 */
  else if (level <= 20)
    return 4;	  /* level 14 - 20 */
  else if (level <= 28)
    return 5;	  /* level 21 - 28 */
  else if (level < LVL_IMMORT)
    return 6;	  /* all remaining mortal levels */
  else
    return 20;	  /* immortals */
}

/* invalid_class is used by handler.c to determine if a piece of equipment is
 * usable by a particular class, based on the ITEM_ANTI_{class} bitvectors. */
int invalid_class(struct char_data *ch, struct obj_data *obj)
{
  if (OBJ_ANTICLASS_FLAGGED(obj, ITEM_ANTI_MAGE) && IS_MAGE(ch))
    return TRUE;

  if (OBJ_ANTICLASS_FLAGGED(obj, ITEM_ANTI_KNIGHT) && IS_KNIGHT(ch))
    return TRUE;
	
  if (OBJ_ANTICLASS_FLAGGED(obj, ITEM_ANTI_CLERIC) && IS_CLERIC(ch))
    return TRUE;
	
  if (OBJ_ANTICLASS_FLAGGED(obj, ITEM_ANTI_BARBARIAN) && IS_BARBARIAN(ch))
    return TRUE;
	
  if (OBJ_ANTICLASS_FLAGGED(obj, ITEM_ANTI_RANGER) && IS_RANGER(ch))
    return TRUE;
	
  if (OBJ_ANTICLASS_FLAGGED(obj, ITEM_ANTI_MONK) && IS_MONK(ch))
    return TRUE;
	
  if (OBJ_ANTICLASS_FLAGGED(obj, ITEM_ANTI_NECROMANCER) && IS_NECROMANCER(ch))
    return TRUE;
	
  if (OBJ_ANTICLASS_FLAGGED(obj, ITEM_ANTI_DRUID) && IS_DRUID(ch))
    return TRUE;
	
  if (OBJ_ANTICLASS_FLAGGED(obj, ITEM_ANTI_THIEF) && IS_THIEF(ch))
    return TRUE;
	
  if (OBJ_ANTICLASS_FLAGGED(obj, ITEM_ANTI_PSIONICIST) && IS_PSIONICIST(ch))
    return TRUE;
	
  return FALSE;
}

/* invalid_class is used by handler.c to determine if a piece of equipment is
 * usable by a particular class, based on the ITEM_ANTI_{class} bitvectors. */
int invalid_needed_class(struct char_data *ch, struct obj_data *obj)
{
  if (OBJ_NEEDCLASS_FLAGGED(obj, ITEM_NEED_MAGE) && !IS_MAGE(ch))
    return TRUE;

  if (OBJ_NEEDCLASS_FLAGGED(obj, ITEM_NEED_KNIGHT) && !IS_KNIGHT(ch))
    return TRUE;
	
  if (OBJ_NEEDCLASS_FLAGGED(obj, ITEM_NEED_CLERIC) && !IS_CLERIC(ch))
    return TRUE;
	
  if (OBJ_NEEDCLASS_FLAGGED(obj, ITEM_NEED_BARBARIAN) && !IS_BARBARIAN(ch))
    return TRUE;
	
  if (OBJ_NEEDCLASS_FLAGGED(obj, ITEM_NEED_RANGER) && !IS_RANGER(ch))
    return TRUE;
	
  if (OBJ_NEEDCLASS_FLAGGED(obj, ITEM_NEED_MONK) && !IS_MONK(ch))
    return TRUE;
	
  if (OBJ_NEEDCLASS_FLAGGED(obj, ITEM_NEED_NECROMANCER) && !IS_NECROMANCER(ch))
    return TRUE;
	
  if (OBJ_NEEDCLASS_FLAGGED(obj, ITEM_NEED_DRUID) && !IS_DRUID(ch))
    return TRUE;
	
  if (OBJ_NEEDCLASS_FLAGGED(obj, ITEM_NEED_THIEF) && !IS_THIEF(ch))
    return TRUE;
	
  if (OBJ_NEEDCLASS_FLAGGED(obj, ITEM_NEED_PSIONICIST) && !IS_PSIONICIST(ch))
    return TRUE;
	
  return FALSE;
}

/* invalid_race is used by handler.c to determine if a piece of equipment is
 * usable by a particular race, based on the ITEM_ANTI_{race} bitvectors. */
int invalid_race(struct char_data *ch, struct obj_data *obj)
{

  if (OBJ_ANTIRACE_FLAGGED(obj, ITEM_ANTI_ELF) && IS_ELF(ch))
    return TRUE;
	
  if (OBJ_ANTIRACE_FLAGGED(obj, ITEM_ANTI_DWARF) && IS_DWARF(ch))
    return TRUE;
	
  if (OBJ_ANTIRACE_FLAGGED(obj, ITEM_ANTI_HUMAN) && IS_HUMAN(ch))
    return TRUE;
	
  if (OBJ_ANTIRACE_FLAGGED(obj, ITEM_ANTI_KENDER) && IS_KENDER(ch))
    return TRUE;
	
  if (OBJ_ANTIRACE_FLAGGED(obj, ITEM_ANTI_HALF_ORC) && IS_HALF_ORC(ch))
    return TRUE;
	
  if (OBJ_ANTIRACE_FLAGGED(obj, ITEM_ANTI_HALF_OGRE) && IS_HALF_OGRE(ch))
    return TRUE;
	
  if (OBJ_ANTIRACE_FLAGGED(obj, ITEM_ANTI_BAKALI) && IS_BAKALI(ch))
    return TRUE;
	
  return FALSE;
}


/* SPELLS AND SKILLS.  This area defines which spells are assigned to which 
 * classes, and the minimum level the character must be to use the spell or 
 * skill. */
void init_spell_levels(void)
{
  /* KNIGHT */
  spell_level(SKILL_KICK, CLASS_KNIGHT, 1);
  spell_level(SKILL_RESCUE, CLASS_KNIGHT, 3);
  spell_level(SKILL_BASH, CLASS_KNIGHT, 12);
  spell_level(SPELL_ARMOR, CLASS_KNIGHT, 20);
  spell_level(SPELL_CURE_LIGHT, CLASS_KNIGHT, 25);

  /* CLERICS */
  spell_level(SPELL_CURE_LIGHT, CLASS_CLERIC, 1);
  spell_level(SPELL_ARMOR, CLASS_CLERIC, 1);
  spell_level(SPELL_CREATE_FOOD, CLASS_CLERIC, 2);
  spell_level(SPELL_CREATE_WATER, CLASS_CLERIC, 2);
  spell_level(SPELL_DETECT_POISON, CLASS_CLERIC, 3);
  spell_level(SPELL_DETECT_ALIGN, CLASS_CLERIC, 4);
  spell_level(SPELL_CURE_BLIND, CLASS_CLERIC, 4);
  spell_level(SPELL_BLESS, CLASS_CLERIC, 5);
  spell_level(SPELL_DETECT_INVIS, CLASS_CLERIC, 6);
  spell_level(SPELL_BLINDNESS, CLASS_CLERIC, 6);
  spell_level(SPELL_INFRAVISION, CLASS_CLERIC, 7);
  spell_level(SPELL_PROT_FROM_EVIL, CLASS_CLERIC, 8);
  spell_level(SPELL_POISON, CLASS_CLERIC, 8);
  spell_level(SPELL_GROUP_ARMOR, CLASS_CLERIC, 9);
  spell_level(SPELL_CURE_CRITIC, CLASS_CLERIC, 9);
  spell_level(SPELL_SUMMON, CLASS_CLERIC, 10);
  spell_level(SPELL_REMOVE_POISON, CLASS_CLERIC, 10);
  spell_level(SPELL_WORD_OF_RECALL, CLASS_CLERIC, 12);
  spell_level(SPELL_EARTHQUAKE, CLASS_CLERIC, 12);
  spell_level(SPELL_DISPEL_EVIL, CLASS_CLERIC, 14);
  spell_level(SPELL_DISPEL_GOOD, CLASS_CLERIC, 14);
  spell_level(SPELL_SANCTUARY, CLASS_CLERIC, 15);
  spell_level(SPELL_CALL_LIGHTNING, CLASS_CLERIC, 15);
  spell_level(SPELL_HEAL, CLASS_CLERIC, 16);
  spell_level(SPELL_CONTROL_WEATHER, CLASS_CLERIC, 17);
  spell_level(SPELL_SENSE_LIFE, CLASS_CLERIC, 18);
  spell_level(SPELL_HARM, CLASS_CLERIC, 19);
  spell_level(SPELL_GROUP_HEAL, CLASS_CLERIC, 22);
  spell_level(SPELL_REMOVE_CURSE, CLASS_CLERIC, 26);

  /* BARBARIAN */
  spell_level(SKILL_KICK, CLASS_BARBARIAN, 1);
  spell_level(SKILL_RESCUE, CLASS_BARBARIAN, 3);
  spell_level(SKILL_BASH, CLASS_BARBARIAN, 12);

  /* RANGER */
  spell_level(SKILL_KICK, CLASS_RANGER, 1);
  spell_level(SKILL_RESCUE, CLASS_RANGER, 3);
  spell_level(SKILL_BASH, CLASS_RANGER, 12);

  /* MAGES */
  spell_level(SPELL_MAGIC_MISSILE, CLASS_MAGE, 1);
  spell_level(SPELL_DETECT_INVIS, CLASS_MAGE, 2);
  spell_level(SPELL_DETECT_MAGIC, CLASS_MAGE, 2);
  spell_level(SPELL_CHILL_TOUCH, CLASS_MAGE, 3);
  spell_level(SPELL_INFRAVISION, CLASS_MAGE, 3);
  spell_level(SPELL_INVISIBLE, CLASS_MAGE, 4);
  spell_level(SPELL_ARMOR, CLASS_MAGE, 4);
  spell_level(SPELL_BURNING_HANDS, CLASS_MAGE, 5);
  spell_level(SPELL_LOCATE_OBJECT, CLASS_MAGE, 6);
  spell_level(SPELL_STRENGTH, CLASS_MAGE, 6);
  spell_level(SPELL_SHOCKING_GRASP, CLASS_MAGE, 7);
  spell_level(SPELL_SLEEP, CLASS_MAGE, 8);
  spell_level(SPELL_LIGHTNING_BOLT, CLASS_MAGE, 9);
  spell_level(SPELL_BLINDNESS, CLASS_MAGE, 9);
  spell_level(SPELL_DETECT_POISON, CLASS_MAGE, 10);
  spell_level(SPELL_COLOR_SPRAY, CLASS_MAGE, 11);
  spell_level(SPELL_ENERGY_DRAIN, CLASS_MAGE, 13);
  spell_level(SPELL_CURSE, CLASS_MAGE, 14);
  spell_level(SPELL_POISON, CLASS_MAGE, 14);
  spell_level(SPELL_FIREBALL, CLASS_MAGE, 15);
  spell_level(SPELL_CHARM, CLASS_MAGE, 16);
  spell_level(SPELL_SUMMON, CLASS_MAGE, 17);
  spell_level(SPELL_IDENTIFY, CLASS_MAGE, 22);
  spell_level(SPELL_FLY, CLASS_MAGE, 25);
  spell_level(SPELL_ENCHANT_WEAPON, CLASS_MAGE, 26);
  spell_level(SPELL_CLONE, CLASS_MAGE, 30);
  spell_level(SPELL_TELEPORT, CLASS_MAGE, 35);
  
  /* MONK */
  spell_level(SKILL_KICK, CLASS_MONK, 1);
  spell_level(SKILL_RESCUE, CLASS_MONK, 3);
  spell_level(SKILL_BASH, CLASS_MONK, 12);
  spell_level(SKILL_WHIRLWIND, CLASS_MONK, 20);

  /* NECROMANCER */
  spell_level(SPELL_MAGIC_MISSILE, CLASS_NECROMANCER, 1);
  spell_level(SPELL_DETECT_INVIS, CLASS_NECROMANCER, 2);
  spell_level(SPELL_CHILL_TOUCH, CLASS_NECROMANCER, 3);
  spell_level(SPELL_SLEEP, CLASS_NECROMANCER, 8);
  spell_level(SPELL_LIGHTNING_BOLT, CLASS_NECROMANCER, 9);
  spell_level(SPELL_FIREBALL, CLASS_NECROMANCER, 15);

  /* DRUID */
  spell_level(SPELL_CURE_LIGHT, CLASS_DRUID, 1);
  spell_level(SPELL_ARMOR, CLASS_DRUID, 1);
  spell_level(SPELL_CREATE_FOOD, CLASS_DRUID, 2);
  spell_level(SPELL_CREATE_WATER, CLASS_DRUID, 2);
  spell_level(SPELL_DETECT_POISON, CLASS_DRUID, 3);
  spell_level(SPELL_DETECT_ALIGN, CLASS_DRUID, 4);
  spell_level(SPELL_DETECT_INVIS, CLASS_DRUID, 6);
  spell_level(SPELL_BLINDNESS, CLASS_DRUID, 6);
  spell_level(SPELL_INFRAVISION, CLASS_DRUID, 7);
  spell_level(SPELL_POISON, CLASS_DRUID, 8);
  spell_level(SPELL_CURE_CRITIC, CLASS_DRUID, 9);
  spell_level(SPELL_REMOVE_POISON, CLASS_DRUID, 10);
  spell_level(SPELL_CALL_LIGHTNING, CLASS_DRUID, 15);
  spell_level(SPELL_HEAL, CLASS_DRUID, 16);
  spell_level(SPELL_CONTROL_WEATHER, CLASS_DRUID, 17);
  spell_level(SPELL_SENSE_LIFE, CLASS_DRUID, 18);


  /* THIEVES */
  spell_level(SKILL_SNEAK, CLASS_THIEF, 1);
  spell_level(SKILL_PICK_LOCK, CLASS_THIEF, 2);
  spell_level(SKILL_BACKSTAB, CLASS_THIEF, 3);
  spell_level(SKILL_STEAL, CLASS_THIEF, 4);
  spell_level(SKILL_HIDE, CLASS_THIEF, 5);
  spell_level(SKILL_TRACK, CLASS_THIEF, 6);

  /* PSIONICIST */
  spell_level(SPELL_CURE_LIGHT, CLASS_PSIONICIST, 1);
  spell_level(SPELL_DETECT_INVIS, CLASS_PSIONICIST, 2);
}


/* This is the exp given to implementors -- it must always be greater than the 
 * exp required for immortality, plus at least 20,000 or so. */
#define EXP_MAX  1215000000

/* Function to return the exp required for each class/level */
int level_exp(int chclass, int level)
{
  if (level > LVL_IMPL || level < 0) {
    log("SYSERR: Requesting exp for invalid level %d!", level);
    return 0;
  }

  /* Gods have exp close to EXP_MAX.  This statement should never have to
   * changed, regardless of how many mortal or immortal levels exist. */
   if (level > LVL_IMMORT) {
     return EXP_MAX - ((LVL_IMPL - level) * 1000);
   }

  /* Exp required for normal mortals is below */
  switch (chclass) {

    case CLASS_KNIGHT:
    switch (level) {
      case  0: return 0;
      case  1: return 1;
      case  2: return 2500;
      case  3: return 5250;
      case  4: return 10500;
      case  5: return 20000;
      case  6: return 30000;
      case  7: return 45000;
      case  8: return 67500;
      case  9: return 100000;
      case 10: return 140000;
      case 11: return 200000;
      case 12: return 280000;
      case 13: return 400000;
      case 14: return 550000;
      case 15: return 750000;
      case 16: return 1000000;
      case 17: return 1400000;
      case 18: return 1900000;
      case 19: return 2500000;
      case 20: return 3400000;
      case 21: return 4500000;
      case 22: return 5900000;
      case 23: return 7700000;
      case 24: return 10000000;
      case 25: return 13000000;
      case 26: return 17000000;
      case 27: return 21000000;
      case 28: return 26500000;
      case 29: return 33000000;
      case 30: return 42500000;
      case 31: return 52500000;
      case 32: return 65000000;
      case 33: return 80000000;
      case 34: return 97000000;
      case 35: return 117000000;
      case 36: return 140000000;
      case 37: return 170000000;
      case 38: return 200000000;
      case 39: return 240000000; 
      case 40: return 275000000;
      case 41: return 320000000;
      case 42: return 375000000;
      case 43: return 425000000;
      case 44: return 490000000;
      case 45: return 550000000;
      case 46: return 625000000;
      case 47: return 700000000;
      case 48: return 785000000;
      case 49: return 870000000;
      case 50: return 965000000; 
      /* add new levels here */
      case LVL_IMMORT: return 1115000000; 
    }

    break;

    case CLASS_CLERIC:
    switch (level) {
      case  0: return 0;
      case  1: return 1;
      case  2: return 2500;
      case  3: return 5250;
      case  4: return 10500;
      case  5: return 20000;
      case  6: return 30000;
      case  7: return 45000;
      case  8: return 67500;
      case  9: return 100000;
      case 10: return 140000;
      case 11: return 200000;
      case 12: return 280000;
      case 13: return 400000;
      case 14: return 550000;
      case 15: return 750000;
      case 16: return 1000000;
      case 17: return 1400000;
      case 18: return 1900000;
      case 19: return 2500000;
      case 20: return 3400000;
      case 21: return 4500000;
      case 22: return 5900000;
      case 23: return 7700000;
      case 24: return 10000000;
      case 25: return 13000000;
      case 26: return 17000000;
      case 27: return 21000000;
      case 28: return 26500000;
      case 29: return 33000000;
      case 30: return 42500000;
      case 31: return 52500000;
      case 32: return 65000000;
      case 33: return 80000000;
      case 34: return 97000000;
      case 35: return 117000000;
      case 36: return 140000000;
      case 37: return 170000000;
      case 38: return 200000000;
      case 39: return 240000000; 
      case 40: return 275000000;
      case 41: return 320000000;
      case 42: return 375000000;
      case 43: return 425000000;
      case 44: return 490000000;
      case 45: return 550000000;
      case 46: return 625000000;
      case 47: return 700000000;
      case 48: return 785000000;
      case 49: return 870000000;
      case 50: return 965000000; 
      /* add new levels here */
      case LVL_IMMORT: return 1115000000; 
    }
    break;

    case CLASS_BARBARIAN:
    switch (level) {
      case  0: return 0;
      case  1: return 1;
      case  2: return 2500;
      case  3: return 5250;
      case  4: return 10500;
      case  5: return 20000;
      case  6: return 30000;
      case  7: return 45000;
      case  8: return 67500;
      case  9: return 100000;
      case 10: return 140000;
      case 11: return 200000;
      case 12: return 280000;
      case 13: return 400000;
      case 14: return 550000;
      case 15: return 750000;
      case 16: return 1000000;
      case 17: return 1400000;
      case 18: return 1900000;
      case 19: return 2500000;
      case 20: return 3400000;
      case 21: return 4500000;
      case 22: return 5900000;
      case 23: return 7700000;
      case 24: return 10000000;
      case 25: return 13000000;
      case 26: return 17000000;
      case 27: return 21000000;
      case 28: return 26500000;
      case 29: return 33000000;
      case 30: return 42500000;
      case 31: return 52500000;
      case 32: return 65000000;
      case 33: return 80000000;
      case 34: return 97000000;
      case 35: return 117000000;
      case 36: return 140000000;
      case 37: return 170000000;
      case 38: return 200000000;
      case 39: return 240000000; 
      case 40: return 275000000;
      case 41: return 320000000;
      case 42: return 375000000;
      case 43: return 425000000;
      case 44: return 490000000;
      case 45: return 550000000;
      case 46: return 625000000;
      case 47: return 700000000;
      case 48: return 785000000;
      case 49: return 870000000;
      case 50: return 965000000; 
      /* add new levels here */
      case LVL_IMMORT: return 1115000000; 
    }
    break;

    case CLASS_RANGER:
    switch (level) {
      case  0: return 0;
      case  1: return 1;
      case  2: return 2500;
      case  3: return 5250;
      case  4: return 10500;
      case  5: return 20000;
      case  6: return 30000;
      case  7: return 45000;
      case  8: return 67500;
      case  9: return 100000;
      case 10: return 140000;
      case 11: return 200000;
      case 12: return 280000;
      case 13: return 400000;
      case 14: return 550000;
      case 15: return 750000;
      case 16: return 1000000;
      case 17: return 1400000;
      case 18: return 1900000;
      case 19: return 2500000;
      case 20: return 3400000;
      case 21: return 4500000;
      case 22: return 5900000;
      case 23: return 7700000;
      case 24: return 10000000;
      case 25: return 13000000;
      case 26: return 17000000;
      case 27: return 21000000;
      case 28: return 26500000;
      case 29: return 33000000;
      case 30: return 42500000;
      case 31: return 52500000;
      case 32: return 65000000;
      case 33: return 80000000;
      case 34: return 97000000;
      case 35: return 117000000;
      case 36: return 140000000;
      case 37: return 170000000;
      case 38: return 200000000;
      case 39: return 240000000; 
      case 40: return 275000000;
      case 41: return 320000000;
      case 42: return 375000000;
      case 43: return 425000000;
      case 44: return 490000000;
      case 45: return 550000000;
      case 46: return 625000000;
      case 47: return 700000000;
      case 48: return 785000000;
      case 49: return 870000000;
      case 50: return 965000000; 
      /* add new levels here */
      case LVL_IMMORT: return 1115000000; 
    }
    break;

    case CLASS_MAGE:
    switch (level) {
      case  0: return 0;
      case  1: return 1;
      case  2: return 2500;
      case  3: return 5250;
      case  4: return 10500;
      case  5: return 20000;
      case  6: return 30000;
      case  7: return 45000;
      case  8: return 67500;
      case  9: return 100000;
      case 10: return 140000;
      case 11: return 200000;
      case 12: return 280000;
      case 13: return 400000;
      case 14: return 550000;
      case 15: return 750000;
      case 16: return 1000000;
      case 17: return 1400000;
      case 18: return 1900000;
      case 19: return 2500000;
      case 20: return 3400000;
      case 21: return 4500000;
      case 22: return 5900000;
      case 23: return 7700000;
      case 24: return 10000000;
      case 25: return 13000000;
      case 26: return 17000000;
      case 27: return 21000000;
      case 28: return 26500000;
      case 29: return 33000000;
      case 30: return 42500000;
      case 31: return 52500000;
      case 32: return 65000000;
      case 33: return 80000000;
      case 34: return 97000000;
      case 35: return 117000000;
      case 36: return 140000000;
      case 37: return 170000000;
      case 38: return 200000000;
      case 39: return 240000000; 
      case 40: return 275000000;
      case 41: return 320000000;
      case 42: return 375000000;
      case 43: return 425000000;
      case 44: return 490000000;
      case 45: return 550000000;
      case 46: return 625000000;
      case 47: return 700000000;
      case 48: return 785000000;
      case 49: return 870000000;
      case 50: return 965000000; 
      /* add new levels here */
      case LVL_IMMORT: return 1115000000; 
    }
    break;

    case CLASS_MONK:
    switch (level) {
      case  0: return 0;
      case  1: return 1;
      case  2: return 2500;
      case  3: return 5250;
      case  4: return 10500;
      case  5: return 20000;
      case  6: return 30000;
      case  7: return 45000;
      case  8: return 67500;
      case  9: return 100000;
      case 10: return 140000;
      case 11: return 200000;
      case 12: return 280000;
      case 13: return 400000;
      case 14: return 550000;
      case 15: return 750000;
      case 16: return 1000000;
      case 17: return 1400000;
      case 18: return 1900000;
      case 19: return 2500000;
      case 20: return 3400000;
      case 21: return 4500000;
      case 22: return 5900000;
      case 23: return 7700000;
      case 24: return 10000000;
      case 25: return 13000000;
      case 26: return 17000000;
      case 27: return 21000000;
      case 28: return 26500000;
      case 29: return 33000000;
      case 30: return 42500000;
      case 31: return 52500000;
      case 32: return 65000000;
      case 33: return 80000000;
      case 34: return 97000000;
      case 35: return 117000000;
      case 36: return 140000000;
      case 37: return 170000000;
      case 38: return 200000000;
      case 39: return 240000000; 
      case 40: return 275000000;
      case 41: return 320000000;
      case 42: return 375000000;
      case 43: return 425000000;
      case 44: return 490000000;
      case 45: return 550000000;
      case 46: return 625000000;
      case 47: return 700000000;
      case 48: return 785000000;
      case 49: return 870000000;
      case 50: return 965000000; 
      /* add new levels here */
      case LVL_IMMORT: return 1115000000; 
    }
    break;

    case CLASS_NECROMANCER:
    switch (level) {
      case  0: return 0;
      case  1: return 1;
      case  2: return 2500;
      case  3: return 5250;
      case  4: return 10500;
      case  5: return 20000;
      case  6: return 30000;
      case  7: return 45000;
      case  8: return 67500;
      case  9: return 100000;
      case 10: return 140000;
      case 11: return 200000;
      case 12: return 280000;
      case 13: return 400000;
      case 14: return 550000;
      case 15: return 750000;
      case 16: return 1000000;
      case 17: return 1400000;
      case 18: return 1900000;
      case 19: return 2500000;
      case 20: return 3400000;
      case 21: return 4500000;
      case 22: return 5900000;
      case 23: return 7700000;
      case 24: return 10000000;
      case 25: return 13000000;
      case 26: return 17000000;
      case 27: return 21000000;
      case 28: return 26500000;
      case 29: return 33000000;
      case 30: return 42500000;
      case 31: return 52500000;
      case 32: return 65000000;
      case 33: return 80000000;
      case 34: return 97000000;
      case 35: return 117000000;
      case 36: return 140000000;
      case 37: return 170000000;
      case 38: return 200000000;
      case 39: return 240000000; 
      case 40: return 275000000;
      case 41: return 320000000;
      case 42: return 375000000;
      case 43: return 425000000;
      case 44: return 490000000;
      case 45: return 550000000;
      case 46: return 625000000;
      case 47: return 700000000;
      case 48: return 785000000;
      case 49: return 870000000;
      case 50: return 965000000; 
      /* add new levels here */
      case LVL_IMMORT: return 1115000000; 
    }
    break;

    case CLASS_DRUID:
    switch (level) {
      case  0: return 0;
      case  1: return 1;
      case  2: return 2500;
      case  3: return 5250;
      case  4: return 10500;
      case  5: return 20000;
      case  6: return 30000;
      case  7: return 45000;
      case  8: return 67500;
      case  9: return 100000;
      case 10: return 140000;
      case 11: return 200000;
      case 12: return 280000;
      case 13: return 400000;
      case 14: return 550000;
      case 15: return 750000;
      case 16: return 1000000;
      case 17: return 1400000;
      case 18: return 1900000;
      case 19: return 2500000;
      case 20: return 3400000;
      case 21: return 4500000;
      case 22: return 5900000;
      case 23: return 7700000;
      case 24: return 10000000;
      case 25: return 13000000;
      case 26: return 17000000;
      case 27: return 21000000;
      case 28: return 26500000;
      case 29: return 33000000;
      case 30: return 42500000;
      case 31: return 52500000;
      case 32: return 65000000;
      case 33: return 80000000;
      case 34: return 97000000;
      case 35: return 117000000;
      case 36: return 140000000;
      case 37: return 170000000;
      case 38: return 200000000;
      case 39: return 240000000; 
      case 40: return 275000000;
      case 41: return 320000000;
      case 42: return 375000000;
      case 43: return 425000000;
      case 44: return 490000000;
      case 45: return 550000000;
      case 46: return 625000000;
      case 47: return 700000000;
      case 48: return 785000000;
      case 49: return 870000000;
      case 50: return 965000000; 
      /* add new levels here */
      case LVL_IMMORT: return 1115000000; 
    }
    break;

    case CLASS_THIEF:
    switch (level) {
      case  0: return 0;
      case  1: return 1;
      case  2: return 2500;
      case  3: return 5250;
      case  4: return 10500;
      case  5: return 20000;
      case  6: return 30000;
      case  7: return 45000;
      case  8: return 67500;
      case  9: return 100000;
      case 10: return 140000;
      case 11: return 200000;
      case 12: return 280000;
      case 13: return 400000;
      case 14: return 550000;
      case 15: return 750000;
      case 16: return 1000000;
      case 17: return 1400000;
      case 18: return 1900000;
      case 19: return 2500000;
      case 20: return 3400000;
      case 21: return 4500000;
      case 22: return 5900000;
      case 23: return 7700000;
      case 24: return 10000000;
      case 25: return 13000000;
      case 26: return 17000000;
      case 27: return 21000000;
      case 28: return 26500000;
      case 29: return 33000000;
      case 30: return 42500000;
      case 31: return 52500000;
      case 32: return 65000000;
      case 33: return 80000000;
      case 34: return 97000000;
      case 35: return 117000000;
      case 36: return 140000000;
      case 37: return 170000000;
      case 38: return 200000000;
      case 39: return 240000000; 
      case 40: return 275000000;
      case 41: return 320000000;
      case 42: return 375000000;
      case 43: return 425000000;
      case 44: return 490000000;
      case 45: return 550000000;
      case 46: return 625000000;
      case 47: return 700000000;
      case 48: return 785000000;
      case 49: return 870000000;
      case 50: return 965000000; 
      /* add new levels here */
      case LVL_IMMORT: return 1115000000; 
    }
    break;

    case CLASS_PSIONICIST:
    switch (level) {
      case  0: return 0;
      case  1: return 1;
      case  2: return 2500;
      case  3: return 5250;
      case  4: return 10500;
      case  5: return 20000;
      case  6: return 30000;
      case  7: return 45000;
      case  8: return 67500;
      case  9: return 100000;
      case 10: return 140000;
      case 11: return 200000;
      case 12: return 280000;
      case 13: return 400000;
      case 14: return 550000;
      case 15: return 750000;
      case 16: return 1000000;
      case 17: return 1400000;
      case 18: return 1900000;
      case 19: return 2500000;
      case 20: return 3400000;
      case 21: return 4500000;
      case 22: return 5900000;
      case 23: return 7700000;
      case 24: return 10000000;
      case 25: return 13000000;
      case 26: return 17000000;
      case 27: return 21000000;
      case 28: return 26500000;
      case 29: return 33000000;
      case 30: return 42500000;
      case 31: return 52500000;
      case 32: return 65000000;
      case 33: return 80000000;
      case 34: return 97000000;
      case 35: return 117000000;
      case 36: return 140000000;
      case 37: return 170000000;
      case 38: return 200000000;
      case 39: return 240000000; 
      case 40: return 275000000;
      case 41: return 320000000;
      case 42: return 375000000;
      case 43: return 425000000;
      case 44: return 490000000;
      case 45: return 550000000;
      case 46: return 625000000;
      case 47: return 700000000;
      case 48: return 785000000;
      case 49: return 870000000;
      case 50: return 965000000; 
      /* add new levels here */
      case LVL_IMMORT: return 1115000000; 
    }
    break;
  }


  /* This statement should never be reached if the exp tables in this function
   * are set up properly.  If you see exp of 123456 then the tables above are
   * incomplete. */
  log("SYSERR: XP tables not set up correctly in class.c!");
  return 123456;
}

/* Default titles of male characters. */
const char *title_male(int chclass, int level)
{
  if (level <= 0 || level > LVL_IMPL)
    return "the new adventurer";
  if (level == LVL_IMMORT)
    return "the Immortal";

  return "the Classless";
}

/* Default titles of female characters. */
const char *title_female(int chclass, int level)
{
  if (level <= 0 || level > LVL_IMPL)
    return "the new adventurer";
	
  if (level == LVL_IMMORT)
    return "the Immortal";

    /* Default title for classes which do not have titles defined */
    return "the Classless";
}

