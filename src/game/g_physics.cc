#include "g_local.hh"
#include "bg_physics.hh"

std::unique_ptr<physics_world_t> g_phys;

void G_Physics_Init() {
	
	Com_Printf("================================================\n");
	Com_Printf("INITIALIZING SERVERSIDE PHYSICS\n");
	Com_Printf("------------------------------------------------\n");
	
	g_phys = Physics_Create();
	g_phys->set_gravity(g_gravity.value);
	
	clipMap_t const * cm = reinterpret_cast<clipMap_t const *>(trap->CM_Get());
	g_phys->add_world(cm);
	Com_Printf("DONE\n================================================\n");
}

void G_Physics_Shutdown() {
	g_phys.reset();
}

void G_Physics_Frame(int time) {
	if (!g_phys) return;
	float adv = time / 1000.0f;
	if (adv <= 0) return;
	if (adv > 1) adv = 1;
	g_phys->advance( adv, g_physics_resolution.integer );
}

void G_RunPhysicsProp( gentity_t * ent ) {
	
	auto physics = ent->get_component<GEntPhysics>();
	if (ent->s.eFlags & EF_PHYSICS && physics) {
		
		qm::vec3_t new_origin = physics->object->get_origin();
		new_origin.assign_to(ent->s.origin);
		new_origin.assign_to(ent->r.currentOrigin);
		new_origin.assign_to(ent->s.pos.trBase);
		
		qm::vec3_t new_angles = physics->object->get_angles();
		new_angles.assign_to(ent->s.angles);
		new_angles.assign_to(ent->r.currentAngles);
		new_angles.assign_to(ent->s.apos.trBase);
	}
	
	ent->link();
}

void G_RunPhysicsMover( gentity_t * ent ) {
	
	auto physics = ent->get_component<GEntPhysics>();
	if (ent->s.eFlags & EF_PHYSICS && physics) {
		
		physics->object->set_origin(ent->r.currentOrigin);
		physics->object->set_angles(ent->r.currentAngles);
	}
	
	ent->link();
}
