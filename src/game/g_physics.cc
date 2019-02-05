#include "g_local.hh"

std::unique_ptr<physics_world_t> g_phys;

void G_Physics_Init() {
	g_phys = std::make_unique<physics_world_t>();
}

void G_Physics_Shutdown() {
	g_phys.reset();
}
