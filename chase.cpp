//Code for one creature to chase another
//chase.c     1.32    (A.I. Design) 12/12/84

#include <stdlib.h>

#include "rogue.h"
#include "chase.h"
#include "fight.h"
#include "move.h"
#include "io.h"
#include "sticks.h"
#include "misc.h"
#include "curses.h"
#include "main.h"
#include "monsters.h"
#include "list.h"
#include "level.h"
#include "weapons.h"
#include "scrolls.h"
#include "pack.h"
#include "room.h"
#include "game_state.h"
#include "hero.h"


//orcs should pick up gold in a room, then chase the player.
//a bug prevented orcs from picking up gold, so they'd just
//remain on the gold space.
const bool orc_bugfix = true;

#define DRAGONSHOT  5 //one chance in DRAGONSHOT that a dragon will flame

Coord ch_ret; //Where chasing takes you

//runners: Make all the running monsters move.
void runners()
{
    Agent *monster, *next = NULL;
    int dist;

    for (auto it = game->level().monsters.begin(); it != game->level().monsters.end();){
        monster = *(it++);
        if (!monster->is_held() && monster->is_running())
        {
            dist = distance(game->hero().pos, monster->pos);
            if (!(monster->is_slow() || (monster->can_divide() && dist > 3)) || monster->turn)
            if (!monster->do_chase())
                continue;
            if (monster->is_fast())
            if (!monster->do_chase())
                continue;
            dist = distance(game->hero().pos, monster->pos);
            if (monster->is_flying() && dist > 3)
            if (!monster->do_chase())
                continue;
            monster->turn ^= true;
        }
    }
}

//do_chase: Make one thing chase another.
bool Agent::do_chase()
{
  int mindist = 32767, i, dist;
  bool door;
  Item *obj;
  struct Room *oroom;
  struct Room *monster_room, *dest_room; //room of chaser, room of chasee
  Coord tempdest; //Temporary destination for chaser

  monster_room = this->room; //Find room of chaser
  if (this->is_greedy() && monster_room->gold_val == 0) 
    this->dest = &game->hero().pos; //If gold has been taken, run after hero

  dest_room = game->hero().room;
  if (this->dest != &game->hero().pos) 
    dest_room = get_room_from_position(this->dest); //Find room of chasee
  if (dest_room==NULL) 
    return true;

  //We don't count doors as inside rooms for this routine
  door = (game->level().get_tile(this->pos)==DOOR);
  //If the object of our desire is in a different room, and we are not in a maze, run to the door nearest to our goal.

over:

  if (monster_room!=dest_room && (monster_room->is_maze())==0)
  {
    //loop through doors
    for (i = 0; i<monster_room->num_exits; i++)
    {
      dist = distance(*(this->dest), monster_room->exits[i]);
      if (dist<mindist) {tempdest = monster_room->exits[i]; mindist = dist;}
    }
    if (door)
    {
      monster_room = &passages[game->level().get_passage_num(this->pos)];
      door = false;
      goto over;
    }
  }
  else
  {
    tempdest = *this->dest;
    //For monsters which can fire bolts at the poor hero, we check to see if (a) the hero is on a straight line from it, and (b) that it is within shooting distance, but outside of striking range.
    if ((this->shoots_fire() || this->shoots_ice()) && 
        (this->pos.y==game->hero().pos.y || this->pos.x==game->hero().pos.x || abs(this->pos.y-game->hero().pos.y)==abs(this->pos.x-game->hero().pos.x)) &&
        ((dist = distance(this->pos, game->hero().pos))>2 && dist<=BOLT_LENGTH*BOLT_LENGTH) && !this->powers_cancelled() && rnd(DRAGONSHOT)==0)
    {
      game->modifiers.m_running = false;
      delta.y = sign(game->hero().pos.y-this->pos.y);
      delta.x = sign(game->hero().pos.x-this->pos.x);
      return fire_bolt(&this->pos, &delta, this->shoots_fire()?"flame":"frost");
    }
  }
  //This now contains what we want to run to this time so we run to it. If we hit it we either want to fight it or stop running
  this->chase(&tempdest);
  if (equal(ch_ret, game->hero().pos)) {
    return attack(this); 
  }
  else if (equal(ch_ret, *this->dest))
  {
      for (auto it = game->level().items.begin(); it != game->level().items.end(); ){
          obj = *(it++);
          if (orc_bugfix && equal(*(this->dest), obj->pos)) //todo:why didn't old code work?
          {
              byte oldchar;
              game->level().items.remove(obj);
              this->pack.push_front(obj);
              oldchar = (this->room->is_gone()) ? PASSAGE : FLOOR;
              game->level().set_tile(obj->pos, oldchar);
              if (game->hero().can_see(obj->pos))
                  Screen::DrawChar(obj->pos, oldchar);
              set_destination();
              break;
          }
      }
  }
  if (this->is_stationary()) 
      return true;
  //If the chasing thing moved, update the screen
  if (this->oldch!=MDK)
  {
    if (this->oldch==' ' && game->hero().can_see(this->pos) && game->level().get_tile(this->pos)==FLOOR)
      Screen::DrawChar(this->pos, (char)FLOOR);
    else if (this->oldch == FLOOR && !game->hero().can_see(this->pos) && !game->hero().detects_others())
      Screen::DrawChar(this->pos, ' ');
    else
      Screen::DrawChar(this->pos, this->oldch);
  }
  oroom = this->room;
  if (!equal(ch_ret, this->pos))
  {
    if ((this->room = get_room_from_position(&ch_ret))==NULL) {
        this->room = oroom; 
        return true;
    }
    if (oroom!=this->room) 
        set_destination();
    this->pos = ch_ret;
  }
  if (game->hero().can_see_monster(this))
  {
    if (game->level().get_flags(ch_ret)&F_PASS) standout();
    this->oldch = mvinch(ch_ret.y, ch_ret.x);
    Screen::DrawChar(ch_ret, this->disguise);
  }
  else if (game->hero().detects_others())
  {
    standout();
    this->oldch = mvinch(ch_ret.y, ch_ret.x);
    Screen::DrawChar(ch_ret, this->type);
  }
  else 
    this->oldch = MDK;
  if (this->oldch==FLOOR && oroom->is_dark()) 
      this->oldch = ' ';
  standend();
  return true;
}

//can_see_monster: Return true if the hero can see the monster
bool Hero::can_see_monster(Agent *monster)
{
  // player is blind
  if (this->is_blind())
    return false;

  //monster is invisible, and can't see invisible
  if (monster->is_invisible() && !this->sees_invisible())
    return false;
  
  if (distance(monster->pos, this->pos) >= LAMP_DIST &&
    (monster->room != this->room || monster->room->is_dark() || monster->room->is_maze()))
    return false;
  
  //If we are seeing the enemy of a vorpally enchanted weapon for the first time, 
  //give the player a hint as to what that weapon is good for.
  Item* weapon = get_current_weapon();
  if (weapon && weapon->is_vorpalized_against(monster) && !weapon->did_flash())
  {
    weapon->set_flashed();
    msg(flash, get_weapon_name(weapon->which), short_msgs()?"":intense);
  }
  return true;
}

//start_run: Set a monster running after something or stop it from running (for when it dies)
void Agent::start_run()
{
  //Start the beastie running
  set_running(true);
  set_is_held(false);
  set_destination();
}

//chase: Find the spot for the chaser(er) to move closer to the chasee(ee). Returns true if we want to keep on chasing later. false if we reach the goal.
void Agent::chase(Coord *chasee_pos)
{
  int x, y;
  int dist, thisdist;
  Coord *chaser_pos;
  byte ch;
  int plcnt = 1;

  chaser_pos = &this->pos;
  //If the thing is confused, let it move randomly. Phantoms are slightly confused all of the time, and bats are quite confused all the time
  if (this->is_monster_confused_this_turn())
  {
    //get a valid random move
    rndmove(this, &ch_ret);
    dist = distance(ch_ret, *chasee_pos);
    //Small chance that it will become un-confused
    if (rnd(30)==17) 
        this->set_confused(false);
  }
  //Otherwise, find the empty spot next to the chaser that is closest to the chasee.
  else
  {
    int ey, ex;

    //This will eventually hold where we move to get closer. If we can't find an empty spot, we stay where we are.
    dist = distance(*chaser_pos, *chasee_pos);
    ch_ret = *chaser_pos;
    ey = chaser_pos->y+1;
    ex = chaser_pos->x+1;
    for (x = chaser_pos->x-1; x<=ex; x++)
    {
      for (y = chaser_pos->y-1; y<=ey; y++)
      {
        Coord try_pos;

        try_pos.x = x;
        try_pos.y = y;
        if (offmap({x,y}) || !diag_ok(chaser_pos, &try_pos)) continue;
        ch = game->level().get_tile_or_monster({x,y});
        if (step_ok(ch))
        {
          //If it is a scroll, it might be a scare monster scroll so we need to look it up to see what type it is.
            if (ch == SCROLL)
            {
                Item *obj = 0;
                for (auto it = game->level().items.begin(); it != game->level().items.end(); ++it)
                {
                    obj = *it;
                    if (equal(try_pos, obj->pos))
                        break;
                    obj = 0;
                }
                if (is_scare_monster_scroll(obj)) 
                    continue;
            }
          //If we didn't find any scrolls at this place or it wasn't a scare scroll, then this place counts
          thisdist = distance({ x, y }, *chasee_pos);
          if (thisdist<dist) {plcnt = 1; ch_ret = try_pos; dist = thisdist;}
          else if (thisdist==dist && rnd(++plcnt)==0) {ch_ret = try_pos; dist = thisdist;}
        }
      }
    }
  }
}

//diag_ok: Check to see if the move is legal if it is diagonal
int diag_ok(const Coord *sp, const Coord *ep )
{
  if (ep->x==sp->x || ep->y==sp->y) return true;
  return (step_ok(game->level().get_tile({sp->x,ep->y})) && step_ok(game->level().get_tile({ep->x,sp->y})));
}

//can_see: Returns true if the hero can see a certain coordinate.
int Hero::can_see(Coord p)
{
    if (is_blind()) 
        return false;
    //if the coordinate is close.
    if (distance(p, pos) < LAMP_DIST)
        return true;
    //if the coordinate is in the same room as the hero, and the room is lit
    return (room == get_room_from_position(&p) && !room->is_dark());
}

//find_dest: find the proper destination for the monster
Coord* Agent::find_dest()
{
    // if we're in the same room as the player, or can see the player, then we go after the player
    // if we have a chance to carry an item, we may go after an unclaimed item in the same room
    int carry_prob;
    if ((carry_prob = this->get_monster_carry_prob()) <= 0 || this->in_same_room_as(&game->hero()) || game->hero().can_see_monster(this))
        return &game->hero().pos;

    for (auto i = game->level().items.begin(); i != game->level().items.end(); ++i)
    {
        Item* obj = *i;
        if (is_scare_monster_scroll(obj))
            continue;

        if (this->in_same_room_as(obj) && rnd(100) < carry_prob)
        {
            // don't go after the same object as another monster
            Agent* monster = 0;
            for (auto m = game->level().monsters.begin(); m != game->level().monsters.end(); ++m){
                if ((*m)->is_seeking(obj)) {
                    monster = *m;
                    break;
                }
            }
            if (monster == NULL)
                return &obj->pos;
        }
    }
    return &game->hero().pos;
}

void Agent::set_destination()
{
    dest = find_dest();
}

bool Agent::in_same_room_as(Agent* other)
{
    return room == other->room;
}

bool Agent::is_seeking(Item * obj)
{
    return dest == &obj->pos;
}

bool Agent::in_same_room_as(Item * obj)
{
    return room == obj->get_room();
}
