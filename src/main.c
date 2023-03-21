#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "backend.h"
#include "renderer.h"

scterm_backend_t *scterm_create_wl_backend();

scterm_backend_t *scterm_create_backend() {
	scterm_backend_t *backend = NULL;
	if(getenv("WAYLAND_DISPLAY")) {
		backend = scterm_create_wl_backend();		
	}

	return backend;
}


scterm_renderer_t *scterm_vk_renderer_create(scterm_backend_t *backend);


int main() {
	
	scterm_backend_t *backend = scterm_create_backend();

	scterm_vk_renderer_create(backend);

	backend->destroy(backend);
}
