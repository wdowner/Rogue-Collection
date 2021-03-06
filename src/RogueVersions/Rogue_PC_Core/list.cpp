//Functions for dealing with linked lists of goodies
//Functions with names starting with an "_" have companion #defines in random.h which take the address of the first argument and pass it on.
//list.c      1.4 (A.I. Design) 12/5/85

#include "random.h"
#include "list.h"
#include "io.h"
#include "misc.h"

void free_item_list(std::list<Item*>& l)
{
    for (auto it = l.begin(); it != l.end(); ++it) {
        delete(*it);
    }
    l.clear();
}

//_free_list: Throw the whole blamed thing away
void free_agent_list(std::list<Monster*>& l)
{
    for (auto it = l.begin(); it != l.end(); ++it) {
        delete(*it);
    }
    l.clear();
}
