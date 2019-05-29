#pragma once

#include "qcommon/q_shared.hh"

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#define VK_GLOBAL_FN(name) PFN_vk##name name

#define EXECUTOR_VK_DEBUG_MODE 1

namespace vk {
	
	extern PFN_vkGetInstanceProcAddr GetInstanceProcAddr;
	void resolve_globals(PFN_vkGetInstanceProcAddr);
	
	struct instance;
	struct physical_device;
	struct device;
	
	using instance_ptr = std::shared_ptr<instance>;
	using device_ptr = std::shared_ptr<device>;
	
	// ================================
	// INSTANCE
	// ================================
	
	struct instance final {
		
		#define VK_FN_IDECL
		#include "vk.inl"
		
		~instance();
		static instance_ptr create(std::vector<std::string> const & extensions, std::vector<std::string> const & layers, uint32_t version);
		
		inline std::vector<physical_device> const & physical_devices() const { return m_physical_devices; }
	private:
		
		VkInstance handle = nullptr;
		std::vector<physical_device> m_physical_devices;
		
		instance() = default;
		instance(instance const &) = delete;
		instance(instance &&) = delete;
		
		void resolve_symbols();
		void enumerate_physical_devices();
	};
	
	// ================================
	// PHYSICAL DEVICE
	// ================================
	
	struct physical_device {
		physical_device(instance const *, VkPhysicalDevice);
		~physical_device() = default;
		
		inline VkPhysicalDeviceProperties const & properties() const { return m_properties; }
	private:
		VkPhysicalDevice m_handle;
		VkPhysicalDeviceProperties m_properties;
		std::vector<VkExtensionProperties> m_extensions;
		std::vector<VkLayerProperties> m_layers;
		std::vector<VkQueueFamilyProperties> m_queue_families;
		VkPhysicalDeviceMemoryProperties m_memory_properties;
	};
	
	// ================================
	// LOGICAL DEVICE
	// ================================
	
	struct device {
		
		#define VK_FN_DDECL
		#include "vk.inl"
		
		~device();
		static device_ptr create(instance_ptr, physical_device const &);
	private:
		VkDevice m_handle;
		instance_ptr m_instance;
		physical_device const & m_pdev;
		
		device(instance_ptr, physical_device const &);
	};
	
	#define VK_FN_GEDECL
	#include "vk.inl"
};

#undef VK_GLOBAL_FN
