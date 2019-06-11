#include "tr_local.hh"

q3framebuffer::q3framebuffer(GLsizei w, GLsizei h) : width(w), height(h) {
	glCreateFramebuffers(1, &id);
}

q3framebuffer::~q3framebuffer() {
	attachments.clear();
	if (dsrb) {
		glDeleteRenderbuffers(1, &dsrb);
	}
	if (id) glDeleteFramebuffers(1, &id);
}

void q3framebuffer::resize(GLsizei w, GLsizei h) {
	width = w;
	height = h;
	
	for (auto & iter : attachments) {
		auto & tex = iter.second.first;
		tex = make_q3texture(width, height, iter.second.second);
		glNamedFramebufferTexture(id, static_cast<GLenum>(iter.first), tex->get_id(), 0);
	}
	
	if (dsrb) {
		glDeleteRenderbuffers(1, &dsrb);
		dsrb = 0;
		attach_depth_stencil_renderbuffer();
	}
	
	assert(validate());
}

void q3framebuffer::attach(attachment atch, GLenum type) {
	
	switch (atch) {
		default:
			if (std::find(color_attachments.begin(), color_attachments.end(), static_cast<GLenum>(atch)) == color_attachments.end()) {
				color_attachments.push_back(static_cast<GLenum>(atch));
				glNamedFramebufferDrawBuffers(id, color_attachments.size(), color_attachments.data());
			}
			break;
		case attachment::depth:
			if (dsrb || attachments.find(attachment::depth_stencil) != attachments.end())
				Com_Error(ERR_FATAL, "q3framebuffer::attach:: attempted to add depth buffer but depth-stencil buffer already present.");
			break;
		case attachment::stencil:
			if (dsrb || attachments.find(attachment::depth_stencil) != attachments.end())
				Com_Error(ERR_FATAL, "q3framebuffer::attach:: attempted to add stencil buffer but depth-stencil buffer already present.");
			break;
		case attachment::depth_stencil:
			if (dsrb) 
				Com_Error(ERR_FATAL, "q3framebuffer::attach:: attempted to add depth-stencil buffer but depth-stencil renderbuffer already present.");
			if (attachments.find(attachment::depth) != attachments.end())
				Com_Error(ERR_FATAL, "q3framebuffer::attach:: attempted to add depth-stencil buffer but depth buffer already present.");
			if (attachments.find(attachment::stencil) != attachments.end())
				Com_Error(ERR_FATAL, "q3framebuffer::attach:: attempted to add depth-stencil buffer but stencil buffer already present.");
			break;
	}
	
	auto & iter = attachments[atch] = { make_q3texture(width, height, false, type), type };
	q3texture_ptr & tex = iter.first;
	glNamedFramebufferTexture(id, static_cast<GLenum>(atch), tex->get_id(), 0);
}

void q3framebuffer::attach_depth_stencil_renderbuffer() {
	if (dsrb) return;
	glCreateRenderbuffers(1, &dsrb);
	glNamedRenderbufferStorage(dsrb, GL_DEPTH24_STENCIL8, width, height);
	glNamedFramebufferRenderbuffer(id, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, dsrb);
}

q3texture_const_ptr q3framebuffer::get_attachment(attachment atch) const {
	auto const & iter = attachments.find(atch);
	if (iter == attachments.end()) return nullptr;
	return iter->second.first;
}

void q3framebuffer::bind() {
	glBindFramebuffer(GL_FRAMEBUFFER, id);
	glViewport(0, 0, width, height);
	glScissor(0, 0, width, height);
}

void q3framebuffer::unbind() {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, r->width, r->height);
	glScissor(0, 0, r->width, r->height);
}

bool q3framebuffer::validate() const {
	if (glCheckNamedFramebufferStatus(id, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		return false;
	return true;
}
