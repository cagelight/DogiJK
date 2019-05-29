#include "vk_local.hh"

PFN_vkGetInstanceProcAddr vk::GetInstanceProcAddr;
#define VK_FN_GDECL
#include "vk.inl"

void vk::resolve_globals(PFN_vkGetInstanceProcAddr proc) {
	vk::GetInstanceProcAddr = proc;
	
	#define VK_FN_SYM_GLOBAL
	#include "vk.inl"
}

vk::instance_ptr vk::instance::create(std::vector<std::string> const & extensions, std::vector<std::string> const & layers, uint32_t version) {
	
	instance_ptr inst { new instance };
	
	std::vector<char const *> extensions_ptrs;
	extensions_ptrs.reserve(extensions.size());
	for (std::string const & str : extensions) extensions_ptrs.push_back(str.c_str());
	
	std::vector<char const *> layers_ptrs;
	layers_ptrs.reserve(layers.size());
	for (std::string const & str : layers) layers_ptrs.push_back(str.c_str());
	
	VkApplicationInfo app {};
	app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app.pApplicationName = "DogiJK";
	app.apiVersion = version;
	
	VkInstanceCreateInfo info {};
	info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	info.pApplicationInfo = &app;
	info.enabledExtensionCount = extensions_ptrs.size();
	info.ppEnabledExtensionNames = extensions_ptrs.size() ? extensions_ptrs.data() : nullptr;
	info.enabledLayerCount = layers_ptrs.size();
	info.ppEnabledLayerNames = layers_ptrs.size() ? layers_ptrs.data() : nullptr;
	
	VkResult res = vk::CreateInstance(&info, nullptr, &inst->handle);
	if (res != VK_SUCCESS) Com_Error(ERR_FATAL, "vk::instance::create: could not create a vulkan instance (result: %i)", res);
	
	inst->resolve_symbols();
	inst->enumerate_physical_devices();
	return inst;
}

vk::instance::~instance() {
	// this is segfaulting for some reason... need to investigate
	//if (handle) this->vkDestroyInstance(handle, nullptr);
}

void vk::instance::resolve_symbols() {
	#define VK_FN_SYM_INSTANCE
	#include "vk.inl"
}

void vk::instance::enumerate_physical_devices() {
	uint32_t pdev_num;
	this->vkEnumeratePhysicalDevices(handle, &pdev_num, nullptr);
	std::vector<VkPhysicalDevice> pdevs;
	pdevs.resize(pdev_num);
	this->vkEnumeratePhysicalDevices(handle, &pdev_num, pdevs.data());
	for (auto const & dev : pdevs) m_physical_devices.emplace_back(this, dev);
}

vk::physical_device::physical_device(instance const * inst, VkPhysicalDevice handle_in) : m_handle(handle_in) {
	inst->vkGetPhysicalDeviceProperties(m_handle, &m_properties);
	uint32_t num;
	inst->vkEnumerateDeviceExtensionProperties(m_handle, nullptr, &num, nullptr);
	m_extensions.resize(num);
	inst->vkEnumerateDeviceExtensionProperties(m_handle, nullptr, &num, m_extensions.data());
	inst->vkEnumerateDeviceLayerProperties(m_handle, &num, nullptr);
	m_layers.resize(num);
	inst->vkEnumerateDeviceLayerProperties(m_handle, &num, m_layers.data());
	inst->vkGetPhysicalDeviceQueueFamilyProperties(m_handle, &num, nullptr);
	m_queue_families.resize(num);
	inst->vkGetPhysicalDeviceQueueFamilyProperties(m_handle, &num, m_queue_families.data());
	inst->vkGetPhysicalDeviceMemoryProperties(m_handle, &m_memory_properties);
}
