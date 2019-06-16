#include "hw_local.hh"
using namespace howler;

template <typename T, void(*UPLOAD_FUNC)(T const & v)>
struct state_object {
	
	constexpr state_object(T const & default_value) : m_default(default_value), m_value(m_default) {}
	
	constexpr T const & value() const { return m_value; }
	constexpr T const & default_value() const { return m_default; }
	
	constexpr void upload() {
		UPLOAD_FUNC(m_value);
	}
	
	constexpr void reset() { 
		m_value = m_default;
		upload();
	}
	
	// return true if value changed
	constexpr void set(T const & new_value) { 
		if (new_value != m_value) {
			m_value = new_value; 
			upload();
		}
	}
	
	constexpr bool operator == (T const & other) const { return m_value == other; }
	
private:
	T const m_default;
	T m_value;
};

//================================
// ALPHA
//================================

inline static void alpha_test_enabled_func(bool const & v) { v ? glEnable(GL_ALPHA_TEST) : glDisable(GL_ALPHA_TEST); }
static state_object<bool, alpha_test_enabled_func> alpha_test_enabled_state {false};

using alpha_func_pair = std::pair<GLenum, float>;
inline static void alpha_func_func(alpha_func_pair const & v) { glAlphaFunc(v.first, v.second); }
static state_object<alpha_func_pair, alpha_func_func> alpha_func_state {{GL_GREATER, 0.0f}};

void gl::alpha_test(bool v) {
	alpha_test_enabled_state.set(v);
}

void gl::alpha_func(GLenum func, float value) {
	alpha_func_state.set({func, value});
}

//================================
// BLENDING
//================================

inline static void blend_enabled_func(bool const & v) { v ? glEnable(GL_BLEND) : glDisable(GL_BLEND); }
static state_object<bool, blend_enabled_func> blend_enabled {false};

using blendfunc_pair = std::pair<GLenum, GLenum>;
inline static void blendfunc_func(blendfunc_pair const & v) { glBlendFunc(v.first, v.second); }
static state_object<blendfunc_pair, blendfunc_func> blendfunc {{GL_ONE, GL_ZERO}};

void gl::blend(bool v) {
	blend_enabled.set(v);
}

void gl::blend(GLenum src, GLenum dst) {
	blendfunc.set({src, dst});
}

//================================
// CULL
//================================

inline static void cull_enabled_func(bool const & v) { v ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE); }
static state_object<bool, cull_enabled_func> cull_enabled_obj {false};

inline static void cull_face_func(GLenum const & v) { glCullFace(v); }
static state_object<GLenum, cull_face_func> cull_face_obj {GL_BACK};

void gl::cull(bool v) {
	cull_enabled_obj.set(v);
}

void gl::cull_face(GLenum v) {
	cull_face_obj.set(v);
}

//================================
// DEPTH
//================================

inline static void depth_func_func(GLenum const & v) { glDepthFunc(v); }
static state_object<GLenum, depth_func_func> depthfunc {GL_LEQUAL};

inline static void depth_test_enabled_func(bool const & v) { v ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST); }
static state_object<bool, depth_test_enabled_func> depth_test_enabled {false};

inline static void depth_write_enabled_func(bool const & v) { glDepthMask(v ? GL_TRUE : GL_FALSE); }
static state_object<bool, depth_write_enabled_func> depth_write_enabled {false};

void gl::depth_func(GLenum v) {
	depthfunc.set(v);
}

void gl::depth_test(bool v) {
	depth_test_enabled.set(v);
}

void gl::depth_write(bool v) {
	depth_write_enabled.set(v);
}

//================================
// STENCIL
//================================

using stencil_func_set = std::tuple<GLenum, GLint, GLuint>;
inline static void stencil_func_func(stencil_func_set const & v) { glStencilFunc(std::get<0>(v), std::get<1>(v), std::get<2>(v)); }
static state_object<stencil_func_set, stencil_func_func> stencil_func_state {{ GL_ALWAYS, 0, -1 }};

inline static void stencil_test_enabled_func(bool const & v) { v ? glEnable(GL_STENCIL_TEST) : glDisable(GL_STENCIL_TEST); }
static state_object<bool, stencil_test_enabled_func> stencil_test_state {false};

inline static void stencil_write_enabled_func(GLuint const & v) { glStencilMask(v ? GL_TRUE : GL_FALSE); }
static state_object<GLuint, stencil_write_enabled_func> stencil_mask_state {static_cast<GLuint>(-1)};

using stencil_op_set = std::tuple<GLenum, GLenum, GLenum>;
inline static void stencil_op_func(stencil_op_set const & v) { glStencilOp(std::get<0>(v), std::get<1>(v), std::get<2>(v)); }
static state_object<stencil_op_set, stencil_op_func> stencil_op_state {{ GL_KEEP, GL_KEEP, GL_KEEP }};

void gl::stencil_func(GLenum func, GLint ref, GLuint mask) {
	stencil_func_state.set({func, ref, mask});
}

void gl::stencil_test(bool v) {
	stencil_test_state.set(v);
}

void gl::stencil_mask(GLuint v) {
	stencil_mask_state.set(v);
}

void gl::stencil_op(GLenum fail, GLenum passdfail, GLenum pass) {
	stencil_op_state.set({fail, passdfail, pass});
}

//================================
// PRIMITIVES
//================================

inline static void line_width_func(float const & v) { glLineWidth(v); }
static state_object<float, line_width_func> line_width_state { 1.0f };

void gl::line_width(float v) {
	line_width_state.set(v);
}

//================================
// POLYGONS
//================================

using polygon_mode_pair = std::pair<GLenum, GLenum>;
inline static void polygon_mode_func(polygon_mode_pair const & v) { glPolygonMode(v.first, v.second); }
static state_object<polygon_mode_pair, polygon_mode_func> polygonmode {{GL_FRONT_AND_BACK, GL_FILL}};

inline static void polygon_offset_enabled_func(bool const & v) { v ? glEnable(GL_POLYGON_OFFSET_FILL) : glDisable(GL_POLYGON_OFFSET_FILL); }
static state_object<bool, polygon_offset_enabled_func> polygon_offset_enabled {false};

using polygon_offset_pair = std::pair<float, float>;
inline static void polygon_offset_func(polygon_offset_pair const & v) { glPolygonOffset(v.first, v.second); }
static state_object<polygon_offset_pair, polygon_offset_func> polygonoffset {{-1, -2}};

void gl::polygon_mode(GLenum face, GLenum mode) {
	polygonmode.set({face, mode});
}

void gl::polygon_offset_fill(bool v) {
	polygon_offset_enabled.set(v);
}

void gl::polygon_offset(float factor, float units) {
	polygonoffset.set({factor, units});
}

//================================
// INITIALIZE
//================================

void gl::initialize() {
	alpha_test_enabled_state.reset();
	alpha_func_state.reset();
	blend_enabled.reset();
	blendfunc.reset();
	cull_enabled_obj.reset();
	cull_face_obj.reset();
	depthfunc.reset();
	depth_test_enabled.reset();
	depth_write_enabled.reset();
	stencil_func_state.reset();
	stencil_test_state.reset();
	stencil_mask_state.reset();
	stencil_op_state.reset();
	line_width_state.reset();
	polygonmode.reset();
	polygon_offset_enabled.reset();
	polygonoffset.reset();
}
