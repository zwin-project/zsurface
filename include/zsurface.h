#ifndef ZSURFACE_H
#define ZSURFACE_H

#include <stdint.h>

enum zsurf_seat_capability {
  ZSURF_SEAT_CAPABILITY_POINTER = 1,
  ZSURF_SEAT_CAPABILITY_KEYBOARD = 2,
};

enum zsurf_pointer_button_state {
  ZSURF_POINTER_BUTTON_STATE_RELEASED = 0,
  ZSURF_POINTER_BUTTON_STATE_PRESSED = 1,
};

struct zsurf_view;

struct zsurf_toplevel;

struct zsurf_display;

struct zsurf_color_bgra {
  uint8_t b, g, r, a;
};

typedef void (*zsurf_view_frame_callback_func_t)(
    void* data, uint32_t callback_time);

void* zsurf_view_get_user_data(struct zsurf_view* view);

void zsurf_view_add_frame_callback(struct zsurf_view* view,
    zsurf_view_frame_callback_func_t done_func, void* data);

/**
 * return -1 when railed to truncate a shared memory file
 */
int zsurf_view_set_texture(struct zsurf_view* view,
    struct zsurf_color_bgra* data, uint32_t width, uint32_t height);

/**
 * return NULL when failed to trancate a shared memory file
 */
struct zsurf_color_bgra* zsurf_view_get_texture_buffer(
    struct zsurf_view* view, uint32_t width, uint32_t height);

void zsurf_view_commit(struct zsurf_view* view);

struct zsurf_view* zsurf_toplevel_get_view(struct zsurf_toplevel* topelevel);

void zsurf_toplevel_move(struct zsurf_toplevel* toplevel, uint32_t serial);

struct zsurf_toplevel* zsurf_toplevel_create(
    struct zsurf_display* surface_display, void* view_user_data);

void zsurf_toplevel_destroy(struct zsurf_toplevel* toplevel);

struct zsurf_display_interface {
  void (*seat_capabilities)(void* data, uint32_t capabilities);

  void (*pointer_enter)(void* data, struct zsurf_view* view, float x, float y);
  void (*pointer_motion)(void* data, uint32_t time, float x, float y);
  void (*pointer_leave)(void* data, struct zsurf_view* view);
  void (*pointer_button)(void* data, uint32_t serial, uint32_t time,
      uint32_t button, enum zsurf_pointer_button_state state);

  void (*keyboard_keymap)(void* data, uint32_t format, int fd, uint32_t size);
  void (*keyboard_enter)(void* data, uint32_t serial, struct zsurf_view* view,
      uint32_t* keys, uint32_t key_count);
  void (*keyboard_leave)(void* data, uint32_t serial, struct zsurf_view* view);
  void (*keyboard_key)(
      void* data, uint32_t serial, uint32_t time, uint32_t key, uint32_t state);
};

struct zsurf_display* zsurf_display_create(const char* socket,
    const struct zsurf_display_interface* interface, void* user_data);

void zsurf_display_destroy(struct zsurf_display* z_display);

int zsurf_display_prepare_read(struct zsurf_display* z_display);

int zsurf_display_dispatch_pending(struct zsurf_display* z_display);

int zsurf_display_flush(struct zsurf_display* z_display);

void zsurf_display_cancel_read(struct zsurf_display* z_display);

int zsurf_display_read_events(struct zsurf_display* z_display);

int zsurf_display_get_fd(struct zsurf_display* z_display);

int zsurf_display_dispatch(struct zsurf_display* z_display);

#endif  //  ZSURFACE_H
