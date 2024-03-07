#include "defs.h"
#include "entity.h"

void
mob_process(Entity *entity, Mob *mob) 
{
	if(mob->health > mob->health_max)
		mob->health = mob->health_max;

	if(mob->health <= 0.0)
		ent_del(entity);
}
