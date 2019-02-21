#include <btBulletDynamicsCommon.h>

#include "bg_local.hh"
#include "bg_physics.hh"

#include "qcommon/q_math2.hh"

static qm::vec3_t plane_intersection(cplane_t const & a, cplane_t const & b, cplane_t const & c) {
	matrix3_t sysmat, sysmatInverse;
	vec3_t dists;
	
	sysmat[0][0] = a.normal[0];
	sysmat[0][1] = b.normal[0];
	sysmat[0][2] = c.normal[0];
	sysmat[1][0] = a.normal[1];
	sysmat[1][1] = b.normal[1];
	sysmat[1][2] = c.normal[1];
	sysmat[2][0] = a.normal[2];
	sysmat[2][1] = b.normal[2];
	sysmat[2][2] = c.normal[2];
	
	qm::vec3_t out;

	MatrixInverse(sysmat, sysmatInverse);
	VectorSet(dists, a.dist, b.dist, c.dist);
	MatrixVectorMultiply(sysmatInverse, dists, out.ptr());
	
	return out;
}

static inline bool pointcheck(float f) {
	return !(std::isnan(f) || std::isinf(f));
}

static std::vector<qm::vec3_t> calculate_brush_hull_points(cbrush_t const & brush) {
	std::vector<qm::vec3_t> hull;
	
	for (size_t i = 0; i < brush.numsides; i++) 
	for (size_t j = 0; j < i; j++)
	for (size_t k = 0; k < j; k++) {
		if (i == j || j == k || i == k) continue;
		qm::vec3_t intersect = plane_intersection(*brush.sides[0].plane, *brush.sides[1].plane, *brush.sides[2].plane);
		if (!std::all_of(intersect.begin(), intersect.end(), pointcheck)) continue;
		
		bool legal = true;
		for (size_t l = 0; l < brush.numsides; l++) {
			if ( l == i || l == j || l == k ) { legal = false; break; }
			if ( intersect.dot( brush.sides[l].plane->normal ) > brush.sides[l].plane->dist + 0.01f ) { legal = false; break; }
		}
		if (!legal) continue;
		
		hull.emplace_back(std::move(intersect));
	}
	
	return hull;
}

#include <unordered_set>

struct submodel_t {
	
	clipMap_t const * map;
	
	std::unordered_set<cbrush_t *> brushes;
	std::unordered_set<cPatch_t *> patches;
	
	submodel_t ( clipMap_t const * map, int index ) : map { map } {
		
		cmodel_t const & smod = map->cmodels[index];
		
		if ( smod.firstNode < 0 ) {
			for ( int i = 0; i < smod.leaf.numLeafBrushes; i++ )
				brushes.insert( &map->brushes[ map->leafbrushes[ smod.leaf.firstLeafBrush + i ]]);
			for ( int i = 0; i < smod.leaf.numLeafSurfaces; i++ )
				patches.insert( map->surfaces[ map->leafsurfaces[ smod.leaf.firstLeafSurface + i ]]);
		} else {
			submodel_recurse( map->nodes[ smod.firstNode ]);
		}
		
	}
	
private:
	
	void submodel_recurse ( cNode_t const & node ) {
		
		if (node.children[0] < 0) {
			cLeaf_t const & leaf = map->leafs[-node.children[0] - 1];
			for ( int i = 0; i < leaf.numLeafBrushes; i++ )
				brushes.insert( &map->brushes[ map->leafbrushes[ leaf.firstLeafBrush + i ]]);
			for ( int i = 0; i < leaf.numLeafSurfaces; i++ )
				patches.insert( map->surfaces[ map->leafsurfaces[ leaf.firstLeafSurface + i ]]);
		} else submodel_recurse( map->nodes[ node.children[0] ] );
		
		if (node.children[1] < 0) {
			cLeaf_t const & leaf = map->leafs[-node.children[1] - 1];
			for ( int i = 0; i < leaf.numLeafBrushes; i++ )
				brushes.insert( &map->brushes[ map->leafbrushes[ leaf.firstLeafBrush + i ]]);
			for ( int i = 0; i < leaf.numLeafSurfaces; i++ )
				patches.insert( map->surfaces[ map->leafsurfaces[ leaf.firstLeafSurface + i ]]);
		} else submodel_recurse( map->nodes[ node.children[1] ] );
		
	}
	
};

struct bullet_world_t : public physics_world_t {
	
	struct world_data_t {
		
		btBroadphaseInterface * broadphase = nullptr;
		btDefaultCollisionConfiguration * config = nullptr;
		btCollisionDispatcher * dispatch = nullptr;
		btSequentialImpulseConstraintSolver * solver = nullptr;
		btDiscreteDynamicsWorld * world = nullptr;
		
		world_data_t() {
			broadphase = new btDbvtBroadphase;
			config = new btDefaultCollisionConfiguration;
			dispatch = new btCollisionDispatcher {config};
			solver = new btSequentialImpulseConstraintSolver;
			world = new btDiscreteDynamicsWorld {dispatch, broadphase, solver, config};
		}
		
		~world_data_t() {
			if (world) delete world;
			if (solver) delete solver;
			if (dispatch) delete dispatch;
			if (config) delete config;
			if (broadphase) delete broadphase;
		}
	};
	
	using world_data_ptr = std::shared_ptr<world_data_t>;
	
	struct world_shape_t {
		
		world_data_ptr parent;
		
		btCompoundShape * shape = nullptr;
		btMotionState * motion = nullptr;
		btRigidBody * body = nullptr;
		std::vector<btConvexHullShape *> components;
		
		world_shape_t(world_data_ptr parent, clipMap_t const * map) : parent { parent } {
			
			btVector3 inertia;
			
			shape = new btCompoundShape;
			submodel_t subm {map, 0};
			
			for (cbrush_t * brush : subm.brushes) {
				if (!(brush->contents & CONTENTS_SOLID)) continue;
				
				std::vector<qm::vec3_t> points = calculate_brush_hull_points(*brush);
				btConvexHullShape * brush_shape = new btConvexHullShape;
				components.push_back(brush_shape);
				
				for (qm::vec3_t const & vec : points) brush_shape->addPoint({vec[0], vec[1],vec[2]}, false);
				brush_shape->recalcLocalAabb();
				shape->addChildShape( btTransform { btQuaternion {0, 0, 0, 1}, btVector3 {0, 0, 0} }, brush_shape );
				shape->recalculateLocalAabb();
			}
			
			shape->calculateLocalInertia( 0, inertia );
			
			motion = new btDefaultMotionState { btTransform { btQuaternion {0, 0, 0, 1}, btVector3 {0, 0, 0} } };
			body = new btRigidBody {{ 0, motion, shape, inertia }};
			
			parent->world->addRigidBody(body);
		}
		
		~world_shape_t() {
			
			if (body) parent->world->removeRigidBody(body);
			
			if (shape) delete shape;
			if (motion) delete motion;
			if (body) delete body;
			for (btConvexHullShape * chs : components) {
				if (chs) delete chs;
			}
		}
	};
	
	std::shared_ptr<world_data_t> world_data;
	std::unique_ptr<world_shape_t> world_solid;
	float resolution = 120;
	
	bullet_world_t() {
		world_data = std::make_shared<world_data_t>();
	}
	
	~bullet_world_t() {
		
	}
	
	void advance( float time ) override {
		world_data->world->stepSimulation( time, resolution, 1.0f / resolution );
	}
	
	void add_world( clipMap_t const * map ) override {
		world_solid = std::make_unique<world_shape_t>( world_data, map );
	}
};

std::unique_ptr<physics_world_t> Physics_Create() {
	return std::make_unique<bullet_world_t>();
}
