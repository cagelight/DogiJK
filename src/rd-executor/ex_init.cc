#include "ex_local.hh"
using namespace executor;

instance::instance(refimport_t const * ri_in) : ri(ri_in) {
	
}

#define VK_GLOBAL_LOAD(name) PFN_##name name = (PFN_##name)iproc(nullptr, #name);

void instance::initialize_renderer() {
	
	windowDesc_t windowDesc {};
	windowDesc.api = GRAPHICS_API_VULKAN;
	window = ri->WIN_Init(&windowDesc, &glConfig);
	
	vk::resolve_globals((PFN_vkGetInstanceProcAddr)ri->VK_GetVkInstanceProcAddr());
	
	uint32_t version;
	vk::EnumerateInstanceVersion(&version);
	windowDesc.vulkan.majorVersion = VK_VERSION_MAJOR(version);
	windowDesc.vulkan.minorVersion = VK_VERSION_MINOR(version);
	
	Com_Printf("Vulkan -- API Version: %u.%u\n", VK_VERSION_MAJOR(version), VK_VERSION_MINOR(version));
	
	std::vector<std::string> required_extensions, required_layers;
	ri->VK_GetInstanceExtensions(required_extensions);
	
	#if EXECUTOR_VK_DEBUG_MODE == 1
		required_extensions.push_back("VK_EXT_debug_report");
		required_layers.push_back("VK_LAYER_KHRONOS_validation");
	#endif
	
	uint32_t supported_extensions_cnt;
	vk::EnumerateInstanceExtensionProperties(nullptr, &supported_extensions_cnt, nullptr);
	std::vector<VkExtensionProperties> supported_extensions;
	supported_extensions.resize(supported_extensions_cnt);
	vk::EnumerateInstanceExtensionProperties(nullptr, &supported_extensions_cnt, supported_extensions.data());
	
	uint32_t supported_layers_cnt;
	vk::EnumerateInstanceLayerProperties(&supported_layers_cnt, nullptr);
	std::vector<VkLayerProperties> supported_layers;
	supported_layers.resize(supported_layers_cnt);
	vk::EnumerateInstanceLayerProperties(&supported_layers_cnt, supported_layers.data());
	
	for (std::string const & ext : required_extensions) {
		bool present = std::any_of(supported_extensions.begin(), supported_extensions.end(), [&](VkExtensionProperties const & str){return ext == str.extensionName;});
		if (!present)
			Com_Error(ERR_FATAL, "required vulkan extension not present: \"%s\"", ext.c_str());
	}
	
	for (std::string const & ext : required_layers) {
		bool present = std::any_of(supported_layers.begin(), supported_layers.end(), [&](VkLayerProperties const & str){return ext == str.layerName;});
		if (!present)
			Com_Error(ERR_FATAL, "required vulkan layer not present: \"%s\"", ext.c_str());
	}
	
	std::stringstream ext_sup_str, lay_sup_str, ext_use_str, lay_use_str;
	for (auto const & ext : supported_extensions) ext_sup_str << " " << ext.extensionName;
	for (auto const & lay : supported_layers) lay_sup_str << " " << lay.layerName;
	for (auto const & ext : required_extensions) ext_use_str << " " << ext;
	for (auto const & lay : required_layers) lay_use_str << " " << lay;
	
	Com_Printf("Vulkan -- Available Extensions: %s\n", ext_sup_str.str().c_str());
	Com_Printf("Vulkan -- Available Layers: %s\n", lay_sup_str.str().c_str());
	Com_Printf("Vulkan -- Using Extensions: %s\n", ext_use_str.str().c_str());
	Com_Printf("Vulkan -- Using Layers: %s\n", lay_use_str.str().c_str());
	
	vk_inst = vk::instance::create(required_extensions, required_layers, VK_MAKE_VERSION(1, 1, 0));
	
	auto const & pdevs = vk_inst->physical_devices();
	uint32_t chosen_pdev = qm::clamp<uint32_t>(r_vkdeviceindex->integer, 0, pdevs.size() - 1);
	vk::physical_device const & pdev = vk_inst->physical_devices()[chosen_pdev];
	Com_Printf("Vulkan -- Device: %s\n", pdev.properties().deviceName);
	
	
}
