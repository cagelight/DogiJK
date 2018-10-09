#include "tr_local.hh"

struct draw_visitor {
	
	rm4_t vp;
	float shader_time = 0;
	
	void operator () (rv4_t const & color_2d) {
		r->color_2d = color_2d;
	}
	
	void operator () (stretch_pic const & pic) {
		rm4_t m = rm4_t::scale(pic.w ? pic.w : pic.h * (640.0f / 480.0f), pic.h, 1);
		m *= rm4_t::translate(pic.x, pic.y, 0);
		
		rm3_t uv = rm3_t::scale(pic.s2 - pic.s1, pic.t2 - pic.t1);
		uv *= rm3_t::translate(pic.s1, pic.t1);
		
		glDepthMask(GL_FALSE);
		
		r->unitquad.bind();
		if (!pic.shader->index) {
			r->missingnoise_program->use();
			r->shader_set_mvp(m * vp);
			glUniform1f(UNIFORM_TIME, shader_time);
			r->unitquad.draw();
			r->q3program->use();
			return;
		}
		
		r->shader_set_mvp(m * vp);
		
		for (size_t i = 0; i < pic.shader->stages.size(); i++) {
			r->shader_setup_stage(pic.shader->stages[i], uv, shader_time);
			r->unitquad.draw();
		}
	}
	
	void operator () (refEntity_t const & ent) {
		qhandle_t model = 0;
		
		if (ent.hModel) model = ent.hModel;
		else if (ent.ghoul2) {
			model = ri.G2_At(*(CGhoul2Info_v *)ent.ghoul2, 0).mModel;
		}
		
		if (!model) return;
		
		auto mod = r->models.find(model);
		if (mod == r->models.end()) return;
		
		rm4_t v = rm4_t::translate({ent.origin[1], -ent.origin[2], ent.origin[0]});
		rm4_t a = {
			ent.axis[1][1], -ent.axis[1][2], ent.axis[1][0], 0,
			ent.axis[2][1], -ent.axis[2][2], ent.axis[2][0], 0,
			ent.axis[0][1], -ent.axis[0][2], ent.axis[0][0], 0,
			0, 0, 0, 1
		};
		auto m = a * v;
		r->shader_set_mvp(m * vp);
		glDepthMask(GL_FALSE);
		
		for (rend::rendmesh const & mesh : mod->second.meshes) {
			mesh.bind();
			q3shader_ptr shad = r->shader_get(mesh.shader);
			if (!shad || !shad->index) {
				r->missingnoise_program->use();
				r->shader_set_mvp(m * vp);
				glDepthMask(GL_TRUE);
				glUniform1f(UNIFORM_TIME, shader_time);
				mesh.draw();
				r->q3program->use();
				continue;
			}
			
			glSamplerParameteri(r->q3sampler, GL_TEXTURE_MIN_FILTER, shad->mipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
			for (size_t i = 0; i < shad->stages.size(); i++) {
				if (i == shad->stages.size() - 1 && shad->opaque) glDepthMask(GL_TRUE);
				r->shader_setup_stage(shad->stages[i], {}, shader_time);
				mesh.draw();
			}
		}
	}
};

struct line_visitor {
	rm4_t vp;
	
	void operator () (rv4_t const & color_2d) {
		
	}
	
	void operator () (stretch_pic const & pic) {
		
		rm4_t m = rm4_t::scale(pic.w ? pic.w : pic.h * (640.0f / 480.0f), pic.h, 1);
		m *= rm4_t::translate(pic.x, pic.y, 0);
		
		r->unitquad.bind();
		r->shader_set_mvp(m * vp);
		r->unitquad.draw();
	}
	
	void operator () (refEntity_t const & ent) {
		
		qhandle_t model = 0;
		
		if (ent.hModel) model = ent.hModel;
		else if (ent.ghoul2) {
			model = ri.G2_At(*(CGhoul2Info_v *)ent.ghoul2, 0).mModel;
		}
		
		if (!model) return;
		
		auto mod = r->models.find(model);
		if (mod == r->models.end()) return;
		
		rm4_t v = rm4_t::translate({ent.origin[1], -ent.origin[2], ent.origin[0]});
		rm4_t a = {
			ent.axis[1][1], -ent.axis[1][2], ent.axis[1][0], 0,
			ent.axis[2][1], -ent.axis[2][2], ent.axis[2][0], 0,
			ent.axis[0][1], -ent.axis[0][2], ent.axis[0][0], 0,
			0, 0, 0, 1
		};
		
		auto m = a * v;
		r->shader_set_mvp(m * vp);
		
		for (rend::rendmesh const & mesh : mod->second.meshes) {
			mesh.bind();
			mesh.draw();
		}
	}
};

void rend::draw(std::shared_ptr<frame_t> frame) {
	
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	q3program->use();
	
	draw_visitor dv {
		.vp = frame->vp,
		.shader_time = frame->shader_time
	};
	for (rcmd const & cmd : frame->cmds) {
		std::visit(dv, cmd);
	}
	
	if (r_showtris->integer) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
		glDepthMask(GL_FALSE);
		basic_color_program->use();
		static float line_color[4] = {1, 1, 1, 1};
		glUniform4fv(UNIFORM_COLOR, 1, line_color);
		
		line_visitor lv {
			.vp = frame->vp
		};
		for (rcmd const & cmd : frame->cmds) {
			std::visit(lv, cmd);
		}
	}
}
