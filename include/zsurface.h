#ifndef ZSURFACE_H
#define ZSURFACE_H

#include <stdint.h>

/* view */

struct zsurface_view;

/* zsurface */

struct zsurface;

struct zsurface_interface {
  void (*seat_capability)(
      struct zsurface* surface, void* data, uint32_t capability);
};

struct zsurface* zsurface_create(
    const char* socket, void* data, struct zsurface_interface* interface);

void zsurface_destroy(struct zsurface* surface);

// return 0 if ok
int zsurface_check_globals(struct zsurface* surface);

struct zsurface_toplevel_option {
  float width;
  float height;
};

struct zsurface_view* zsurface_create_toplevel_view(
    struct zsurface* surface, struct zsurface_toplevel_option option);

void zsurface_remove_toplevel_view(struct zsurface* surface);

struct zsurface_view* zsurface_get_toplevel_view(struct zsurface* surface);

void zsurface_run(struct zsurface* surface);

/* error */

enum zsurface_error {
  ZSURFACE_ERR_NONE = 0,
  ZSURFACE_ERR_COMPOSITOR_NOT_SUPPORTED = 1,
};

enum zsurface_error zsurface_get_error();

#endif  //  ZSURFACE_H
