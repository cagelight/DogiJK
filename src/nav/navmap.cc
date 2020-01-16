#include "navmap.hh"

struct NavMap::Impl {
	
	Impl() {
		
	}
	
	~Impl() {
		
	}
	
	void generate(clipMap_t const * cm) {
		
	}
	
	std::vector<qm::vec3_t> get_points() {
		std::vector<qm::vec3_t> points;
		points.emplace_back(10, 10, 10);
		points.emplace_back(20, 20, 20);
		points.emplace_back(30, 30, 30);
		return points;
	}
	
};

NavMap::NavMap() : m_impl { new Impl } {}
NavMap::~NavMap() = default;
void NavMap::generate(clipMap_t const * cm) { m_impl->generate(cm); }
std::vector<qm::vec3_t> NavMap::get_points() { return m_impl->get_points(); }
