#include "tr_local.hh"

void rend::draw(std::shared_ptr<frame_t> frame) {
	
	shader_set_vp(frame->vp);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	q3program->use();
	
	//Com_Printf("\n\n========RENDER FRAME========\n");
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
				
				shader_set_m(m);
				glDepthMask(GL_FALSE);
				
				unitquad.bind();
				q3shader const & shad = shader_get(cmd.stretch_pic.hShader);
				if (!shad.index) {
					missingnoise_program->use();
					shader_set_m(m);
					glUniform1f(UNIFORM_TIME, frame->shader_time);
					unitquad.draw();
					q3program->use();
					continue;
				}
				
				for (size_t i = 0; i < shad.stages.size(); i++) {
					shader_setup_stage(shad.stages[i], uv, frame->shader_time);
					unitquad.draw();
				}
				
			} break;
			case rcmd::mode_e::refent: {
				
				qhandle_t model = 0;
				
				if (cmd.refent.hModel) model = cmd.refent.hModel;
				else if (cmd.refent.ghoul2) {
					model = ri.G2_At(*(CGhoul2Info_v *)cmd.refent.ghoul2, 0).mModel;
				}
				
				if (!model) continue;
				
				auto mod = bankmodels.find(model);
				if (mod == bankmodels.end()) continue;
				//Com_Printf("Rendering: %s\n", mod->second.name.c_str());
				
				rm4_t v = rm4_t::translate({cmd.refent.origin[1], -cmd.refent.origin[2], cmd.refent.origin[0]});
				rm4_t a = {
					cmd.refent.axis[1][1], -cmd.refent.axis[1][2], cmd.refent.axis[1][0], 0,
					cmd.refent.axis[2][1], -cmd.refent.axis[2][2], cmd.refent.axis[2][0], 0,
					cmd.refent.axis[0][1], -cmd.refent.axis[0][2], cmd.refent.axis[0][0], 0,
					0, 0, 0, 1
				};
				v = a * v;
				shader_set_m(v);
				glDepthMask(GL_FALSE);
				
				for (rendmesh const & mesh : mod->second.meshes) {
					mesh.bind();
					q3shader const & shad = shader_get(mesh.shader);
					if (!shad.index) {
						missingnoise_program->use();
						shader_set_m(v);
						glDepthMask(GL_TRUE);
						glUniform1f(UNIFORM_TIME, frame->shader_time);
						mesh.draw();
						q3program->use();
						continue;
					}
					
					glSamplerParameteri(q3sampler, GL_TEXTURE_MIN_FILTER, shad.mipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
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
	
	if (!r_showtris->integer) return;
	
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glDepthMask(GL_FALSE);
	basic_color_program->use();
	static float line_color[4] = {1, 1, 1, 1};
	glUniform4fv(UNIFORM_COLOR, 1, line_color);
	
	for (rcmd & cmd : frame->cmds) {
		switch (cmd.mode) {
			case rcmd::mode_e::stretch_pic: {
				if (cmd.stretch_pic.w == 0) {
					cmd.stretch_pic.w = cmd.stretch_pic.h * (640.0f / 480.0f);
				}
				
				rm4_t m = rm4_t::scale(cmd.stretch_pic.w, cmd.stretch_pic.h, 1);
				m *= rm4_t::translate(cmd.stretch_pic.x, cmd.stretch_pic.y, 0);
				
				rm3_t uv = rm3_t::scale(cmd.stretch_pic.s2 - cmd.stretch_pic.s1, cmd.stretch_pic.t2 - cmd.stretch_pic.t1);
				uv *= rm3_t::translate(cmd.stretch_pic.s1, cmd.stretch_pic.t1);
				
				unitquad.bind();
				shader_set_m(m);
				unitquad.draw();
			} break;
			case rcmd::mode_e::refent: {
				
				qhandle_t model = 0;
				
				if (cmd.refent.hModel) model = cmd.refent.hModel;
				else if (cmd.refent.ghoul2) {
					model = ri.G2_At(*(CGhoul2Info_v *)cmd.refent.ghoul2, 0).mModel;
				}
				
				if (!model) continue;
				
				auto mod = bankmodels.find(model);
				if (mod == bankmodels.end()) continue;
				
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
					mesh.draw();
				}
			} break;
			default:
				break;
		}
	}
}
