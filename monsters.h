const char* get_monster_name(char monster);
int get_monster_carry_prob(char monster);

//randmonster: Pick a monster to show up.  The lower the level, the meaner the monster.
char randmonster(bool wander);

//new_monster: Pick a new monster and add it to the list
void new_monster(AGENT *monster, byte type, Coord *cp);

//f_restor(): restor initial damage string for flytraps
void f_restor();

//expadd: Experience to add for this monster's level/hit points
int exp_add(AGENT *monster);

//wanderer: Create a new wandering monster and aim it at the player
void wanderer();

//wake_monster: What to do when the hero steps next to a monster
AGENT *wake_monster(int y, int x);

//give_pack: Give a pack to a monster if it deserves one
void give_pack(AGENT *monster);

//pick_mons: Choose a sort of monster for the enemy of a vorpally enchanted weapon
char pick_monster();

//moat(x,y): returns pointer to monster at coordinate. if no monster there return NULL
AGENT *monster_at(int y, int x);
