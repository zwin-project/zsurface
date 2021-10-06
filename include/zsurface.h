#ifndef ZSURFACE_H
#define ZSURFACE_H

#include <stdint.h>

/* view */

struct zsurface_view;

struct zsurface_color_bgra {
  uint8_t b, g, r, a;
};

typedef void (*zsurface_view_frame_callback_func_t)(
    void* data, uint32_t callback_time);

void zsurface_view_add_frame_callback(struct zsurface_view* view,
    zsurface_view_frame_callback_func_t done_func, void* data);

struct zsurface_color_bgra* zsurface_view_get_texture_data(
    struct zsurface_view* view);

uint32_t zsurface_view_get_texture_width(struct zsurface_view* view);

uint32_t zsurface_view_get_texture_height(struct zsurface_view* view);

// return -1 when failed to trancate a shared memory file
int zsurface_view_resize_texture(
    struct zsurface_view* view, uint32_t width, uint32_t height);

void zsurface_view_commit(struct zsurface_view* view);

/* zsurface toplevel */

struct zsurface_toplevel;

struct zsurface_view* zsurface_toplevel_get_view(
    struct zsurface_toplevel* toplevel);

/* zsurface */

struct zsurface;

struct zsurface_interface {
  void (*seat_capability)(
      void* data, struct zsurface* surface, uint32_t capability);
  void (*pointer_enter)(
      void* data, struct zsurface_view* view, float view_x, float view_y);
  void (*pointer_motion)(void* data, float view_x, float view_y);
  void (*pointer_leave)(void* data);
};

struct zsurface* zsurface_create(
    const char* socket, void* data, const struct zsurface_interface* interface);

void zsurface_destroy(struct zsurface* surface);

// return 0 if ok
int zsurface_check_globals(struct zsurface* surface);

struct zsurface_toplevel* zsurface_create_toplevel_view(
    struct zsurface* surface, float width, float height);

void zsurface_destroy_toplevel_view(
    struct zsurface* surface, struct zsurface_toplevel* toplevel);

int zsurface_prepare_read(struct zsurface* surface);

int zsurface_dispatch_pending(struct zsurface* surface);

int zsurface_flush(struct zsurface* surface);

void zsurface_cancel_read(struct zsurface* surface);

int zsurface_read_events(struct zsurface* surface);

int zsurface_get_fd(struct zsurface* surface);

int zsurface_dispatch(struct zsurface* surface);

/* error */

enum zsurface_error {
  ZSURFACE_ERR_NONE = 0,
  ZSURFACE_ERR_COMPOSITOR_NOT_SUPPORTED = 1,
};

enum zsurface_error zsurface_get_error();

#endif  //  ZSURFACE_H
