#include "vk_local.hh"

vk::device::device(instance_ptr inst, physical_device const & pdev) : m_instance(inst), m_pdev(pdev) {
	
}

vk::device::~device() {
	if (m_handle) this->vkDestroyDevice(m_handle, nullptr);
}
