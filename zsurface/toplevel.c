#include <z11-client-protocol.h>
#include <zsurface.h>

#include "internal.h"

const float padding = 5;

static void zsurface_toplevel_protocol_configure(void* data,
    struct z11_cuboid_window* cuboid_window, wl_fixed_t width,
    wl_fixed_t height, wl_fixed_t depth)
{
  UNUSED(cuboid_window);
  UNUSED(depth);
  struct zsurface_toplevel* toplevel = data;
  zsurface_view_resize(toplevel->view, wl_fixed_to_double(width) - padding,
      wl_fixed_to_double(height) - padding);
}

static const struct z11_cuboid_window_listener cuboid_window_listener = {
    .configure = zsurface_toplevel_protocol_configure,
};

struct zsurface_toplevel* zsurface_toplevel_create(
    struct zsurface* surface, struct zsurface_toplevel_option option)
{
  struct zsurface_toplevel* toplevel;

  toplevel = zalloc(sizeof *toplevel);
  if (toplevel == NULL) goto out;

  toplevel->surface = surface;

  toplevel->virtual_object =
      z11_compositor_create_virtual_object(surface->compositor);

  toplevel->cuboid_window =
      z11_shell_get_cuboid_window(surface->shell, toplevel->virtual_object);
  z11_cuboid_window_add_listener(
      toplevel->cuboid_window, &cuboid_window_listener, toplevel);

  z11_cuboid_window_request_window_size(toplevel->cuboid_window,
      wl_fixed_from_double(option.width + padding),
      wl_fixed_from_double(option.height + padding),
      wl_fixed_from_double(padding));

  toplevel->view = zsurface_view_create(toplevel, 0, 0);
  if (toplevel->view == NULL) goto out_toplevel;

  return toplevel;

out_toplevel:
  z11_cuboid_window_destroy(toplevel->cuboid_window);
  z11_virtual_object_destroy(toplevel->virtual_object);
  free(toplevel);

out:
  return NULL;
}

void zsurface_toplevel_destroy(struct zsurface_toplevel* toplevel)
{
  zsurface_view_destroy(toplevel->view);
  z11_cuboid_window_destroy(toplevel->cuboid_window);
  z11_virtual_object_destroy(toplevel->virtual_object);
  free(toplevel);
}

struct zsurface_view* zsurface_toplevel_get_view(
    struct zsurface_toplevel* toplevel)
{
  return toplevel->view;
}
