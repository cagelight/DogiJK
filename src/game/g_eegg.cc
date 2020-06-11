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

static qm::vec3_t center_sink(qm::vec3_t const & outer_mins, qm::vec3_t const & outer_maxs, qm::vec3_t const & inner_mins, qm::vec3_t const & inner_maxs) {
	qm::vec3_t v;
	v[0] = 0; // TODO
	v[1] = 0; // TODO
	v[2] = outer_mins[2] - inner_mins[2];
	return v;
}

uint EEggPathfinder::explore(qm::vec3_t start, uint divisions, std::chrono::high_resolution_clock::duration time_alloted) {
	
	auto deadline = std::chrono::high_resolution_clock::now() + time_alloted;
	
	qm::vec3_t mins, maxs;
	qm::vec3_t loc_ofs;
	
	if (0) {
		mins = player_mins;
		maxs = player_maxs;
		loc_ofs = center_sink(player_mins, player_maxs, m_concept.mins, m_concept.maxs);
	} else {
		mins = m_concept.mins;
		maxs = m_concept.maxs;
		loc_ofs = {0, 0, 0};
	}
	
	auto settle = [&](qm::vec3_t const & pos, qm::vec3_t const & dir, qm::vec3_t & out) -> qm::vec3_t {
		trace_t tr {};
		qm::vec3_t origin, dest;
		dest = origin.move_along(dir, Q3_INFINITE);
		trap->Trace(&tr, pos.ptr(), mins, maxs, dest, -1, MASK_SOLID, qfalse, 0 ,0);
		out = tr.endpos;
		return tr.plane.normal;
	};
	
	auto score_location = [&](qm::vec3_t const & pos) -> float {
		
		m_data->locations_scored++;
		
		float score = 0;
		auto dir_dist = [&](qm::vec3_t const & dir, qm::vec3_t const & mins, qm::vec3_t const & maxs) -> float {
			qm::vec3_t dest = pos.move_along(dir, Q3_INFINITE);
			trace_t tr {};
			// test nearby solid surfaces, as well as lava and water (no burning or drowning players...)
			trap->Trace(&tr, pos, mins, maxs, dest, 0, MASK_PLAYERSOLID | CONTENTS_WATER | CONTENTS_LAVA, qfalse, 0 ,0);
			if (tr.startsolid) return Q3_INFINITE;
			dest = tr.endpos;
			return (dest - pos).magnitude();
		};
		
		auto score_axis = [&](qm::vec3_t const & axis, qm::vec3_t const & mins, qm::vec3_t const & maxs) -> float {
			float p = dir_dist( axis, mins, maxs);
			float n = dir_dist(-axis, mins, maxs);
			// score uses total distances on the two horizontal axis, and the closest wall
			return (p + n) + (p < n ? p : n);
		};
		
		// score ceiling
		score = dir_dist(qm::cardinal_zp, mins, maxs) * 3; // this one is extra penalized (heuristic)
		if (score == Q3_INFINITE) return score; // early short circuit for bad loation
		
		// score by nearby walls
		score += score_axis(qm::cardinal_xp, mins, maxs);
		score += score_axis(qm::cardinal_yp, mins, maxs);
		
		if (g_eegg_intercardinal.integer) {
			score += score_axis(qm::intercardinal_xpyp, qm::vec3_t { 0, 0, mins[2] }, qm::vec3_t { 0, 0, maxs[2] }) / 2;
			score += score_axis(qm::intercardinal_xpyn, qm::vec3_t { 0, 0, mins[2] }, qm::vec3_t { 0, 0, maxs[2] }) / 2;
		}
		
		return score;
	};
	
	size_t old_count = m_data->prospects.size();
	
	auto explore_task = [&, this](){
		qm::vec3_t origin = start;
		while (std::chrono::high_resolution_clock::now() < deadline) {
			qm::vec3_t dest = origin.move_along(qm::quat_t::random() * qm::vec3_t {0, 0, 1}, Q3_INFINITE);
			trace_t tr {};
			trap->Trace(&tr, origin, mins, maxs, dest, -1, MASK_SOLID, qfalse, 0 ,0);
			origin = qm::lerp<qm::vec3_t>(origin, tr.endpos, Q_flrand(0.4, 0.9));
			
			EEggProspect p;
			auto norm = settle(origin, qm::cardinal_zn, p.location);
			
			// reject these
			if (norm[2] <= 0)
				continue;
			
			p.score = score_location(p.location + qm::vec3_t {0, 0, 0.5});
			
			// severely penalize by slope
			if (norm[2] != 1)
				p.score /= norm[2] * norm[2] * norm[2];
			
			// reject these
			if (p.score >= Q3_INFINITE) continue;
			
			std::lock_guard lock { m_data->prospect_mut };
			if (m_data->prospects.size() >= (static_cast<size_t>(g_eegg_bufferMiB.integer) * 1024 * 1024) / sizeof(EEggProspect)) return;
			m_data->prospects.push_back(p);
		}
	};
	
	trap->GetTaskCore()->enqueue_fill_wait(explore_task, divisions);
	std::sort(m_data->prospects.begin(), m_data->prospects.end(), EEggProspect::compare);
	return m_data->prospects.size() - old_count;
}

static void HSVtoRGB( float h, float s, float v, float rgb[3] )
{
	int i;
	float f;
	float p, q, t;

	h *= 5;

	i = floor( h );
	f = h - i;

	p = v * ( 1 - s );
	q = v * ( 1 - s * f );
	t = v * ( 1 - s * ( 1 - f ) );

	switch ( i )
	{
	case 0:
		rgb[0] = v;
		rgb[1] = t;
		rgb[2] = p;
		break;
	case 1:
		rgb[0] = q;
		rgb[1] = v;
		rgb[2] = p;
		break;
	case 2:
		rgb[0] = p;
		rgb[1] = v;
		rgb[2] = t;
		break;
	case 3:
		rgb[0] = p;
		rgb[1] = q;
		rgb[2] = v;
		break;
	case 4:
		rgb[0] = t;
		rgb[1] = p;
		rgb[2] = v;
		break;
	case 5:
		rgb[0] = v;
		rgb[1] = p;
		rgb[2] = q;
		break;
	}
}

uint EEggPathfinder::spawn_eggs(uint egg_target) {
	
	auto create_egg = [this](qm::vec3_t const & pos){
		auto & conc = m_concept;
		
		gentity_t * ent = G_Spawn();
		ent->classname = conc.classname;
		ent->clipmask = MASK_SOLID;
		ent->use = conc.use;
		
		std::uniform_int_distribution<size_t> dist { 0, conc.models.size() - 1 };
		ent->s.modelindex = G_ModelIndex(conc.models[dist(qm::rng)].data());
		ent->s.eType = ET_GENERAL;
		
		if (conc.random_entity_color) {
			float rgb[3];
			HSVtoRGB(Q_flrand(0, 1), 1.0, 1.0, rgb);
			ent->s.customRGBA[0] = rgb[0] * 255;
			ent->s.customRGBA[1] = rgb[1] * 255;
			ent->s.customRGBA[2] = rgb[2] * 255;
			ent->s.customRGBA[3] = 255;
		}
		
		VectorCopy(conc.mins, ent->r.mins);
		VectorCopy(conc.maxs, ent->r.maxs);
		ent->r.contents = CONTENTS_SOLID;
		if (conc.use)
			ent->r.svFlags |= SVF_PLAYER_USABLE;
		
		ent->set_origin(pos.ptr());
		ent->link();
	};
	
	uint approved = 0;
	
	for (EEggProspect const & p : m_data->prospects) {
		if (approved == egg_target) break;
		
		// ==== DEALBREAKING ====
		bool dealbreaker = false;
		
		// social distancing
		for (auto const & [grp, ap] : m_data->approved_prospects) {
			if ((ap.location - p.location).magnitude() < (grp == m_data->spawn_group ? g_eegg_sdintra.value : g_eegg_sdinter.value)) {
				dealbreaker = true;
				break;
			}
		}
		if (dealbreaker) continue;
		
		// forbid if too close to steep ledge
		{
			float vtest_allowance = (m_concept.maxs[2] - m_concept.mins[2]) / 4;
			float xcenter = (m_concept.maxs[0] + m_concept.mins[0]) / 2;
			float ycenter = (m_concept.maxs[1] + m_concept.mins[1]) / 2;
			trace_t tr {};
			qm::vec3_t dest = p.location.move_along(qm::cardinal_zn, Q3_INFINITE);
			qm::vec3_t hmins = { xcenter, ycenter, m_concept.mins[2] };
			qm::vec3_t hmaxs = { xcenter, ycenter, m_concept.maxs[2] };
			trap->Trace(&tr, p.location, hmins.ptr(), hmaxs.ptr(), dest, 0, MASK_PLAYERSOLID, qfalse, 0 ,0);
			if (std::abs(tr.endpos[2] - p.location[2]) > vtest_allowance) continue;
		}
		
		// don't hurt players who find this
		{
			trace_t tr {};
			trap->Trace(&tr, p.location, m_concept.mins.ptr(), m_concept.maxs.ptr(), p.location, ENTITYNUM_WORLD, CONTENTS_TRIGGER, qfalse, 0, 0);
			if (tr.startsolid && g_entities[tr.entityNum].damage) continue;
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

uint EEggPathfinder::locations_scored() const {
	return m_data->locations_scored;
}

uint EEggPathfinder::locations_valid() const {
	return m_data->prospects.size();
}

uint EEggPathfinder::locations_used() const {
	return m_data->approved_prospects.size();
}
