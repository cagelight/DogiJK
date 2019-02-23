#pragma once

#include "qcommon/q_shared.hh"
#include "qcommon/q_math2.hh"
#include "qcommon/cm_public.hh"

struct physics_object_t {
	
	virtual ~physics_object_t() = default;
	
	virtual void set_origin( qm::vec3_t const & origin ) = 0;
	virtual qm::vec3_t get_origin() = 0;
	
protected:
	
	physics_object_t() = default;
};

using physics_object_ptr = std::shared_ptr<physics_object_t>;

struct physics_world_t {
	
	virtual ~physics_world_t() = default;
	
	virtual void advance( float time ) = 0;
	
	virtual void add_world( clipMap_t const * map ) = 0;
	
	virtual void set_gravity( float ) = 0;
	
	virtual void remove_object( physics_object_ptr object ) = 0;
	virtual physics_object_ptr add_object_obj( char const * model ) = 0;
	
	
protected:
	
	physics_world_t() = default;
};

std::unique_ptr<physics_world_t> Physics_Create();
