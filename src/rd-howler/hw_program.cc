#include "hw_local.hh"
using namespace howler;

GLuint q3program::bound_id = 0;

//================================================================
// PUBLIC
//================================================================

q3program::~q3program() {
	if (m_handle) {
		glDeleteProgram(m_handle);
		if (is_bound()) bound_id = 0;
	}
}

void q3program::bind() {
	if (is_bound()) return;
	glUseProgram(m_handle);
	bound_id = m_handle;
	on_bind();
}

//================================================================
// PROTECTED
//================================================================

q3program::q3program() {
	m_handle = glCreateProgram();
}

void q3program::attach(shader const & sh) {
	glAttachShader(m_handle, sh.m_handle);
}

bool q3program::link(std::string * log) {
	GLint success = GL_FALSE;
	glLinkProgram(m_handle);
	glGetProgramiv(m_handle, GL_LINK_STATUS, &success);
	if (log) {
		log->clear();
		GLint loglen = 0;
		glGetProgramiv(m_handle, GL_INFO_LOG_LENGTH, &loglen);
		if (loglen) {
			log->resize(loglen);
			glGetProgramInfoLog(m_handle, loglen, &loglen, log->data());
		}
	}
	return success == GL_TRUE;
}

//--------------------------------
// SHADER

q3program::shader::shader(GLenum type) {
	m_handle = glCreateShader(type);
}

q3program::shader::~shader() {
	if (m_handle) glDeleteShader(m_handle);
}

void q3program::shader::source(char const * src) {
	glShaderSource(m_handle, 1, &src, 0); 
}

bool q3program::shader::compile(std::string * log) {
	GLint success = GL_FALSE;
	glCompileShader(m_handle);
	glGetShaderiv(m_handle, GL_COMPILE_STATUS, &success);
	if (log) {
		log->clear();
		GLint loglen = 0;
		glGetShaderiv(m_handle, GL_INFO_LOG_LENGTH, &loglen);
		if (loglen) {
			log->resize(loglen);
			glGetShaderInfoLog(m_handle, loglen, &loglen, log->data());
		}
	}
	return success == GL_TRUE;
}

//================================================================
