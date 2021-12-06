#ifndef ZSURFACE_INTERNAL_H
#define ZSURFACE_INTERNAL_H

#include <cglm/cglm.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client.h>
#include <zigen-client-protocol.h>
#include <zigen-opengl-client-protocol.h>
#include <zigen-shell-client-protocol.h>
#include <zsurface.h>

struct zsurf_listener;

typedef void (*zsurf_notify_func_t)(
    struct zsurf_listener* listener, void* data);

struct zsurf_listener {
  struct wl_list link;
  zsurf_notify_func_t notify;
};

struct zsurf_signal {
  struct wl_list listener_list;
};

void zsurf_log(const char* fmt, ...);

static inline void
zsurf_signal_init(struct zsurf_signal* signal)
{
  wl_list_init(&signal->listener_list);
}

static inline void
zsurf_signal_add(struct zsurf_signal* signal, struct zsurf_listener* listener)
{
  wl_list_insert(signal->listener_list.prev, &listener->link);
}

static inline void
zsurf_signal_emit(struct zsurf_signal* signal, void* data)
{
  struct zsurf_listener *l, *next;

  wl_list_for_each_safe(l, next, &signal->listener_list, link)
      l->notify(l, data);
}

static inline int
glm_vec3_from_wl_array(vec3 v, struct wl_array* array)
{
  float* data = array->data;
  if (array->size != sizeof(vec3)) return -1;
  memcpy(v, data, sizeof(vec3));
  return 0;
}

static inline void
glm_vec3_to_wl_array(vec3 v, struct wl_array* array)
{
  if (array->alloc > 0) {
    wl_array_release(array);
    wl_array_init(array);
  }
  float* data = wl_array_add(array, sizeof(vec3));
  memcpy(data, v, sizeof(vec3));
}

#define UNUSED(x) ((void)x)

static inline void*
zalloc(size_t size)
{
  return calloc(1, size);
}

struct vertex {
  vec3 p;
  vec2 uv;
};

struct triangle {
  struct vertex vertices[3];
};

struct view_rect {
  struct triangle triangles[2];
};

enum zsurf_view_state {
  ZSURF_VIEW_STATE_NO_TEXTURE = 0,
  ZSURF_VIEW_STATE_FIRST_TEXTURE_ATTACHED = 1,
  ZSURF_VIEW_STATE_NEW_TEXTURE_ATTACHED = 2,
  ZSURF_VIEW_STATE_TEXTURE_COMMITTED = 3,
};

struct zsurf_view {
  void* user_data;
  struct zsurf_display* surface_display;
  struct zsurf_toplevel* toplevel;
  enum zsurf_view_state state;

  struct {
    vec2 half_size;
    vec2 center;
  } space_geometry;

  struct {
    uint32_t width;
    uint32_t height;
  } surface_geometry;

  int fd, vertex_shader_fd, fragment_shader_fd;
  void* shm_data;
  size_t shm_size;
  struct wl_shm_pool* pool;

  struct zgn_opengl_component* component;

  struct zgn_opengl_vertex_buffer* vertex_buffer;
  struct wl_buffer* vertex_buffer_buffer;
  struct view_rect* vertex_data;

  struct zgn_opengl_shader_program* shader;  // TODO: share shaders across views

  struct zgn_opengl_texture* texture;
  struct wl_buffer* texture_buffer;
  struct zsurf_color_bgra* texture_data;

  struct zsurf_signal commit_signal;
  struct zsurf_signal destroy_signal;
};

void zsurf_view_update_space_geom(
    struct zsurf_view* view, vec2 half_size, vec2 center);

struct zsurf_view* zsurf_view_create(struct zsurf_display* surface_display,
    struct zsurf_toplevel* toplevel, void* user_data);

void zsurf_view_destroy(struct zsurf_view* view);

struct zsurf_toplevel {
  struct zsurf_display* surface_display;
  struct zsurf_view* view;

  struct zgn_virtual_object* virtual_object;
  struct zgn_cuboid_window* cuboid_window;  // null at the beginning

  struct zsurf_listener view_commit_listener;
  struct zsurf_signal destroy_signal;
};

struct zsurf_view* zsurf_toplevel_pick_view(struct zsurf_toplevel* toplevel,
    vec3 ray_origin, vec3 ray_direction, vec2 local_coord);

struct zsurf_display {
  const struct zsurf_display_interface* interaface;
  void* user_data;

  struct wl_display* display;
  struct wl_registry* registry;
  struct zgn_compositor* compositor;
  struct zgn_seat* seat;
  struct zgn_shell* shell;
  struct wl_shm* shm;
  struct zgn_opengl* opengl;

  struct zgn_ray* ray;            // nullable
  struct zgn_keyboard* keyboard;  // nullable

  struct zsurf_toplevel* focus_toplevel;
  struct zsurf_listener focus_toplevel_destroy_listener;

  struct zsurf_view* focus_view;
  struct zsurf_listener focus_view_destroy_listener;
};

#endif  //  ZSURFACE_INTERNAL_H
