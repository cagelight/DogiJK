#pragma once

#include "qcommon/q_shared.hh"

struct physics_world_t {
	
	physics_world_t();
	~physics_world_t();
	
private:
	
	struct impl_t;
	std::unique_ptr<impl_t> impl;
	
};
