#include "tr_local.hh"

static float	s_flipMatrix[16] = {
	// convert from our coordinate system (looking down X)
	// to OpenGL's coordinate system (looking down -Z)
	0, 0, -1, 0,
	-1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 0, 1
};

static void myGlMultMatrix( const float *a, const float *b, float *out ) {
	int		i, j;

	for ( i = 0 ; i < 4 ; i++ ) {
		for ( j = 0 ; j < 4 ; j++ ) {
			out[ i * 4 + j ] =
				a [ i * 4 + 0 ] * b [ 0 * 4 + j ]
				+ a [ i * 4 + 1 ] * b [ 1 * 4 + j ]
				+ a [ i * 4 + 2 ] * b [ 2 * 4 + j ]
				+ a [ i * 4 + 3 ] * b [ 3 * 4 + j ];
		}
	}
}

void rend::draw(std::shared_ptr<frame_t> frame) {
	shader_set_vp(frame->vp);
	for (rcmd & cmd : frame->cmds) {
		switch (cmd.mode) {
			case rcmd::mode_e::color_2d: {
				r->set_color_2d(cmd.color_2d);
			} break;
			case rcmd::mode_e::stretch_pic: {
				if (cmd.stretch_pic.w == 0) {
					cmd.stretch_pic.w = cmd.stretch_pic.h * (640.0f / 480.0f);
				}
				
				rm4_t m = rm4_t::scale(cmd.stretch_pic.w, cmd.stretch_pic.h, 1);
				m *= rm4_t::translate(cmd.stretch_pic.x, cmd.stretch_pic.y, 0);
				
				rm3_t uv = rm3_t::scale(cmd.stretch_pic.s2 - cmd.stretch_pic.s1, cmd.stretch_pic.t2 - cmd.stretch_pic.t1);
				uv *= rm3_t::translate(cmd.stretch_pic.s1, cmd.stretch_pic.t1);
				
				unitquad.bind();
				q3shader const & shad = shaders[cmd.stretch_pic.hShader];
				glDepthMask(GL_FALSE);
				
				for (size_t i = 0; i < shad.stages.size(); i++) {
					if (i == shad.stages.size() - 1 && shad.opaque) glDepthMask(GL_TRUE);
					shader_set_m(m);
					shader_setup_stage(shad.stages[i], uv, frame->shader_time);
					unitquad.draw();
				}
				
			} break;
			case rcmd::mode_e::refent: {
				auto mod = bankmodels.find(cmd.refent.hModel);
				if (mod == bankmodels.end()) continue;
				Com_Printf("Rendering: %s\n", mod->second.name.c_str());
				
				rm4_t v = rm4_t::translate({cmd.refent.origin[1], -cmd.refent.origin[2], cmd.refent.origin[0]});
				rm4_t a = {
					cmd.refent.axis[1][1], -cmd.refent.axis[1][2], cmd.refent.axis[1][0], 0,
					cmd.refent.axis[2][1], -cmd.refent.axis[2][2], cmd.refent.axis[2][0], 0,
					cmd.refent.axis[0][1], -cmd.refent.axis[0][2], cmd.refent.axis[0][0], 0,
					0, 0, 0, 1
				};
				v = a * v;
				shader_set_m(v);
				
				for (rendmesh const & mesh : mod->second.meshes) {
					mesh.bind();
					q3shader const & shad = shaders[mesh.shader];
					glDepthMask(GL_FALSE);
					
					for (size_t i = 0; i < shad.stages.size(); i++) {
						if (i == shad.stages.size() - 1 && shad.opaque) glDepthMask(GL_TRUE);
						shader_setup_stage(shad.stages[i], {}, frame->shader_time);
						mesh.draw();
					}
				}
			} break;
			default:
				break;
		}
	}
}
