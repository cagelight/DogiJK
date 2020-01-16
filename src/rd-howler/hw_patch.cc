#include "hw_local.hh"

using namespace howler;

struct q3world::q3patchsubdivider {
	
	using int_t = int_fast16_t;
	static constexpr int_t MAX_GRID_SIZE = 128 + 1;
	
	q3patchsubdivider(q3patchsurface const & parent) : parent(parent) {
		memset(ctrl, 0, sizeof(ctrl));
		width = parent.width; height = parent.height;
		for ( int_t x = 0 ; x < width ; x++ ) {
			for ( int_t y = 0 ; y < height ; y++ ) {
				ctrl[y][x] = parent.verts[y*width+x];
			}
		}
	}
	
	void subdivide();
	q3worldmesh_proto_variant generate_surface();
	
private:
	q3patchsurface const & parent;
	int_t width, height;
	q3patchvert ctrl[MAX_GRID_SIZE][MAX_GRID_SIZE] {};
	float errors[2][MAX_GRID_SIZE] {};
	
	q3patchvert patchlerp(q3patchvert const & A, q3patchvert const & B) {
		q3patchvert C {};
		
		C.xyz = qm::lerp(A.xyz, B.xyz, 0.5f);
		C.uv = qm::lerp(A.uv, B.uv, 0.5f);
		C.normal = qm::lerp(A.normal, B.normal, 0.5f);
		for (auto i = 0; i < 4; i++) {
			C.lm_uvs[i] = qm::lerp(A.lm_uvs[i], B.lm_uvs[i], 0.5f);
			C.lm_colors[i] = qm::lerp(A.lm_colors[i], B.lm_colors[i], 0.5f);
		}
		assert(A.styles == B.styles);
		C.styles = A.styles;
		return C;
	}
	
	void transpose() {
		int_t y, x;
		if ( width > height ) for ( y = 0 ; y < height ; y++ ) for ( x = y + 1 ; x < width ; x++ )
			if ( x < height )
				std::swap(ctrl[x][y], ctrl[y][x]); // swap the value
			else
				ctrl[x][y] = ctrl[y][x]; // just copy
		else for ( x = 0 ; x < width ; x++ ) for ( y = x + 1 ; y < height ; y++ )
			if ( y < width )
				std::swap(ctrl[y][x], ctrl[x][y]); // swap the value
			else
				ctrl[x][y] = ctrl[y][x]; // just copy
				
		std::swap(width, height);
	}
	
	void put_points_on_curve() {
		
		for (int_t x = 0; x < width; x++) for (int_t y = 1; y < height; y += 2) {
			auto prev = patchlerp(ctrl[y][x], ctrl[y+1][x]);
			auto next = patchlerp(ctrl[y][x], ctrl[y-1][x]);
			ctrl[y][x] = patchlerp(prev, next);
		}
		
		for (int_t y = 0; y < height; y++) for (int_t x = 1; x < width; x += 2) {
			auto prev = patchlerp(ctrl[y][x], ctrl[y][x+1]);
			auto next = patchlerp(ctrl[y][x], ctrl[y][x-1]);
			ctrl[y][x] = patchlerp(prev, next);
		}
	}
};

void q3world::q3patchsubdivider::subdivide() {
	
	for (int_t dir = 0; dir < 2; dir++) {
		for (int_t x = 0 ; x + 2 < width ; x += 2) {
			
			float len_max = 0;
			for (int_t y = 0 ; y < height ; y++) {
				
				qm::vec3_t mid;
				for (auto i = 0; i < 3; i++) {
					mid[i] = (ctrl[y][x].xyz[i] + ctrl[y][x+1].xyz[i] * 2 + ctrl[y][x+2].xyz[i]) * 0.25f;
				}
				mid -= ctrl[y][x].xyz;
				qm::vec3_t dir2 = ctrl[y][x+2].xyz - ctrl[y][x].xyz;
				dir2.normalize();
				float d = qm::vec3_t::dot(mid, dir2);
				qm::vec3_t proj = dir2 * d;
				mid -= proj;
				float len = mid.magnitude_squared();
				if (len > len_max) len_max = len;
			}
			
			len_max = std::sqrt(len_max);
			if (len_max < 0.1f) {
				errors[dir][x+1] = 999;
				continue;
			}
			
			if (width + 2 > MAX_GRID_SIZE) {
				errors[dir][x+1] = 1.0f / len_max;
				continue;
			}
			
			if ( len_max <= r_patch_minsize->value ) {
				errors[dir][x+1] = 1.0f / len_max;
				continue;
			}
			
			errors[dir][x+2] = 1.0f / len_max;
			
			width += 2;
			for (int_t y = 0; y < height; y++) {
				q3patchvert prev = patchlerp(ctrl[y][x], ctrl[y][x+1]);
				q3patchvert next = patchlerp(ctrl[y][x+1], ctrl[y][x+2]);
				q3patchvert mid  = patchlerp(prev, next);
				
				for (int_t x2 = width - 1; x2 > x + 3; x2--)
					ctrl[y][x2] = ctrl[y][x2-2];
				
				ctrl[y][x+1] = prev;
				ctrl[y][x+2] = mid;
				ctrl[y][x+3] = next;
			}
			
			x -= 2;
		}
		
		transpose();
	}
	
	put_points_on_curve();
	
	for (int_t x = 1; x < width-1; x++) {
		if (errors[0][x] != 999) continue;
		for (int_t x2 = x+1; x2 < width; x2++) {
			for (int_t y = 0; y < height; y++)
				ctrl[y][x2-1] = ctrl[y][x2];
			errors[0][x2-1] = errors[0][x2];
		}
		width --;
	}
	
	for (int_t y = 1; y < height-1; y++) {
		if (errors[1][y] != 999) continue;
		for (int_t y2 = y+1; y2 < height; y2++) {
			for (int_t x = 0; x < width; x++)
				ctrl[y2-1][x] = ctrl[y2][x];
			errors[1][y2-1] = errors[1][y2];
		}
		height --;
	}
	
	/* TODO
	// flip for longest tristrips as an optimization
	// the results should be visually identical with or
	// without this step
	if ( height > width ) {
		Transpose( width, height, ctrl );
		InvertErrorTable( errorTable, width, height );
		t = width;
		width = height;
		height = t;
		InvertCtrl( width, height, ctrl );
	}
	*/
	
	// MakeMeshNormals( width, height, ctrl ); TODO
}

static qm::vec4_t protocolor(qm::vec4_t const & v) { return {v[0], v[1], v[2], v[3]}; }
static qm::vec2_t protouv(qm::vec2_t const & v) { return {v[0], v[1]}; }

q3world::q3worldmesh_proto_variant q3world::q3patchsubdivider::generate_surface() {
	
	q3worldmesh_proto_variant proto_var;
	
	if (parent.vertex_lit) {
		q3worldmesh_vertexlit_proto & proto = proto_var.emplace<q3worldmesh_vertexlit_proto>();
		
		auto vertfunc = [&](q3patchvert const & v) {
			proto.verticies.emplace_back( q3worldmesh_vertexlit::vertex_t {
				v.xyz,
				v.uv,
				v.normal,
				lightmap_color_t {
					protocolor(v.lm_colors[0]),
					protocolor(v.lm_colors[1]),
					protocolor(v.lm_colors[2]),
					protocolor(v.lm_colors[3])
				},
				v.styles
			});
		};
		
		auto idxfunc = [&](int_t x, int_t y) { proto.indicies.push_back(y * width + x); };
		
		for (int_t y = 0; y < height; y++) for (int_t x = 0; x < width; x++) {
			vertfunc(ctrl[y][x]);
		}
		
		for (int_t y = 0; y < height - 1; y++) for (int_t x = 0; x < width - 1; x++) {
			idxfunc(x + 0, y + 0);
			idxfunc(x + 0, y + 1);
			idxfunc(x + 1, y + 0);
			idxfunc(x + 1, y + 0);
			idxfunc(x + 0, y + 1);
			idxfunc(x + 1, y + 1);
		}
		
		
	} else {
		q3worldmesh_maplit_proto & proto = proto_var.emplace<q3worldmesh_maplit_proto>();
		
		auto vertfunc = [&](q3patchvert const & v) {
			proto.verticies.emplace_back( q3worldmesh_maplit::vertex_t {
				v.xyz,
				v.uv,
				v.normal,
				protocolor(v.lm_colors[0]),
				lightmap_uv_t {
					protouv(v.lm_uvs[0]),
					protouv(v.lm_uvs[1]),
					protouv(v.lm_uvs[2]),
					protouv(v.lm_uvs[3])
				},
				v.styles
			});
		};
		
		auto idxfunc = [&](int_t x, int_t y) { proto.indicies.push_back(y * width + x); };
		
		for (int_t y = 0; y < height; y++) for (int_t x = 0; x < width; x++) {
			vertfunc(ctrl[y][x]);
		}
		
		for (int_t x = 0; x < width - 1; x++) for (int_t y = 0; y < height - 1; y++) {
			idxfunc(x + 0, y + 0);
			idxfunc(x + 0, y + 1);
			idxfunc(x + 1, y + 0);
			idxfunc(x + 1, y + 0);
			idxfunc(x + 0, y + 1);
			idxfunc(x + 1, y + 1);
		}
	}
	
	return proto_var;
}

q3world::q3worldmesh_proto_variant q3world::q3patchsurface::process() {
	std::unique_ptr<q3patchsubdivider> sub { new q3patchsubdivider {*this} };
	sub->subdivide();
	return sub->generate_surface();
}
