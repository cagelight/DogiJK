#include "tr_local.hh"

#include "ghoul2/g2_public.hh"

/*
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
		glDisable(GL_CULL_FACE);
		
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
			return;
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
			q3shader_ptr shad = r->shader_get(mesh.shader);
			if (!shad || !shad->index) {
				glDepthMask(GL_TRUE);
				r->missingnoise_program->use();
				r->shader_set_mvp(m * vp);
				glUniform1f(UNIFORM_TIME, shader_time);
				mesh.draw();
				r->q3program->use();
				continue;
			}
			
			glDepthMask( shad->opaque ? GL_TRUE : GL_FALSE );
			r->shader_presetup(*shad);
			glSamplerParameteri(r->q3sampler, GL_TEXTURE_MIN_FILTER, shad->mipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
			for (size_t i = 0; i < shad->stages.size(); i++) {
				r->shader_setup_stage(shad->stages[i], {}, shader_time);
				mesh.draw();
			}
		}
	}
};

struct line_visitor {
	
	rm4_t vp;
	float shader_time = 0;
	
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
		bool ghoul2 = false;
		
		if (ent.hModel) model = ent.hModel;
		else if (ent.ghoul2) {
			model = ri.G2_At(*(CGhoul2Info_v *)ent.ghoul2, 0).mModel;
			ghoul2 = true;
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

*/

static constexpr rm4_t projection_2d = rm4_t::ortho(0, 480, 0, 640, 0, 1);

struct visitor_2d {
	
	float shader_time = 0;
	
	void operator () (rv4_t const & color_2d) {
		r->color_2d = color_2d;
	}
	
	void operator () (stretch_pic const & pic) {
		rm4_t m = rm4_t::scale(pic.w ? pic.w : pic.h * (640.0f / 480.0f), pic.h, 1);
		m *= rm4_t::translate(pic.x, pic.y, 0);
		
		rm3_t uv = rm3_t::scale(pic.s2 - pic.s1, pic.t2 - pic.t1);
		uv *= rm3_t::translate(pic.s1, pic.t1);
		
		if (!pic.shader) {
			return;
		}
		
		r->shader_set_mvp(m * projection_2d);
		r->unitquad.bind();
		
		for (size_t i = 0; i < pic.shader->stages.size(); i++) {
			r->shader_setup_stage(pic.shader->stages[i], uv, shader_time);
			r->unitquad.draw();
		}
	}
};

struct visitor_3d {
	
	float shader_time = 0.4f;
	rm4_t vp;
	
	void operator () (basic_mesh const & ref) {
		
		/*
		qhandle_t model = 0;
		bool ghoul2 = false;
		
		if (ref.hModel) model = ent.hModel;
		else if (ent.ghoul2) {
			model = ri.G2_At(*(CGhoul2Info_v *)ent.ghoul2, 0).mModel;
			ghoul2 = true;
		}
		*/
		
		if (!ref.model) return;
		
		rm4_t v = rm4_t::translate({ref.origin[1], -ref.origin[2], ref.origin[0]});
		rm4_t a = {
			ref.pre[1][1], -ref.pre[1][2], ref.pre[1][0], 0,
			ref.pre[2][1], -ref.pre[2][2], ref.pre[2][0], 0,
			ref.pre[0][1], -ref.pre[0][2], ref.pre[0][0], 0,
			0, 0, 0, 1
		};
		
		auto m = a * v;
		r->shader_set_mvp(m * vp);
		
		for (q3mesh const & mesh : ref.model->meshes) {
			mesh.bind();
			
			if (!mesh.shader || !mesh.shader->index) {
				glDepthMask(GL_TRUE);
				r->missingnoise_program->use();
				r->shader_set_mvp(m * vp);
				glUniform1f(UNIFORM_TIME, shader_time);
				mesh.draw();
				r->q3program->use();
				continue;
			}
			
			glDepthMask( mesh.shader->opaque ? GL_TRUE : GL_FALSE );
			r->shader_presetup(*mesh.shader);
			glSamplerParameteri(r->q3sampler, GL_TEXTURE_MIN_FILTER, mesh.shader->mipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
			
			for (size_t i = 0; i < mesh.shader->stages.size(); i++) {
				r->shader_setup_stage(mesh.shader->stages[i], {}, shader_time);
				mesh.draw();
			}
		}
		
	}
	
};

void rend::draw(std::shared_ptr<frame_t> frame) {
	
	rstate::reset();
	
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glDepthFunc(GL_LEQUAL);
	q3program->use();
	
	for (auto const & scene : frame->cmds3d) {
		
		bool render_world = !(scene.def.rdflags & RDF_NOWORLDMODEL);
		
		if (render_world) {
			shader_set_mvp(scene.vp);
			for (q3mesh const & worldmesh : opaque_world.meshes) {
				worldmesh.bind();
				r->shader_presetup(*worldmesh.shader);
				glSamplerParameteri(r->q3sampler, GL_TEXTURE_MIN_FILTER, worldmesh.shader->mipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
				for (size_t i = 0; i < worldmesh.shader->stages.size(); i++) {
					r->shader_setup_stage(worldmesh.shader->stages[i], {}, frame->shader_time);
					worldmesh.draw();
				}
				worldmesh.draw();
			}
		}
		
		visitor_3d v3d {
			.shader_time = frame->shader_time,
			.vp = scene.vp
		};
		
		for (cmd3d const & cmd : scene.cmds3d) {
			std::visit(v3d, cmd);
		}
	}
	
	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	
	visitor_2d v2d {
		.shader_time = frame->shader_time
	};
	
	for (cmd2d const & cmd : frame->cmds2d) {
		std::visit(v2d, cmd);
	}
	
	/*
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_TRUE);
	
	q3program->use();
	shader_set_mvp(frame->vp);
	
	for (rendmesh const & worldmesh : opaque_world.meshes) {
		worldmesh.bind();
		q3shader_ptr shad = r->shader_get(worldmesh.shader);
		r->shader_presetup(*shad);
		glSamplerParameteri(r->q3sampler, GL_TEXTURE_MIN_FILTER, shad->mipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
		for (size_t i = 0; i < shad->stages.size(); i++) {
			r->shader_setup_stage(shad->stages[i], {}, frame->shader_time);
			worldmesh.draw();
		}
		worldmesh.draw();
	}
	
	draw_visitor dv {
		.vp = frame->vp,
		.shader_time = frame->shader_time
	};
	for (rcmd const & cmd : frame->cmds) {
		std::visit(dv, cmd);
	}
	
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_FALSE);
	
	q3program->use();
	shader_set_mvp(frame->vp);
	
	for (rendmesh const & worldmesh : trans_world.meshes) {
		worldmesh.bind();
		q3shader_ptr shad = r->shader_get(worldmesh.shader);
		r->shader_presetup(*shad);
		glSamplerParameteri(r->q3sampler, GL_TEXTURE_MIN_FILTER, shad->mipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
		for (size_t i = 0; i < shad->stages.size(); i++) {
			r->shader_setup_stage(shad->stages[i], {}, frame->shader_time);
			worldmesh.draw();
		}
		worldmesh.draw();
	}
	
	if (r_showtris->integer) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
		glDisable(GL_CULL_FACE);
		glDepthMask(GL_FALSE);
		basic_color_program->use();
		static float line_color[4] = {1, 1, 1, 1};
		glUniform4fv(UNIFORM_COLOR, 1, line_color);
		
		shader_set_mvp(frame->vp);
		for (rendmesh const & worldmesh : opaque_world.meshes) {
			worldmesh.bind();
			worldmesh.draw();
		}
		for (rendmesh const & worldmesh : trans_world.meshes) {
			worldmesh.bind();
			worldmesh.draw();
		}
		
		line_visitor lv {
			.vp = frame->vp
		};
		for (rcmd const & cmd : frame->cmds) {
			std::visit(lv, cmd);
		}
	}
	*/
}
