#include "ex_local.hh"
using namespace executor;

instance::shader_registry::shader_registry() {
	
}

q3shader_ptr instance::shader_registry::reg(istring const & name, bool mipmaps) {
	auto m = lookup.find(name);
	if (m != lookup.end()) return m->second;
	
	q3shader_ptr shad = shaders.emplace_back(make_q3shader());
	shad->index = shaders.size() - 1;
	shad->name = name;
	
	lookup[name] = shad;
	return shad;
}

q3shader_ptr instance::shader_registry::get(qhandle_t h) {
	if (h < 0 || h >= static_cast<int32_t>(shaders.size())) return nullptr;
	return shaders[h];
}
