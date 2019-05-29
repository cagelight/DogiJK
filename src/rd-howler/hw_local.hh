#pragma once

#include "tr_local.hh"

namespace howler {
	
	#define FUNDAMENTAL(name)\
	struct name;\
	using name##_ptr = std::shared_ptr<name>;\
	using name##_cptr = std::shared_ptr<name const>;\
	template <typename ... T> name##_ptr make_##name(T ... args) { return std::make_shared<name>(args ...); }
	
	FUNDAMENTAL(q3model);
	FUNDAMENTAL(q3shader);
	FUNDAMENTAL(q3skin);
	
	#undef FUNDAMENTAL
	
	// INSTANCE
	struct instance {
		
		instance(refimport_t const *);
		void initialize_renderer();
		
		struct model_registry {
			model_registry();
			
			q3model_ptr reg(char const * name, bool server = false);
			q3model_ptr get(qhandle_t);
		private:
			std::unordered_map<istring, q3model_ptr> lookup;
			std::vector<q3model_ptr> models;
		} models;
		
		struct shader_registry {
			shader_registry();
			
			q3shader_ptr reg(istring const &, bool mipmaps = true);
			q3shader_ptr get(qhandle_t);
		private:
			std::unordered_map<istring, std::string> source_lookup;
			std::unordered_map<istring, q3shader_ptr> lookup;
			std::vector<q3shader_ptr> shaders;
		} shaders;
		
		struct skin_registry {
			q3skin_ptr reg(char const * name, bool server = false);
			q3skin_ptr get(qhandle_t);
		private:
			qhandle_t hcounter = 0;
			std::unordered_map<istring, q3skin_ptr> lookup;
			std::vector<q3skin_ptr> skins;
		} skins;
		
	private:
		
		refimport_t const * ri;
		window_t window;
		
	};
}
