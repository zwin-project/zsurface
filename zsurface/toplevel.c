#include <stdio.h>
#include <zsurface.h>

#include "internal.h"

#define PIXEL_SCALE 1000.0f       // 1000px = 1m
#define SURFACE_THICKNESS 0.001f  // 1mm
#define CUBOID_PADDING 0.05f      // 5cm

static void
cuboid_window_configure(void* data, struct zgn_cuboid_window* cuboid_window,
    uint32_t serial, struct wl_array* cuboid_half_size)
{
  struct zsurf_toplevel* toplevel = data;
  vec2 view_half_size, zero = GLM_VEC2_ZERO_INIT;

  zgn_cuboid_window_ack_configure(cuboid_window, serial);
  if (cuboid_half_size->size != sizeof(float) * 3) {
    fprintf(stderr,
        "zsurface: cuboid window half_size was given with invalid size\n");
    return;
  }

  {
    float* cuboid_half_size_vec = cuboid_half_size->data;
    view_half_size[0] = cuboid_half_size_vec[0] - CUBOID_PADDING;
    view_half_size[1] = cuboid_half_size_vec[1] - CUBOID_PADDING;
  }

  zsurf_view_update_space_geom(toplevel->view, view_half_size, zero);
}

static const struct zgn_cuboid_window_listener cuboid_window_listener = {
    .configure = cuboid_window_configure,
};

static void
view_commit_handler(struct zsurf_listener* listener, void* data)
{
  UNUSED(data);
  struct zsurf_toplevel* toplevel =
      wl_container_of(listener, toplevel, view_commit_listener);

  if (toplevel->view->state == ZSURF_VIEW_STATE_FIRST_TEXTURE_ATTACHED) {
    struct wl_array half_size;
    wl_array_init(&half_size);
    float* half_size_vec = wl_array_add(&half_size, sizeof(float) * 3);
    half_size_vec[0] =
        (float)toplevel->view->surface_geometry.width / 2.0f / PIXEL_SCALE +
        CUBOID_PADDING;
    half_size_vec[1] =
        (float)toplevel->view->surface_geometry.height / 2.0f / PIXEL_SCALE +
        CUBOID_PADDING;
    half_size_vec[2] = SURFACE_THICKNESS;

    toplevel->cuboid_window = zgn_shell_get_cuboid_window(
        toplevel->surface_display->shell, toplevel->virtual_object, &half_size);
    zgn_cuboid_window_add_listener(
        toplevel->cuboid_window, &cuboid_window_listener, toplevel);
    wl_array_release(&half_size);
  }

  zgn_virtual_object_commit(toplevel->virtual_object);

  if (toplevel->view->state != ZSURF_VIEW_STATE_NO_TEXTURE)
    toplevel->view->state = ZSURF_VIEW_STATE_TEXTURE_COMMITTED;
}

WL_EXPORT struct zsurf_view*
zsurf_toplevel_pick_view(struct zsurf_toplevel* toplevel, vec3 ray_origin,
    vec3 ray_direction, vec2 local_coord)
{
  float mul = -ray_origin[2] / ray_direction[2];
  struct zsurf_view* view = toplevel->view;
  if (mul <= 0) return NULL;

  float x = ray_origin[0] + ray_direction[0] * mul;
  float y = ray_origin[1] + ray_direction[1] * mul;
  float w0 = view->space_geometry.center[0] - view->space_geometry.half_size[0];
  float w1 = view->space_geometry.center[0] + view->space_geometry.half_size[0];
  float h0 = view->space_geometry.center[1] - view->space_geometry.half_size[1];
  float h1 = view->space_geometry.center[1] + view->space_geometry.half_size[1];
  if (w0 < x && x < w1 && h0 < y && y < h1) {
    local_coord[0] = (x - w0) * view->surface_geometry.width / (w1 - w0);
    local_coord[1] = (h1 - y) * view->surface_geometry.height / (h1 - h0);
    return toplevel->view;
  }
  return NULL;
}

WL_EXPORT struct zsurf_view*
zsurf_toplevel_get_view(struct zsurf_toplevel* topelevel)
{
  return topelevel->view;
}

WL_EXPORT void
zsurf_toplevel_move(struct zsurf_toplevel* toplevel, uint32_t serial)
{
  if (toplevel->cuboid_window)
    zgn_cuboid_window_move(
        toplevel->cuboid_window, toplevel->surface_display->seat, serial);
}

WL_EXPORT struct zsurf_toplevel*
zsurf_toplevel_create(
    struct zsurf_display* surface_display, void* view_user_data)
{
  struct zsurf_toplevel* toplevel;
  struct zsurf_view* view;
  struct zgn_virtual_object* virtual_object;

  toplevel = zalloc(sizeof *toplevel);
  if (toplevel == NULL) goto err;

  virtual_object =
      zgn_compositor_create_virtual_object(surface_display->compositor);

  toplevel->surface_display = surface_display;
  toplevel->virtual_object = virtual_object;
  zgn_virtual_object_set_user_data(virtual_object, toplevel);
  toplevel->cuboid_window = NULL;

  view = zsurf_view_create(surface_display, toplevel, view_user_data);
  if (view == NULL) goto err_view;

  toplevel->view_commit_listener.notify = view_commit_handler;
  zsurf_signal_add(&view->commit_signal, &toplevel->view_commit_listener);

  zsurf_signal_init(&toplevel->destroy_signal);

  toplevel->view = view;

  return toplevel;

err_view:
  free(toplevel);

err:
  return NULL;
}

WL_EXPORT void
zsurf_toplevel_destroy(struct zsurf_toplevel* toplevel)
{
  zsurf_signal_emit(&toplevel->destroy_signal, NULL);
  zsurf_view_destroy(toplevel->view);
  zgn_virtual_object_destroy(toplevel->virtual_object);
  free(toplevel);
}
