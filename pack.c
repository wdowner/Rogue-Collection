//Routines to deal with the pack
//pack.c      1.4 (A.I. Design)       12/14/84

#include <stdio.h>

#include "rogue.h"
#include "pack.h"
#include "io.h"
#include "misc.h"
#include "curses.h"
#include "list.h"
#include "things.h"
#include "mach_dep.h"
#include "level.h"
#include "thing.h"

int group_in_inventory(type) {
  return (type==POTION || type==SCROLL || type==FOOD || type==GOLD);
}

ITEM *pack_obj(byte ch, byte *chp)
{
  ITEM *obj;
  byte och;

  for (obj = player.t_pack, och = 'a'; obj!=NULL; obj = next(obj), och++) if (ch==och) return obj;
  *chp = och;
  return NULL;
}

//add_pack: Pick up an object and add it to the pack.  If the argument is non-null use it as the linked_list pointer instead of getting it off the ground.
void add_pack(ITEM *obj, bool silent)
{
  ITEM *op, *lp;
  AGENT *mp;
  bool exact, from_floor;
  byte floor;

  if (obj==NULL)
  {
    from_floor = TRUE;
    if ((obj = find_obj(player.t_pos.y, player.t_pos.x))==NULL) return;
    floor = (player.t_room->flags&ISGONE)?PASSAGE:FLOOR;
  }
  else from_floor = FALSE;
  //Link it into the pack.  Search the pack for a object of similar type
  //if there isn't one, stuff it at the beginning, if there is, look for one
  //that is exactly the same and just increment the count if there is.
  //Food is always put at the beginning for ease of access, but it
  //is not ordered so that you can't tell good food from bad.  First check
  //to see if there is something in the same group and if there is then
  //increment the count.

  if (obj->group)
  {
    for (op = player.t_pack; op!=NULL; op = next(op))
    {
      if (op->group==obj->group)
      {
        //Put it in the pack and notify the user
        op->count += obj->count;
        if (from_floor) {
          detach_item(&lvl_obj, obj);
          mvaddch(player.t_pos.y, player.t_pos.x, floor);
          set_tile(player.t_pos.y, player.t_pos.x, floor);
        }
        discard_item(obj);
        obj = op;
        goto picked_up;
      }
    }
  }
  //Check if there is room
  if (inpack>=MAXPACK-1) {msg("you can't carry anything else"); return;}
  //Check for and deal with scare monster scrolls
  if (obj->type==SCROLL && obj->which==S_SCARE) if (obj->flags&ISFOUND)
  {
    detach_item(&lvl_obj, obj);
    mvaddch(player.t_pos.y, player.t_pos.x, floor);
    set_tile(player.t_pos.y, player.t_pos.x, floor);
    msg("the scroll turns to dust%s.", noterse(" as you pick it up"));
    return;
  }
  else obj->flags |= ISFOUND;
  inpack++;
  if (from_floor) {
    detach_item(&lvl_obj, obj);
    mvaddch(player.t_pos.y, player.t_pos.x, floor); 
    set_tile(player.t_pos.y, player.t_pos.x, floor);
  }
  //Search for an object of the same type
  exact = FALSE;
  for (op = player.t_pack; op!=NULL; op = next(op)) if (obj->type==op->type) break;
  if (op==NULL)
  {
    //Put it at the end of the pack since it is a new type
    for (op = player.t_pack; op!=NULL; op = next(op))
    {
      if (op->type!=FOOD) break;
      lp = op;
    }
  }
  else
  {
    //Search for an object which is exactly the same
    while (op->type==obj->type)
    {
      if (op->which==obj->which) {exact = TRUE; break;}
      lp = op;
      if ((op = next(op))==NULL) break;
    }
  }
  if (op==NULL)
  {
    //Didn't find an exact match, just stick it here
    if (player.t_pack==NULL) player.t_pack = obj;
    else {lp->l_next = obj; obj->l_prev = lp; obj->l_next = NULL;}
  }
  else
  {
    //If we found an exact match.  If it is a potion, food, or a scroll, increase the count, otherwise put it with its clones.
    if (exact && group_in_inventory(obj->type))
    {
      op->count++;
      discard_item(obj);
      obj = op;
      goto picked_up;
    }
    if ((obj->l_prev = prev(op))!=NULL) obj->l_prev->l_next = obj;
    else player.t_pack = obj;
    obj->l_next = op;
    op->l_prev = obj;
  }
picked_up:
  //If this was the object of something's desire, that monster will get mad and run at the hero
  for (mp = mlist; mp!=NULL; mp = next(mp))
    if (mp->t_dest && (mp->t_dest->x==obj->pos.x) && (mp->t_dest->y==obj->pos.y))
      mp->t_dest = &player.t_pos;
  if (obj->type==AMULET) {amulet = TRUE; saw_amulet = TRUE;}
  //Notify the user
  if (!silent) msg("%s%s (%c)", noterse("you now have "), inv_name(obj, TRUE), pack_char(obj));
}

//inventory: List what is in the pack
int inventory(ITEM *list, int type, char *lstr)
{
  byte ch;
  int n_objs;
  char inv_temp[MAXSTR];

  n_objs = 0;
  for (ch = 'a'; list!=NULL; ch++, list = next(list))
  {
    //Don't print this one if: the type doesn't match the type we were passed AND it isn't a callable type AND it isn't a zappable weapon
    if (type && type!=list->type && !(type==CALLABLE && (list->type==SCROLL || list->type==POTION || list->type==RING || list->type==STICK)) && !(type==WEAPON && list->type==POTION) && !(type==STICK && list->enemy && list->charges)) continue;
    n_objs++;
    sprintf(inv_temp, "%c) %%s", ch);
    add_line(lstr, inv_temp, inv_name(list, FALSE));
  }
  if (n_objs==0)
  {
    msg(type==0?"you are empty handed":"you don't have anything appropriate");
    return FALSE;
  }
  return (end_line(lstr));
}

//pick_up: Add something to characters pack.
void pick_up(byte ch)
{
  ITEM *obj;

  switch (ch)
  {
  case GOLD:
    if ((obj = find_obj(player.t_pos.y, player.t_pos.x))==NULL) return;
    money(obj->gold_value);
    detach_item(&lvl_obj, obj);
    discard_item(obj);
    player.t_room->goldval = 0;
    break;
  default:
  case ARMOR: case POTION: case FOOD: case WEAPON: case SCROLL: case AMULET: case RING: case STICK:
    add_pack(NULL, FALSE);
    break;
  }
}

//get_item: Pick something out of a pack for a purpose
ITEM *get_item(char *purpose, int type)
{
  ITEM *obj;
  byte ch;
  byte och;
  static byte lch;
  static ITEM *wasthing = NULL;
  byte gi_state; //get item sub state
  int once_only = FALSE;

  if (((!strncmp(s_menu, "sel", 3) && strcmp(purpose, "eat") && strcmp(purpose, "drop"))) || !strcmp(s_menu, "on")) once_only = TRUE;
  gi_state = again;
  if (player.t_pack==NULL) msg("you aren't carrying anything");
  else
  {
    ch = lch;
    for (;;)
    {
      //if we are doing something AGAIN, and the pack hasn't changed then don't ask just give him the same thing he got on the last command.
      if (gi_state && wasthing==pack_obj(ch, &och)) goto skip;
      if (once_only) {ch = '*'; goto skip;}
      if (!terse && !expert) addmsg("which object do you want to ");
      msg("%s? (* for list): ", purpose);
      //ignore any alt characters that may be typed
      ch = readchar();
skip:
      mpos = 0;
      gi_state = FALSE;
      once_only = FALSE;
      if (ch=='*')
      {
        if ((ch = inventory(player.t_pack, type, purpose))==0) {after = FALSE; return NULL;}
        if (ch==' ') continue;
        lch = ch;
      }
      //Give the poor player a chance to abort the command
      if (ch==ESCAPE) {after = FALSE; msg(""); return NULL;}
      if ((obj = pack_obj(ch, &och))==NULL)
      {
        ifterse("range is 'a' to '%c'","please specify a letter between 'a' and '%c'", och-1);
        continue;
      }
      else
      {
        //If you find an object reset flag because you really don't know if the object he is getting is going to change the pack.  If he detaches the thing from the pack later this flag will get set.
        if (strcmp(purpose, "identify")) {lch = ch; wasthing = obj;}
        return obj;
      }
    }
  }
  return NULL;
}

//pack_char: Return which character would address a pack object
int pack_char(ITEM *obj)
{
  ITEM *item;
  byte c;

  c = 'a';
  for (item = player.t_pack; item!=NULL; item = next(item)) if (item==obj) return c; else c++;
  return '?';
}

//money: Add or subtract gold from the pack
void money(int value)
{
  byte floor;

  floor = (player.t_room->flags&ISGONE)?PASSAGE:FLOOR;
  purse += value;
  mvaddch(player.t_pos.y, player.t_pos.x, floor);
  set_tile(player.t_pos.y, player.t_pos.x, floor);
  if (value>0) msg("you found %d gold pieces", value);
}
