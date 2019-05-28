#include "ex_local.hh"
using namespace executor;

instance::instance(refimport_t const * ri_in) : ri(ri_in) {
	
}

#define VK_GLOBAL_LOAD(name) PFN_##name name = (PFN_##name)iproc(nullptr, #name);

void instance::initialize_renderer() {
	
	windowDesc_t windowDesc {};
	windowDesc.api = GRAPHICS_API_VULKAN;
	window = ri->WIN_Init(&windowDesc, &glConfig);
	
	PFN_vkGetInstanceProcAddr iproc = (PFN_vkGetInstanceProcAddr)ri->VK_GetVkInstanceProcAddr();
	
	VK_GLOBAL_LOAD(vkEnumerateInstanceExtensionProperties);
	VK_GLOBAL_LOAD(vkEnumerateInstanceLayerProperties);
}
