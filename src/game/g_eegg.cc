#include "g_local.hh"

//================================================================
// PATHFINDER
//================================================================

struct EEggProspect {
	qm::vec3_t location;
	float score;
	int thoroughcheck = 0;
	int cluster = -1;
	
	static bool compare(EEggProspect const & a, EEggProspect const & b) {
		return a.score < b.score;
	}
};

struct EEggPathfinder::PrivateData {
	uint spawn_group = 0;
	std::atomic_uint64_t locations_scored = 0;
	std::vector<std::pair<uint, EEggProspect>> approved_prospects;
	std::vector<EEggProspect> prospects;
	std::mutex prospect_mut;
};

EEggPathfinder::EEggPathfinder() : m_concept { }, m_data { new PrivateData } {}

EEggPathfinder::~EEggPathfinder() = default;

static constexpr qm::vec3_t player_mins { -16, -16, DEFAULT_MINS_2 };
static constexpr qm::vec3_t player_maxs {  16,  16, DEFAULT_MAXS_2 };

static constexpr int BASE_CLIP = MASK_PLAYERSOLID | CONTENTS_SHOTCLIP;

/*
static qm::vec3_t center_sink(qm::vec3_t const & outer_mins, qm::vec3_t const & outer_maxs, qm::vec3_t const & inner_mins, qm::vec3_t const & inner_maxs) {
	qm::vec3_t v;
	v[0] = 0; // TODO
	v[1] = 0; // TODO
	v[2] = outer_mins[2] - inner_mins[2];
	return v;
}
*/

static bool PointInBox(qm::vec3_t const & point, qm::ivec3_t const & mins, qm::ivec3_t const & maxs) {
	if (point[0] < mins[0] || point[0] > maxs[0]) return false;
	if (point[1] < mins[1] || point[1] > maxs[1]) return false;
	if (point[2] < mins[2] || point[2] > maxs[2]) return false;
	return true;
}

uint EEggPathfinder::explore(qm::vec3_t start, uint divisions, std::chrono::high_resolution_clock::duration time_alloted) {
	
	auto deadline = std::chrono::high_resolution_clock::now() + time_alloted;
	
	qm::vec3_t mins, maxs;
	qm::vec3_t loc_ofs;
	
	mins = m_concept.mins;
	maxs = m_concept.maxs;
	loc_ofs = {0, 0, 0};
	
	BSP::Reader bspr = trap->CM_Read();
	
	auto cluster_of_pos = [&](qm::vec3_t const & pos) -> int32_t {
		
		BSP::Node const * iter = &bspr.nodes()[0], * niter = nullptr, * liter = nullptr;
		BSP::Leaf const * leaf = nullptr;
		
		if (!PointInBox(pos, iter->mins, iter->maxs))
			return -1;
		
		while (true) {
			
			liter = iter;
			
			for (int i = 0; i < 2; i++) {
				int c = iter->children[i];
				if (c >= 0) {
					niter = &bspr.nodes()[c];
					if (PointInBox(pos, niter->mins, niter->maxs)) {
						iter = niter;
						break;
					}
				} else {
					leaf = &bspr.leafs()[-(c + 1)];
					if (PointInBox(pos, leaf->mins, leaf->maxs))
						break;
					else
						leaf = nullptr;
				}
			}
			
			if (leaf) break;
			if (liter == iter) return -1;
		}
		
		return leaf->cluster;
		
	};
	
	auto settle = [&](qm::vec3_t const & pos, qm::vec3_t const & dir, qm::vec3_t & out) -> qm::vec3_t {
		trace_t tr {};
		qm::vec3_t dest;
		dest = pos.move_along(dir, Q3_INFINITE);
		trap->Trace(&tr, pos.ptr(), mins, maxs, dest, -1, BASE_CLIP, qfalse, 0, 0);
		out = tr.endpos;
		return tr.plane.normal;
	};
	
	auto score_location = [&](qm::vec3_t const & pos) -> float {
		
		m_data->locations_scored++;
		
		qm::vec3_t test_mins = mins / 2;
		qm::vec3_t test_maxs = maxs / 2;
		
		float score = 0;
		int sky_hits = 0;
		auto dir_dist = [&](qm::vec3_t const & dir, qm::vec3_t const & mins, qm::vec3_t const & maxs, int forbidden_contents = 0) -> float {
			qm::vec3_t dest = pos.move_along(dir, Q3_INFINITE);
			trace_t tr {};
			trap->Trace(&tr, pos, mins, maxs, dest, ENTITYNUM_NONE, BASE_CLIP | CONTENTS_WATER, qfalse, 0, 0);
			
			// stay out of the map innards >:(
			if (tr.brushside) {
				auto cm = (clipMap_t const *)trap->CM_Get();
				if (!Q_stricmp(cm->shaders[tr.brushside->shaderNum].GetName(), "textures/common/caulk")) return Q3_INFINITE;
				if (!Q_stricmp(cm->shaders[tr.brushside->shaderNum].GetName(), "textures/system/caulk")) return Q3_INFINITE;
				if (!Q_stricmp(cm->shaders[tr.brushside->shaderNum].GetName(), "textures/system/clip")) return Q3_INFINITE;
				if (!Q_stricmp(cm->shaders[tr.brushside->shaderNum].GetName(), "textures/system/physics_clip")) return Q3_INFINITE;
				if (cm->shaders[tr.brushside->shaderNum].contentFlags & forbidden_contents) return Q3_INFINITE;
			}
			
			// probably not supposed to be here (unless testing upwards)
			if (tr.surfaceFlags & SURF_SKY) sky_hits++;
			
			
			dest = tr.endpos;
			return (dest - pos).magnitude();
		};
		
		auto test_position = [&]() -> bool {
			trace_t tr {};
			// test nearby solid surfaces, as well as lava and water (no burning or drowning players...)
			trap->Trace(&tr, pos, mins, maxs, pos, ENTITYNUM_NONE, BASE_CLIP | CONTENTS_WATER | CONTENTS_LAVA | CONTENTS_TRIGGER, qfalse, 0, 0);
			
			// somehow ended up inside the world geometry? definitely not valid.
			if (tr.startsolid || tr.allsolid) return false;
			
			return true;
		};
		
		auto score_axis = [&](qm::vec3_t const & axis, qm::vec3_t const & mins, qm::vec3_t const & maxs) -> float {
			bool sky_hit;
			float p = dir_dist( axis, mins, maxs);
			float n = dir_dist(-axis, mins, maxs);
			// score uses total distances on the two horizontal axis, and the closest wall
			return (p + n) + pow(p < n ? p : n, 3);
		};
		
		if (!test_position()) return Q3_INFINITE;
		
		// score ceiling
		score = dir_dist(qm::cardinal_zp, test_mins, test_maxs, CONTENTS_TRIGGER | CONTENTS_LAVA) * 3; // this one is extra penalized (heuristic), also overhead triggers are hella sus, and overhead lava seems like a bad idea
		if (score >= Q3_INFINITE) return Q3_INFINITE; // early short circuit for bad loation
		
		// reset to 0 to ignore vertical sky hit
		sky_hits = 0;
		
		// score by nearby walls
		score += score_axis(qm::cardinal_xp, test_mins, test_maxs);
		score += score_axis(qm::cardinal_yp, test_mins, test_maxs);
		
		// only 1 horizontal sky hit allowed, visibility heuristic
		if (sky_hits > 1) return Q3_INFINITE;
		
		if (g_eegg_intercardinal.integer) {
			score += score_axis(qm::intercardinal_xpyp, test_mins, test_maxs);
			score += score_axis(qm::intercardinal_xpyn, test_mins, test_maxs);
		}
		
		return score;
	};
	
	// ========
	
	constexpr size_t STUCK_MAX = 32; // TODO: CVAR
	
	struct explore_data {
		qm::vec3_t origin;
		uint32_t stuck_counter;
	};
	
	auto explore_step = [&, this](explore_data & ex){
		qm::vec3_t dest = ex.origin.move_along(qm::quat_t::random() * qm::vec3_t {0, 0, 1}, Q3_INFINITE);
		trace_t tr {};
		trap->Trace(&tr, ex.origin, mins, maxs, dest, -1, BASE_CLIP, qfalse, 0 ,0);
		if (tr.startsolid || tr.allsolid || ex.stuck_counter >= STUCK_MAX) {
			ex.stuck_counter = 0;
			ex.origin = start;
			return;
		}
		
		dest = tr.endpos;
		if (std::abs((dest - ex.origin).magnitude()) < 32) {
			// not enough room to move
			ex.stuck_counter++;
			return;
		}
		
		ex.stuck_counter = 0;
		ex.origin = qm::lerp<qm::vec3_t>(ex.origin, tr.endpos, Q_flrand(0.4, 0.9));
		
		EEggProspect p;
		auto norm = settle(ex.origin, qm::cardinal_zn, p.location);
		
		// reject these
		if (norm[2] <= 0)
			return;
		
		p.score = score_location(p.location + qm::vec3_t {0, 0, 0.5});
		
		// severely penalize by slope
		if (norm[2] != 1)
			p.score /= std::pow(norm[2], 3);
		
		
		// reject these
		if (p.score >= Q3_INFINITE) return;
		
		p.cluster = cluster_of_pos(p.location);
		
		// reject these
		if (p.cluster < 0) return;
		
		std::lock_guard lock { m_data->prospect_mut };
		if (m_data->prospects.size() >= (static_cast<size_t>(g_eegg_bufferMiB.integer) * 1024 * 1024) / sizeof(EEggProspect)) return; // FIXME -- breakout
		m_data->prospects.push_back(p);
	};
	
	auto explore_task = [&, this](){
		std::vector<explore_data> ex;
		for (int i = 0; i < (g_eegg_branches.integer < 1 ? 1 : g_eegg_branches.integer); i++)
			ex.emplace_back(start, 0);
		
		while (std::chrono::high_resolution_clock::now() < deadline) 
			for (auto & exp : ex)
				explore_step(exp);
	};
	
	size_t old_count = m_data->prospects.size();
	
	trap->GetTaskCore()->enqueue_fill_wait(explore_task, divisions);
	if (m_data->prospects.size() >= (static_cast<size_t>(g_eegg_bufferMiB.integer) * 1024 * 1024) / sizeof(EEggProspect)) {
		Com_Printf( S_COLOR_ORANGE "WARNING: eegg buffer limit reached, cull the list and try again or increase g_eegg_bufferMiB.\n");
	}
	
	std::sort(m_data->prospects.begin(), m_data->prospects.end(), EEggProspect::compare);
	return m_data->prospects.size() - old_count;
}

uint EEggPathfinder::spawn_eggs(uint egg_target) {
	
	BSP::Reader bspr = trap->CM_Read();
	
	auto create_egg = [this](qm::vec3_t const & pos){
		auto & conc = m_concept;
		
		gentity_t * ent = G_Spawn();
		ent->classname = conc.classname;
		ent->clipmask = MASK_SOLID;
		ent->use = conc.use;
		ent->pain = conc.pain;
		
		ent->s.iModelScale = conc.modelscalepercent;
		
		std::uniform_int_distribution<size_t> dist { 0, conc.models.size() - 1 };
		ent->s.modelindex = G_ModelIndex(conc.models[dist(qm::rng)].data());
		ent->s.eType = ET_GENERAL;
		
		if (conc.random_entity_color) {
			auto color = HSLtoRGB(Q_flrand(0.0f, 1.0f), 1.0f, Q_flrand(0.5f, 0.8f));
			ent->s.customRGBA[0] = color[0] * 255;
			ent->s.customRGBA[1] = color[1] * 255;
			ent->s.customRGBA[2] = color[2] * 255;
			ent->s.customRGBA[3] = 255;
		}
		
		VectorCopy(conc.mins, ent->r.mins);
		VectorCopy(conc.maxs, ent->r.maxs);
		ent->r.contents = CONTENTS_SOLID;
		
		if (conc.use)
			ent->r.svFlags |= SVF_PLAYER_USABLE;
		
		if (conc.pain) {
			ent->takedamage = true;
			ent->maxHealth = ent->health = 100;
		}
		
		ent->set_origin(pos.ptr());
		ent->link();
	};
	
	uint approved = 0;
	
	for (EEggProspect & p : m_data->prospects) {
		if (approved == egg_target) break;
		
		if (p.thoroughcheck < 0) continue;
		
		// social distancing
		bool sdcancel = false;
		for (auto const & [grp, ap] : m_data->approved_prospects) {
			if ((ap.location - p.location).magnitude() < (grp == m_data->spawn_group ? g_eegg_sdintra.value : g_eegg_sdinter.value)) {
				sdcancel = true;
				break;
			}
		}
		if (sdcancel) continue;
		
		// sight check
		if (g_eegg_sightcheck.integer) {
			bool ok = true;
			for (auto const & [grp, ap] : m_data->approved_prospects) {
				if (grp != m_data->spawn_group) continue;
				trace_t tr {};
				trap->Trace(&tr, p.location, nullptr, nullptr, ap.location, -1, BASE_CLIP, qfalse, 0, 0);
				if ((ap.location - (qm::vec3_t)tr.endpos).magnitude() < 0.1) {
					ok = false;
					break;
				}
			}
			if (!ok) continue;
		}
		
		// forbid if too close to steep ledge
		if (!p.thoroughcheck) {
			float vtest_allowance = (m_concept.maxs[2] - m_concept.mins[2]) / 4;
			float xcenter = (m_concept.maxs[0] + m_concept.mins[0]) / 2;
			float ycenter = (m_concept.maxs[1] + m_concept.mins[1]) / 2;
			trace_t tr {};
			qm::vec3_t dest = p.location.move_along(qm::cardinal_zn, Q3_INFINITE);
			qm::vec3_t hmins = { xcenter, ycenter, m_concept.mins[2] };
			qm::vec3_t hmaxs = { xcenter, ycenter, m_concept.maxs[2] };
			trap->Trace(&tr, p.location, hmins.ptr(), hmaxs.ptr(), dest, -1, BASE_CLIP, qfalse, 0 ,0);
			
			p.thoroughcheck = (std::abs(tr.endpos[2] - p.location[2]) > vtest_allowance) ? -1 : 1;
			if (p.thoroughcheck < 0) continue;
		}
		
		/*
		// don't hurt players who find this
		if (!p.thoroughcheck) {
			trace_t tr {};
			trap->Trace(&tr, p.location, m_concept.mins.ptr(), m_concept.maxs.ptr(), p.location, ENTITYNUM_WORLD, CONTENTS_TRIGGER, qfalse, 0, 0);
			p.thoroughcheck = (tr.startsolid && g_entities[tr.entityNum].damage) ? -1 : 1;
			if (p.thoroughcheck < 0) continue;
		}
		*/
		
		// anti-patch bullshit
		auto boomerang = [&](qm::vec3_t const & dir, vec3_t const mins, vec3_t const maxs) -> bool {
			qm::vec3_t dest = p.location.move_along(dir, Q3_INFINITE);
			trace_t tr {};
			trap->Trace(&tr, p.location, nullptr, nullptr, dest, ENTITYNUM_NONE, BASE_CLIP, qfalse, 0, 0);
			if (tr.startsolid || tr.allsolid) return false;
			dest = tr.endpos;
			tr = {};
			trap->Trace(&tr, dest, nullptr, nullptr, p.location, ENTITYNUM_NONE, BASE_CLIP, qfalse, 0, 0);
			dest = tr.endpos;
			return std::abs((dest - p.location).magnitude()) < 0.1;
		};
		
		auto boomerall = [&](vec3_t const mins, vec3_t const maxs) -> bool {
			return boomerang(qm::cardinal_xn, mins, maxs) &&
			       boomerang(qm::cardinal_xp, mins, maxs) &&
		           boomerang(qm::cardinal_yn, mins, maxs) &&
		           boomerang(qm::cardinal_yp, mins, maxs) &&
		           boomerang(qm::cardinal_zn, mins, maxs) &&
		           boomerang(qm::cardinal_zp, mins, maxs);
		};
		
		if (!p.thoroughcheck && (!boomerall(nullptr, nullptr) || !boomerall(m_concept.mins, m_concept.maxs))) {
			p.thoroughcheck = -1;
			continue;
		} else p.thoroughcheck = 1;
		
		if (g_eegg_vischeck.integer && bspr.has_visibility()) {
			
			BSP::Reader::Visibility vis = bspr.visibility();
			auto clus = vis.cluster(p.cluster);
			bool valid = true;
			
			for (auto const & [grp, ap] : m_data->approved_prospects) {
				if (grp != m_data->spawn_group) continue;
				if (clus.can_see(ap.cluster)) {
					valid = false;
					break;
				}
			}
			if (!valid) continue;
		}
		
		// ==== GOOD TO GO ====
		GTaskType task { std::bind(create_egg, p.location) };
		G_Task_Enqueue(std::move(task));
		m_data->approved_prospects.emplace_back(m_data->spawn_group, p);
		approved++;
	}
	
	m_data->spawn_group++;
	return approved;
}

void EEggPathfinder::forget()  {
	m_data->approved_prospects.clear();
} 

size_t EEggPathfinder::buffer_usage() const {
	return m_data->prospects.size() * sizeof(EEggProspect);
}

void EEggPathfinder::cull() {
	
	std::vector<EEggProspect> new_prospects;
	new_prospects.reserve(m_data->prospects.size());
	
	float compval = std::pow(g_eegg_sdinter.value, 2);
	
	for (auto const & prosp : m_data->prospects) {
		bool ok = true;
		for (auto const & comp : new_prospects) {
			if ((prosp.location - comp.location).magnitude_squared() < compval) {
				ok = false;
				break;
			}
		}
		if (ok) new_prospects.push_back(prosp);
	}
	
	m_data->prospects = std::move(new_prospects);
	m_data->prospects.shrink_to_fit();
}

void EEggPathfinder::shrink(double x) {
	size_t max_eggs = (x / 100) * (g_eegg_bufferMiB.value * 1024 * 1024);
	max_eggs /= sizeof(EEggProspect);
	if (m_data->prospects.size() <= max_eggs)
		return;
	m_data->prospects.resize(max_eggs);
}

uint EEggPathfinder::locations_scored() const {
	return m_data->locations_scored;
}

uint EEggPathfinder::locations_valid() const {
	return m_data->prospects.size();
}

uint EEggPathfinder::locations_used() const {
	return m_data->approved_prospects.size();
}
