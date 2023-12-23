
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

#ifdef BODY_COMPONENT
static inline void __process_init_body(EntityID self_id)
{
	BODY_COMPONENT.body = phx_new();
	BODY_COMPONENT.pre_solve = NULL;
}

static inline void __process_del_body(EntityID self_id)
{
	phx_del(BODY_COMPONENT.body);
}
#endif

static inline void process_init_components(EntityID self_id)
{
	(void)self_id;
	#ifdef BODY_COMPONENT
	__process_init_body(self_id);
	#endif
}

static inline void process_del_components(EntityID self_id)
{
	(void)self_id;
	#ifdef BODY_COMPONENT
	__process_del_body(self_id);
	#endif
}

static inline void process_components(EntityID self_id)
{
	(void)self_id;
	#ifdef MOB_COMPONENT
	__process_mob(self_id);
	#endif
}
