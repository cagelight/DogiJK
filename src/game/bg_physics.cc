#include <btBulletDynamicsCommon.h>

#include "qcommon/cm_public.hh"

#include "bg_local.hh"
#include "bg_physics.hh"

struct physics_world_t::impl_t {
	
	btBroadphaseInterface * broadphase = nullptr;
	btDefaultCollisionConfiguration * config = nullptr;
	btCollisionDispatcher * dispatch = nullptr;
	btSequentialImpulseConstraintSolver * solver = nullptr;
	btDiscreteDynamicsWorld * world = nullptr;
	
	impl_t() {
		
		broadphase = new btDbvtBroadphase;
		config = new btDefaultCollisionConfiguration;
		dispatch = new btCollisionDispatcher {config};
		solver = new btSequentialImpulseConstraintSolver;
		world = new btDiscreteDynamicsWorld {dispatch, broadphase, solver, config};
	}
	
	~impl_t() {
		
		if (world) delete world;
		if (solver) delete solver;
		if (dispatch) delete dispatch;
		if (config) delete config;
		if (broadphase) delete broadphase;
	}
	
};

physics_world_t::physics_world_t() : impl { std::make_unique<impl_t>() } {
	
}

physics_world_t::~physics_world_t() {
	
}
