#pragma once

#include "qcommon/q_shared.hh"
#include "qcommon/cm_public.hh"
#include "qcommon/q_math2.hh"

struct NavMap final {
	
	NavMap();
	~NavMap();
	
	void generate(clipMap_t const *);
	
	std::vector<qm::vec3_t> get_points();
	
private:
	
	struct Impl;
	std::unique_ptr<Impl> m_impl;
	
};
