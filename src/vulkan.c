#include "renderer.h"
#include "backend.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define VK_USE_PLATFORM_WAYLAND_KHR
#define VK_USE_PLATFORM_XCB_KHR
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

typedef struct scterm_vk_renderer {
	scterm_renderer_t impl;
	VkInstance instance;
	VkDebugUtilsMessengerEXT debug_msg; 

} scterm_vk_renderer_t;

const char *vk_layers[] = { 
	"VK_LAYER_KHRONOS_validation",
};

const char *vk_extensions[] = {
	VK_KHR_SURFACE_EXTENSION_NAME,
	VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
	VK_KHR_XCB_SURFACE_EXTENSION_NAME,
};

VKAPI_ATTR VkBool32 VKAPI_CALL scterm_vk_debug_callback(
		VkDebugUtilsMessageSeverityFlagBitsEXT severity,
		VkDebugUtilsMessageTypeFlagsEXT type, 
		const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
		void *user_data) {
	printf("Validation Layer: %s \n", callback_data->pMessage);
	return VK_FALSE;
}

VkResult scterm_vk_create_instance(VkInstance *instance) {
	VkInstanceCreateInfo create_info = { 0 };
	VkDebugUtilsMessengerCreateInfoEXT debug_info = { 0 };

	debug_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debug_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debug_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debug_info.pfnUserCallback = scterm_vk_debug_callback;

	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.enabledLayerCount = sizeof(vk_layers) / sizeof(vk_layers[0]);
	create_info.ppEnabledLayerNames = vk_layers;
	create_info.enabledExtensionCount = sizeof(vk_extensions) / sizeof(vk_extensions[0]);
	create_info.ppEnabledExtensionNames = vk_extensions;
	create_info.pNext = &debug_info;

	return vkCreateInstance(&create_info, NULL, instance);
}

VkResult CreateDebugUtilsMessengerExt(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *create_info, const VkAllocationCallbacks *allocator, VkDebugUtilsMessengerEXT *debug_messenger) {
	PFN_vkCreateDebugUtilsMessengerEXT function = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

	if(function) {
		return function(instance, create_info, allocator, debug_messenger);
	}

	return VK_ERROR_EXTENSION_NOT_PRESENT;
}

VkResult scterm_vk_create_debug_messenger(VkInstance instance,
		VkDebugUtilsMessengerEXT *messenger) {
	VkDebugUtilsMessengerCreateInfoEXT debug_info = { 0 };

	debug_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debug_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debug_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debug_info.pfnUserCallback = scterm_vk_debug_callback;

	return CreateDebugUtilsMessengerExt(instance, &debug_info, 
		NULL, messenger);
}

VkResult scterm_vk_get_phydev(VkInstance instance) {
	uint32_t dev_count = 0;
	VkPhysicalDevice *devices = NULL;

	vkEnumeratePhysicalDevices(instance, &dev_count, NULL);

	devices = calloc(dev_count, sizeof(VkPhysicalDevice));

	vkEnumeratePhysicalDevices(instance, &dev_count, devices);

	for(int i = 0; i < dev_count; ++i) {
		VkPhysicalDeviceProperties deviceProperties;
    	VkPhysicalDeviceFeatures deviceFeatures;
    	vkGetPhysicalDeviceProperties(devices[i], &deviceProperties);
 	    vkGetPhysicalDeviceFeatures(devices[i], &deviceFeatures);
		
	}
	return 0;
}

scterm_renderer_t *scterm_vk_renderer_create(scterm_backend_t *backend) {
	scterm_vk_renderer_t *vk = calloc(1, sizeof(*vk));

	scterm_vk_create_instance(&vk->instance);
 	scterm_vk_create_debug_messenger(vk->instance, &vk->debug_msg);
	scterm_vk_get_phydev(vk->instance);	

	vkDestroyInstance(vk->instance, NULL);

	return (void *)vk;
}
