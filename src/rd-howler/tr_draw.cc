#include "tr_local.hh"

static constexpr rm4_t projection_2d = rm4_t::ortho(0, 480, 0, 640, 0, 1);

void rend::draw(std::shared_ptr<rframe> frame) {
	for (rcmd & cmd : frame->d_2d) {
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
				for (q3stage const & stage : shaders[cmd.stretch_pic.hShader].stages) {
					configure_stage(stage, m * projection_2d, uv, frame->shader_time);
					unitquad.draw();
				}
			} break;
		}
	}
}
