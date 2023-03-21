#pragma once

typedef struct scterm_renderer scterm_renderer_t;

struct scterm_renderer {
	void (*destroy)(scterm_renderer_t *renderer);
	void (*draw_frame)(scterm_renderer_t *renderer);
};
