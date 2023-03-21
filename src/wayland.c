#include "backend.h"
#include "xdg-shell.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include <wayland-util.h>

#include <xkbcommon/xkbcommon.h>

struct scterm_wl_backend {
	scterm_backend_t impl;
	struct wl_display *display;
	struct wl_registry *registry; 

	struct wl_compositor *compositor;
	struct wl_shm *shm; 
	struct wl_surface *surface; 

	//Input 
	struct wl_seat *seat;
	struct wl_pointer *pointer;
	struct wl_keyboard *keyboard;
	struct wl_touch *touchpad;

	//xdg code 
	struct xdg_wm_base *xdg_wm_base;
	struct xdg_surface *xdg_surface;
	struct xdg_toplevel *xdg_toplevel;

	//H & W 
	uint32_t width;
	uint32_t height;
	uint32_t close; 

	//XKB 
	struct xkb_context *xkb_context;
	struct xkb_keymap *xkb_keymap; 
	struct xkb_state *xkb_state;
};

void scterm_wl_keyboard_keymap(void *data, struct wl_keyboard *keyboard, uint32_t keymap_format, int32_t km_fd, uint32_t km_size) {
	struct scterm_wl_backend *wl = data;
	printf("Keyboard keymap fd %d, size: %d\n", km_fd, km_size);

	void *keymap_data = mmap(0, km_size, PROT_READ, MAP_PRIVATE, km_fd, 0);
	
	wl->xkb_keymap = xkb_keymap_new_from_string(wl->xkb_context, keymap_data, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
	
	wl->xkb_state = xkb_state_new(wl->xkb_keymap);

	munmap(keymap_data, km_size);
	close(km_fd);
}

void scterm_wl_keyboard_key(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t key_state) {

	if(WL_KEYBOARD_KEY_STATE_RELEASED) {
		return;
	}
	struct scterm_wl_backend *wl = data;
	char buffer[16];
	key += 8;
	
	xkb_state_key_get_utf8(wl->xkb_state, key, buffer, 16);

	printf("%c", buffer[0]);
	fflush(stdout);
}

void scterm_wl_keyboard_enter(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface, struct wl_array *keys) {
	printf("Keyboard Enter\n");
}

void scterm_wl_keyboard_leave(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface) {
	printf("Keyboard Leave\n");
}

void scterm_wl_keyboard_repeat_info(void *data, struct wl_keyboard *keyboard, int32_t rate, int32_t delay) {
	printf("keyboard Repeat Info\n");	
}

void scterm_wl_keyboard_modifiers(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group) {
	struct scterm_wl_backend *wl = data;
	printf("Keyboard Mods\n");
	xkb_state_update_mask(wl->xkb_state, mods_depressed, mods_latched, mods_locked, 0, 0, 0);
}

struct wl_keyboard_listener wl_keyboard_listener = {
	.keymap = scterm_wl_keyboard_keymap,
	.key = scterm_wl_keyboard_key,
	.enter = scterm_wl_keyboard_enter, 
	.leave = scterm_wl_keyboard_leave,
	.repeat_info = scterm_wl_keyboard_repeat_info,
	.modifiers = scterm_wl_keyboard_modifiers, 
};

void scterm_wl_seat_name(void *data, struct wl_seat *wl_seat,
		const char *name) {
	printf("wl_seat_name: %s\n", name);
	
}

void scterm_wl_seat_capabilities(void *data, struct wl_seat *wl_seat, uint32_t capabilities) {
	struct scterm_wl_backend *wl = data;
	printf("wl_seat capabilities: %d\n"
			"WL_POINTER: %d\n"
			"WL_KEYBOARD: %d\n"
			"WL_TOUCH: %d\n", capabilities,
			WL_SEAT_CAPABILITY_POINTER & capabilities,
			WL_SEAT_CAPABILITY_KEYBOARD & capabilities,
			WL_SEAT_CAPABILITY_TOUCH & capabilities);

	if(WL_SEAT_CAPABILITY_POINTER & capabilities) {
		wl->pointer = wl_seat_get_pointer(wl_seat);
	} 
	if(WL_SEAT_CAPABILITY_KEYBOARD & capabilities) {
		wl->keyboard = wl_seat_get_keyboard(wl_seat);
	
		wl_keyboard_add_listener(wl->keyboard, &wl_keyboard_listener, data);
	}
	if(WL_SEAT_CAPABILITY_TOUCH & capabilities) {
		wl->touchpad = wl_seat_get_touch(wl_seat);
	}
}

static const struct wl_seat_listener wl_seat_listener = { 
	.name = scterm_wl_seat_name,
	.capabilities = scterm_wl_seat_capabilities,
};

void scterm_destroy_wl_backend(scterm_backend_t *backend) {
	struct scterm_wl_backend *wl = (void *)backend; 

	wl_registry_destroy(wl->registry);

	wl_display_disconnect(wl->display);

	free(wl);
}

void xdg_wm_base_ping(void *data, struct xdg_wm_base *base,
		uint32_t serial) {
	xdg_wm_base_pong(base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
	.ping = xdg_wm_base_ping,
};

void wl_registry_global(void *data, struct wl_registry *registry,
		uint32_t name, const char *interface, uint32_t version) {
	struct scterm_wl_backend *wl = data;

	printf("%s: version %d\n", interface, version);
	
	if(strcmp(interface, wl_compositor_interface.name) == 0) {
		wl->compositor = wl_registry_bind(registry, name, &wl_compositor_interface,
				version);
	} else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
		wl->xdg_wm_base = wl_registry_bind(registry, name, &xdg_wm_base_interface,
				version);
		xdg_wm_base_add_listener(wl->xdg_wm_base, &xdg_wm_base_listener, wl);
		
	} else if (strcmp(interface, wl_shm_interface.name) == 0) {
		wl->shm = wl_registry_bind(registry, name, &wl_shm_interface, version);
	} else if(strcmp(interface, wl_seat_interface.name) == 0) {
		wl->seat = wl_registry_bind(registry, name, 
				&wl_seat_interface, version);
		wl_seat_add_listener(wl->seat, &wl_seat_listener, wl);
	}
}

void wl_registry_global_remove(void *data, 
		struct wl_registry *registry, uint32_t name) {
	//Unused 
}

struct wl_registry_listener registry_listener = {
	.global_remove = wl_registry_global_remove, 
	.global = wl_registry_global, 
};



void  scterm_xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial) {
	struct scterm_wl_backend *wl = data;
	printf("%d %d\n", wl->width, wl->height);
	
	//HACK: Error checking
	int fd = shm_open("scterm", O_CREAT | O_RDWR | O_EXCL, 0x600);
	
	//Resize file 
	ftruncate(fd, wl->height * wl->width * 4);
	
	struct wl_shm_pool *pool = wl_shm_create_pool(wl->shm, fd, wl->height * wl->width * 4);
	
	struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0, wl->width, wl->height, wl->width * 4, 
			WL_SHM_FORMAT_ARGB8888);

	uint32_t *map = mmap(NULL, wl->height * wl->width * 4, 
			PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);


	for(uint32_t x = 0; x < wl->width; ++x) {
		for(uint32_t y = 0; y < wl->height; ++y) {
			map[x + (y * wl->width)] = 0xe0282a36;
		}
	}

	munmap(map, wl->height * wl->width * 4);

	shm_unlink("scterm");
	close(fd);

	xdg_surface_ack_configure(xdg_surface, serial);
	
	wl_surface_attach(wl->surface, buffer, 0, 0);

	wl_surface_commit(wl->surface);
}

static const struct xdg_surface_listener xdg_surface_listener = {
		.configure = scterm_xdg_surface_configure,
};

void scterm_xdg_toplevel_configure(void *data, struct xdg_toplevel *toplevel, int32_t width, int32_t height, struct wl_array *states) {
	struct scterm_wl_backend *wl = data; 

	wl->width = width == 0 ? 640 : width;
	wl->height = height == 0 ? 320 : height;
}

void scterm_xdg_toplevel_configure_bounds(void *data, struct xdg_toplevel *toplevel, int32_t width, int32_t height) {
	struct scterm_wl_backend *wl = data;

	wl->width = width == 0 ? 640 : width;
	wl->height = height == 0 ? 320 : height;
}

void scterm_xdg_toplevel_close(void *data, struct xdg_toplevel *toplevel) {
	struct scterm_wl_backend *wl = data;

	

	wl->close = 1;
}

void scterm_xdg_toplevel_wm_caps(void *data, struct xdg_toplevel *toplevel, struct wl_array *capabilites) {
	//BLANK 
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
	.configure = scterm_xdg_toplevel_configure,
	.close = scterm_xdg_toplevel_close, 
	.configure_bounds = scterm_xdg_toplevel_configure_bounds,
	.wm_capabilities = scterm_xdg_toplevel_wm_caps,
};

scterm_backend_t *scterm_create_wl_backend() {
	struct scterm_wl_backend *wl = calloc(1, sizeof(struct scterm_wl_backend));

	wl->display = wl_display_connect(NULL);

	wl->registry = wl_display_get_registry(wl->display);
	wl_registry_add_listener(wl->registry, &registry_listener,wl);	


	wl_display_roundtrip(wl->display);
	if(!wl->compositor) {
		printf("wl_display_roundtrip: failed to find needed globals\n");
		return NULL;
	}

	wl->surface = wl_compositor_create_surface(wl->compositor);
	wl->xdg_surface = xdg_wm_base_get_xdg_surface(wl->xdg_wm_base, wl->surface);
	wl->xdg_toplevel = xdg_surface_get_toplevel(wl->xdg_surface);
	
	xdg_surface_add_listener(wl->xdg_surface, &xdg_surface_listener, wl);

	wl->height = 320;
	wl->width = 640;

	xdg_toplevel_add_listener(wl->xdg_toplevel, &xdg_toplevel_listener, wl);

	wl->impl.destroy = scterm_destroy_wl_backend;

	wl->xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

	return (void *)wl;
}
