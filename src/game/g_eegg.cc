#include "g_local.hh"

//================================================================
// PATHFINDER
//================================================================

struct EEggProspect {
	qm::vec3_t location;
	float score;
	
	static bool compare(EEggProspect const & a, EEggProspect const & b) {
		return a.score < b.score;
	}
};

static constexpr size_t MAX_LOCATION_BUFFER = 200 * 1024 * 1024; // 200 MiB
static constexpr size_t MAX_PROSPECTS = MAX_LOCATION_BUFFER / sizeof(EEggProspect);

struct EEggPathfinder::PrivateData {
	EEggConcept conc;
	std::vector<EEggProspect> prospects;
	std::mutex prospect_mut;
};

EEggPathfinder::EEggPathfinder(EEggConcept const & conc) : m_data { new PrivateData } {
	m_data->conc = conc;
}

EEggPathfinder::~EEggPathfinder() = default;

static constexpr qm::vec3_t cardinal_xp {  1,  0,  0 };
static constexpr qm::vec3_t cardinal_xn { -1,  0,  0 };
static constexpr qm::vec3_t cardinal_yp {  0,  1,  0 };
static constexpr qm::vec3_t cardinal_yn {  0, -1,  0 };
static constexpr qm::vec3_t cardinal_zp {  0,  0,  1 };
static constexpr qm::vec3_t cardinal_zn {  0,  0, -1 };

static constexpr qm::vec3_t intercardinal_xpyp {  0.707,  0.707, 0 };
static constexpr qm::vec3_t intercardinal_xpyn {  0.707, -0.707, 0 };
static constexpr qm::vec3_t intercardinal_xnyp { -0.707,  0.707, 0 };
static constexpr qm::vec3_t intercardinal_xnyn { -0.707, -0.707, 0 };

uint EEggPathfinder::explore(qm::vec3_t start, uint divisions, std::chrono::high_resolution_clock::duration time_alloted) {
	
	auto deadline = std::chrono::high_resolution_clock::now() + time_alloted;
	
	auto settle = [this](qm::vec3_t const & pos, qm::vec3_t const & dir, qm::vec3_t & out) -> qm::vec3_t {
		trace_t tr {};
		qm::vec3_t origin, dest;
		dest = origin.move_along(dir, Q3_INFINITE);
		trap->Trace(&tr, pos.ptr(), m_data->conc.mins.ptr(), m_data->conc.maxs.ptr(), dest, -1, MASK_SOLID, qfalse, 0 ,0);
		out = tr.endpos;
		return tr.plane.normal;
	};
	
	auto score_location = [&](qm::vec3_t const & pos) -> float {
		
		float score = 0;
		auto dir_dist = [&](qm::vec3_t dir) -> float {
			qm::vec3_t dest = pos.move_along(dir, Q3_INFINITE);
			trace_t tr {};
			// test nearby solid surfaces, as well as lava and water (no burning or drowning players...)
			trap->Trace(&tr, pos, m_data->conc.mins.ptr(), m_data->conc.maxs.ptr(), dest, 0, MASK_PLAYERSOLID | CONTENTS_WATER | CONTENTS_LAVA, qfalse, 0 ,0);
			if (tr.startsolid) return Q3_INFINITE;
			dest = tr.endpos;
			return (dest - pos).magnitude();
		};
		
		// score ceiling
		score = dir_dist(cardinal_zp) * 3; // this one is extra penalized (heuristic)
		if (score == Q3_INFINITE) return score; // early short circuit for bad loation
		
		// score by nearby walls
		float sxp = dir_dist(cardinal_xp);
		float sxn = dir_dist(cardinal_xn);
		float syp = dir_dist(cardinal_yp);
		float syn = dir_dist(cardinal_yn);
		
		float sxd = sxp + sxn;
		float syd = syp + syn;
		
		// score uses total distances on the two horizontal axis, and the closest wall
		score += sxd;
		score += syd;
		score += sxp < sxn ? sxp : sxn;
		score += syp < syn ? syp : syn;
		
		return score;
	};
	
	size_t old_count = m_data->prospects.size();
	
	auto explore_task = [&, this](){
		qm::vec3_t origin = start;
		while (std::chrono::high_resolution_clock::now() < deadline) {
			qm::vec3_t dest = origin.move_along(qm::quat_t::random() * qm::vec3_t {0, 0, 1}, Q3_INFINITE);
			trace_t tr {};
			trap->Trace(&tr, origin, m_data->conc.mins.ptr(), m_data->conc.maxs.ptr(), dest, -1, MASK_SOLID, qfalse, 0 ,0);
			origin = qm::lerp<qm::vec3_t>(origin, tr.endpos, Q_flrand(0.4, 0.9));
			
			EEggProspect p;
			auto norm = settle(origin, cardinal_zn, p.location);
			p.score = score_location(p.location);
			
			// severely penalize by slope
			if (norm[2] != 1) {
				if (!norm[2]) 
					p.score = Q3_INFINITE;
				else
					p.score /= std::abs(norm[2] * norm[2] * norm[2]);
			}
			
			std::lock_guard lock { m_data->prospect_mut };
			if (m_data->prospects.size() >= MAX_PROSPECTS) return;
			m_data->prospects.push_back(p);
		}
	};
	
	trap->GetTaskCore()->enqueue_fill_wait(explore_task, divisions);
	return m_data->prospects.size() - old_count;
}

uint EEggPathfinder::spawn_eggs(uint egg_target) {
	std::sort(m_data->prospects.begin(), m_data->prospects.end(), EEggProspect::compare);
	
	auto create_egg = [this](qm::vec3_t const & pos){
		auto & conc = m_data->conc;
		
		gentity_t * ent = G_Spawn();
		ent->classname = conc.classname;
		ent->clipmask = MASK_PLAYERSOLID;
		
		ent->s.modelindex = G_ModelIndex(conc.model.data());
		ent->s.eType = ET_GENERAL;
		
		VectorCopy(conc.mins, ent->r.mins);
		VectorCopy(conc.maxs, ent->r.maxs);
		ent->r.contents = CONTENTS_NONE;
		
		ent->set_origin(pos.ptr());
		ent->link();
	};
	
	std::vector<EEggProspect> approved_prospects;
	
	static constexpr float social_distancing = 1024; // TODO -- concept
	
	for (EEggProspect const & p : m_data->prospects) {
		if (approved_prospects.size() == egg_target) break;
		
		// never place these
		if (p.score >= Q3_INFINITE) break;
		
		// ==== DEALBREAKING ====
		bool dealbreaker = false;
		
		// social distancing
		for (EEggProspect const & ap : approved_prospects) {
			if ((ap.location - p.location).magnitude() < social_distancing) {
				dealbreaker = true;
				break;
			}
		}
		if (dealbreaker) continue;
		
		// forbid if too close to steep ledge
		{
			float vtest_allowance = (m_data->conc.maxs[2] - m_data->conc.mins[2]) / 4;
			float xcenter = (m_data->conc.maxs[0] + m_data->conc.mins[0]) / 2;
			float ycenter = (m_data->conc.maxs[1] + m_data->conc.mins[1]) / 2;
			trace_t tr {};
			qm::vec3_t dest = p.location.move_along(cardinal_zn, Q3_INFINITE);
			qm::vec3_t hmins = { xcenter, ycenter, m_data->conc.mins[2] };
			qm::vec3_t hmaxs = { xcenter, ycenter, m_data->conc.maxs[2] };
			trap->Trace(&tr, p.location, hmins.ptr(), hmaxs.ptr(), dest, 0, MASK_PLAYERSOLID, qfalse, 0 ,0);
			if (std::abs(tr.endpos[2] - p.location[2]) > vtest_allowance) continue;
		}
		
		// don't hurt players who find this
		{
			trace_t tr {};
			trap->Trace(&tr, p.location, m_data->conc.mins.ptr(), m_data->conc.maxs.ptr(), p.location, ENTITYNUM_WORLD, CONTENTS_TRIGGER, qfalse, 0, 0);
			if (tr.startsolid && g_entities[tr.entityNum].damage) continue;
		}
		
		// ==== GOOD TO GO ====
		create_egg(p.location);
		approved_prospects.push_back(p);
	}
	
	return approved_prospects.size();
}
