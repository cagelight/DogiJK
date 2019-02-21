#pragma once

#include "qcommon/q_shared.hh"

struct ruby_core {
	ruby_core() = default;
	virtual ~ruby_core() = default;
	
	virtual void eval(std::string const &) = 0;
};

std::unique_ptr<ruby_core> create_rubycore();
