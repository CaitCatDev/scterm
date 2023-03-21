#pragma once

typedef struct scterm_backend scterm_backend_t;

#define VK_USE_PLATFORM_WAYLAND_KHR
#define VK_USE_PLATFORM_XCB_KHR
#include <vulkan/vulkan.h>

struct scterm_backend {
	void (*destroy)(scterm_backend_t *backend);
	void (*map)(scterm_backend_t *backend);
	VkSurfaceKHR *(*create_vk_surface)(scterm_backend_t *backend);
};

scterm_backend_t *scterm_create_backend();
