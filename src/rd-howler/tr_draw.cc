#include "tr_local.hh"

void rend::draw(std::shared_ptr<frame_t> frame, bool doing_3d) {
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
				
				//glDisable(GL_DEPTH_TEST);
				unitquad.bind();
				for (q3stage const & stage : shaders[cmd.stretch_pic.hShader].stages) {
					shader_set_m(m, doing_3d);
					shader_setup_stage(stage, uv, frame->shader_time);
					unitquad.draw();
				}
			} break;
			case rcmd::mode_e::refent: {
				//glEnable(GL_DEPTH_TEST);
				auto mod = bankmodels.find(cmd.refent.hModel);
				if (mod == bankmodels.end()) continue;
				for (rendmesh const & mesh : mod->second.meshes) {
					mesh.bind();
					for (q3stage const & stage : shaders[mesh.shader].stages) {
						shader_set_m(rm4_t::translate({cmd.refent.origin[1], -cmd.refent.origin[2], cmd.refent.origin[0]}), doing_3d);
						shader_setup_stage(stage, {}, frame->shader_time);
					}
					mesh.draw();
				}
			} break;
			default:
				break;
		}
	}
}
