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
	rm4_t vp = rm4_t::identity();
	
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
				gl::depth_mask(true);
				r->missingnoise_program->use();
				r->shader_set_mvp(m * vp);
				glUniform1f(UNIFORM_TIME, shader_time);
				mesh.draw();
				r->q3program->use();
				continue;
			}
			
			gl::depth_mask(mesh.shader->opaque);
			r->shader_presetup(*mesh.shader);
			glSamplerParameteri(r->q3sampler, GL_TEXTURE_MIN_FILTER, mesh.shader->mipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
			
			for (size_t i = 0; i < mesh.shader->stages.size(); i++) {
				r->shader_setup_stage(mesh.shader->stages[i], rm3_t::identity(), shader_time);
				mesh.draw();
			}
		}
		
	}
	
};

struct objectdraw {
	objectdraw(q3mesh const * mesh, rm4_t const & m) : mesh(mesh), m(m) {}
	q3mesh const * mesh;
	rm4_t m = rm4_t::identity();
};

using drawvariant = std::variant<
	q3mesh const *,
	objectdraw
>;

struct visitor_draw3d {
	
	float shader_time = 0.0f;
	rm4_t vp = rm4_t::identity();
	
	void operator () (q3mesh const * mesh) {
		r->shader_set_mvp(vp);
		mesh->bind();
		
		if (!mesh->shader || !mesh->shader->index) {
			gl::depth_mask(true);
			r->missingnoise_program->use();
			glUniform1f(UNIFORM_TIME, shader_time);
			mesh->draw();
			r->q3program->use();
			return;
		}
		
		gl::depth_mask(mesh->shader->opaque);
		r->shader_presetup(*mesh->shader);
		glSamplerParameteri(r->q3sampler, GL_TEXTURE_MIN_FILTER, mesh->shader->mipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
		
		for (size_t i = 0; i < mesh->shader->stages.size(); i++) {
			r->shader_setup_stage(mesh->shader->stages[i], rm3_t::identity(), shader_time);
			mesh->draw();
		}
	}
	
	void operator () (objectdraw const & ref) {
		r->shader_set_mvp(ref.m * vp);
		ref.mesh->bind();
		
		if (!ref.mesh->shader || !ref.mesh->shader->index) {
			gl::depth_mask(true);
			r->missingnoise_program->use();
			glUniform1f(UNIFORM_TIME, shader_time);
			ref.mesh->draw();
			r->q3program->use();
			return;
		}
		
		gl::depth_mask(ref.mesh->shader->opaque);
		r->shader_presetup(*ref.mesh->shader);
		glSamplerParameteri(r->q3sampler, GL_TEXTURE_MIN_FILTER, ref.mesh->shader->mipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
		
		for (size_t i = 0; i < ref.mesh->shader->stages.size(); i++) {
			r->shader_setup_stage(ref.mesh->shader->stages[i], rm3_t::identity(), shader_time);
			ref.mesh->draw();
		}
	}
};

#include <unordered_set>

size_t q3mesh::draw_count = 0;

void rend::draw(std::shared_ptr<frame_t> frame) {
	
	/*
	if (r_fboratio->modified) framebuffer->resize(width * r_fboratio->value, height * r_fboratio->value);
	framebuffer->bind();
	*/
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	gl::depth_test(true);
	glEnable(GL_BLEND);
	glDepthFunc(GL_LEQUAL);
	q3program->use();
	
	if (world) world->lightmap_atlas->bind(BINDING_LIGHTMAP_ATLAS);
	
	q3mesh::draw_count = 0;
	
	for (auto const & scene : frame->cmds3d) {
		
		if (!scene.active) continue;
		
		rv3_t view_origin = {scene.def.vieworg[0], scene.def.vieworg[1], scene.def.vieworg[2]};
		
		std::map<float, std::vector<drawvariant>> rmap;
		
		bool render_world = !(scene.def.rdflags & RDF_NOWORLDMODEL);
		if (!world) render_world = false;
		
		if (render_world) {
			int32_t cluster = std::get<world_t::mapnode_t::leaf_data>(world->point_in_leaf(view_origin)->data).cluster;
			printf("cluster: %i\n", cluster);
			for (auto const & mesh : world->get_model(cluster)->meshes) {
				rmap[mesh.shader->sort].emplace_back(&mesh);
			}
		}
		
		visitor_draw3d v3d {
			.shader_time = frame->shader_time,
			.vp = scene.vp
		};
		
		for (cmd3d const & cmd : scene.cmds3d) {
			std::visit(lambda_visit{
				[&](basic_mesh const & ref) {
					
					if (!ref.model) return;
					
					rm4_t v = rm4_t::translate({ref.origin[1], -ref.origin[2], ref.origin[0]});
					rm4_t a = {
						ref.pre[1][1], -ref.pre[1][2], ref.pre[1][0], 0,
						ref.pre[2][1], -ref.pre[2][2], ref.pre[2][0], 0,
						ref.pre[0][1], -ref.pre[0][2], ref.pre[0][0], 0,
						0, 0, 0, 1
					};
					auto m = a * v;
					
					for (q3mesh const & mesh : ref.model->meshes) {
						rmap[mesh.shader->sort].emplace_back( objectdraw{&mesh, m} );
					}
				}
			}, cmd);
		}
		
		for (auto const & [sort, draws] : rmap) {
			for (auto const & draw : draws) std::visit(v3d, draw);
		}
	}
	
	gl::depth_mask(false);
	gl::depth_test(false);
	glDisable(GL_CULL_FACE);
	
	visitor_2d v2d {
		.shader_time = frame->shader_time
	};
	
	for (cmd2d const & cmd : frame->cmds2d) {
		std::visit(v2d, cmd);
	}
	
	/*
	dynamicglow_program->use();
	glBindImageTexture(0, framebuffer->get_attachment(q3framebuffer::attachment::color1)->get_id(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
	glBindImageTexture(1, scratch1->get_id(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
	glDispatchCompute(width / 40, height / 20, 1);
	glBindImageTexture(0, scratch1->get_id(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
	glBindImageTexture(1, framebuffer->get_attachment(q3framebuffer::attachment::color1)->get_id(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
	glDispatchCompute(width / 40, height / 20, 1);
	*/
	
	/*
	framebuffer->unbind();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	mainfbotransfer_program->use();
	framebuffer->get_attachment(q3framebuffer::attachment::color0)->bind(FBOFINAL_COLOR);
	framebuffer->get_attachment(q3framebuffer::attachment::color1)->bind(FBOFINAL_GLOW);
	fullquad.bind();
	fullquad.draw();
	*/
	
	printf("Draw Count: %zu\n", q3mesh::draw_count);
	
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
