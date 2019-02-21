#include "ruby.hh"

#include "qcommon/qcommon.hh"

#include <mruby.h>
#include <mruby/class.h>
#include <mruby/compile.h>
#include <mruby/data.h>
#include <mruby/value.h>
#include <mruby/string.h>
#include <mruby/proc.h>

constexpr char const ruby_core_source [] = R"(
module COM
	
end
)";

static mrb_value com_print (mrb_state * state, mrb_value self) {
	
	mrb_value val;
	mrb_get_args(state, "o", &val);
	char const * str = mrb_string_value_ptr(state, val);
	
	if (str) Com_Printf(str);
	return mrb_nil_value();
}

static mrb_value com_cvar_get(mrb_state * state, mrb_value self) {
	
	std::array<mrb_value, 2> args;
	mrb_int nargs = mrb_get_args(state, "s|o", args.data());
	
	char const * cvar_name = mrb_string_value_ptr(state, args[0]);
	char const * default_value = nargs >= 1 ? mrb_string_value_ptr(state, args[1]) : "0";
	
	cvar_t * cvar = Cvar_Get(cvar_name, default_value, 0);
	return mrb_str_new_cstr(state, cvar->string);
}

/*
static void com_file_close(mrb_state * state, void * ptr) {
	fileHandle_t fh = (uint8_t *)(ptr) - (uint8_t *)(0);
	FS_FCloseFile(fh);
}

static mrb_value com_file_open (mrb_state * state, mrb_value self) {
	
	mrb_value val;
	mrb_get_args(state, "o", &val);
	char const * path = mrb_string_value_ptr(state, val);
	
	fileHandle_t fh;
	long len = FS_FOpenFileRead(path, &fh, qfalse);	
	static constexpr mrb_data_type dt {
		"__fh", com_file_close
	};
	mrb_data_init(self, (uint8_t *)0 + fh, &dt);
	return self;
}
*/

struct ruby_core_actual : public ruby_core {
	
	mrb_state * state = nullptr;
	
	struct RClass * kernel = nullptr;
	struct RClass * com = nullptr;
	struct RClass * com_file = nullptr;
	
	ruby_core_actual() {
		
		state = mrb_open();
		mrb_load_string(state, ruby_core_source);
		
		kernel = state->kernel_module;
		mrb_define_method(state, kernel, "__printstr__", com_print, MRB_ARGS_REQ(1));
		
		
		com = mrb_module_get(state, "COM");
		mrb_define_method(state, com, "cvar_get", com_cvar_get, MRB_ARGS_ARG(1, 1));
		/*
		com_file = mrb_define_class_under(state, com, "File", nullptr);
		MRB_SET_INSTANCE_TT(com_file, MRB_TT_DATA);
		mrb_define_method(state, com_file, "open", com_file_open, MRB_ARGS_REQ(1));
		*/
	}
	
	~ruby_core_actual() {
		if (state) mrb_close(state);
	}
	
	void eval(std::string const & str) override {
		mrb_parse_string(state, str.c_str(), nullptr);
		mrb_load_string(state, str.c_str());
	}
	
};

std::unique_ptr<ruby_core> create_rubycore() {
	return std::make_unique<ruby_core_actual>();
}
