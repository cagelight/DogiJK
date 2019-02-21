#include "g_local.hh"

std::unique_ptr<physics_world_t> g_phys;

void G_Physics_Init() {
	
	Com_Printf("================================================\n");
	Com_Printf("INITIALIZING SERVERSIDE PHYSICS\n");
	Com_Printf("------------------------------------------------\n");
	
	g_phys = Physics_Create();
	
	clipMap_t const * cm = reinterpret_cast<clipMap_t const *>(trap->CM_Get());
	g_phys->add_world(cm);
	Com_Printf("DONE\n================================================\n");
}

void G_Physics_Shutdown() {
	g_phys.reset();
}

void G_Physics_Frame(int time) {
	if (!g_phys) return;
	g_phys->advance( time / 1000.0f );
}
