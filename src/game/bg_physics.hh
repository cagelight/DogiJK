#pragma once

#include "qcommon/q_shared.hh"
#include "qcommon/cm_public.hh"

struct physics_world_t {
	
	virtual ~physics_world_t() = default;
	
	virtual void advance(float time) = 0;
	virtual void add_world(clipMap_t const * map) = 0;
	
protected:
	
	physics_world_t() = default;
	
};

std::unique_ptr<physics_world_t> Physics_Create();
