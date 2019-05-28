#include "tr_local.hh"

static bool blend_v;
static std::pair<GLenum, GLenum> blend_func_v;
static bool depth_mask_v;
static bool depth_test_v;

void gl::initialize_defaults() {
	
	blend_v = false;
	blend_func_v = {GL_ONE, GL_ZERO};
	depth_mask_v = false;
	depth_test_v = false;
	
	blend_v ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
	glBlendFunc(blend_func_v.first, blend_func_v.second);
	glDepthMask(depth_mask_v);
	depth_test_v ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
}

void gl::blend(bool v) {
	if (v == blend_v) return;
	(blend_v = v) ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
}

void gl::blend_func(GLenum v1, GLenum v2) {
	std::pair<GLenum, GLenum> v {v1, v2};
	if (v == blend_func_v) return;
	blend_func_v = v;
	glBlendFunc(v1, v2);
}

void gl::depth_mask(bool v) {
	if (v == depth_mask_v) return;
	glDepthMask(depth_mask_v = v);
}

void gl::depth_test(bool v) {
	if (v == depth_test_v) return;
	(depth_test_v = v) ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
}
