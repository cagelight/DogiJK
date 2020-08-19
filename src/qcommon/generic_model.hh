#pragma once

#include "q_shared.hh"
#include "q_math2.hh"
#include <cstdint>

struct GenericModel {
	
	using Element = uint32_t;
	using Triangle = std::array<Element, 3>;
	
	struct Surface {
		int32_t ident;
		istring shader;
		int32_t shader_index;
		std::vector<Triangle> triangles;
	};
	
	istring name;
	std::vector<qm::vec3_t> verts;
	std::vector<qm::vec2_t> uvs;
	std::vector<qm::vec3_t> normals;
	std::vector<Surface> surfaces;
	qm::vec3_t mins, maxs;
};
