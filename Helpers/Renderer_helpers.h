/*
 * render_utils.h
 *
 *  Created on: Mar 8, 2025
 *      Author: tugrul
 */

#ifndef INCLUDE_RENDER_UTILS_H_
#define INCLUDE_RENDER_UTILS_H_

// Function pointer type: takes no args, returns void
typedef void (*func_t)(void);

unsigned int renderer_get_width();
unsigned int renderer_get_height();

int init_renderer(func_t init_f, func_t draw_f, func_t clean_f);

int render_loop();

void free_renderer();

#endif /* INCLUDE_RENDER_UTILS_H_ */
