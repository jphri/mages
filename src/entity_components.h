
#ifdef MOB_COMPONENT
static inline void __process_mob(EntityID self_id)
{
	if(MOB_COMPONENT.health <= 0) {
		ent_del(self_id);
		return;
	}
	
	if(MOB_COMPONENT.health > MOB_COMPONENT.health_max) {
		MOB_COMPONENT.health = MOB_COMPONENT.health_max;
	}
}
#endif

static inline void process_components(EntityID self_id)
{
	#ifdef MOB_COMPONENT
	__process_mob(self_id);
	#endif
}