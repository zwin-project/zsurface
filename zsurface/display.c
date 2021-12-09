#include <string.h>
#include <wayland-client.h>
#include <zsurface.h>

#include "internal.h"

static void
shm_format(void *data, struct wl_shm *shm, enum wl_shm_format format)
{
  UNUSED(data);
  UNUSED(shm);
  UNUSED(format);
}

static const struct wl_shm_listener shm_listener = {
    .format = shm_format,
};

static void
focus_toplevel_destroy_handler(struct zsurf_listener *listener, void *data)
{
  UNUSED(data);
  struct zsurf_display *surface_display = wl_container_of(
      listener, surface_display, focus_toplevel_destroy_listener);

  wl_list_init(&surface_display->focus_toplevel_destroy_listener.link);
  surface_display->focus_toplevel = NULL;
}

static void
focus_view_destroy_handler(struct zsurf_listener *listener, void *data)
{
  UNUSED(data);
  struct zsurf_display *surface_display =
      wl_container_of(listener, surface_display, focus_view_destroy_listener);

  wl_list_init(&surface_display->focus_view_destroy_listener.link);
  surface_display->focus_view = NULL;
}

static void
ray_enter(void *data, struct zgn_ray *ray, uint32_t serial,
    struct zgn_virtual_object *virtual_object, struct wl_array *origin,
    struct wl_array *direction)
{
  UNUSED(ray);
  UNUSED(serial);
  UNUSED(origin);
  UNUSED(direction);
  struct zsurf_display *surface_display = data;
  struct zsurf_toplevel *toplevel;

  toplevel = zgn_virtual_object_get_user_data(virtual_object);

  if (surface_display->focus_toplevel)
    wl_list_remove(&surface_display->focus_toplevel_destroy_listener.link);

  surface_display->focus_toplevel = toplevel;

  zsurf_signal_add(&toplevel->destroy_signal,
      &surface_display->focus_toplevel_destroy_listener);
}

static void
ray_leave(void *data, struct zgn_ray *ray, uint32_t serial,
    struct zgn_virtual_object *virtual_object)
{
  UNUSED(ray);
  UNUSED(serial);
  UNUSED(virtual_object);
  struct zsurf_display *surface_display = data;

  if (surface_display->focus_toplevel) {
    wl_list_remove(&surface_display->focus_toplevel_destroy_listener.link);
    wl_list_init(&surface_display->focus_toplevel_destroy_listener.link);
    surface_display->focus_toplevel = NULL;
  }

  if (surface_display->focus_view) {
    surface_display->interaface->pointer_leave(
        surface_display->user_data, surface_display->focus_view);
    wl_list_remove(&surface_display->focus_view_destroy_listener.link);
    wl_list_init(&surface_display->focus_view_destroy_listener.link);
    surface_display->focus_view = NULL;
  }
}

static void
ray_motion(void *data, struct zgn_ray *ray, uint32_t time,
    struct wl_array *origin, struct wl_array *direction)
{
  UNUSED(ray);
  struct zsurf_display *surface_display = data;
  struct zsurf_toplevel *toplevel;
  struct zsurf_view *view;
  vec3 ray_origin, ray_direction;
  vec2 local_coord;

  toplevel = surface_display->focus_toplevel;
  if (toplevel == NULL) return;

  glm_vec3_from_wl_array(ray_origin, origin);
  glm_vec3_from_wl_array(ray_direction, direction);

  view = zsurf_toplevel_pick_view(
      toplevel, ray_origin, ray_direction, local_coord);

  if (surface_display->focus_view && surface_display->focus_view != view) {
    surface_display->interaface->pointer_leave(
        surface_display->user_data, surface_display->focus_view);
    wl_list_remove(&surface_display->focus_view_destroy_listener.link);
    wl_list_init(&surface_display->focus_view_destroy_listener.link);
    if (surface_display->cursor.view) {
      zsurf_view_destroy(surface_display->cursor.view);
      surface_display->cursor.view = NULL;
    }
  }

  if (view && surface_display->focus_view != view) {
    surface_display->interaface->pointer_enter(
        surface_display->user_data, view, local_coord[0], local_coord[1]);
    zsurf_signal_add(
        &view->destroy_signal, &surface_display->focus_view_destroy_listener);
  }

  if (view) {
    glm_vec2_copy(local_coord, surface_display->cursor.local_coord);
    surface_display->interaface->pointer_motion(
        surface_display->user_data, time, local_coord[0], local_coord[1]);

    if (surface_display->cursor.view) {
      zsurf_view_update_surface_pos(surface_display->cursor.view,
          surface_display->cursor.local_coord[0] -
              surface_display->cursor.hotspot_x,
          surface_display->cursor.local_coord[1] -
              surface_display->cursor.hotspot_y);
      zsurf_view_update_space_geom(surface_display->cursor.view);
      zgn_virtual_object_commit(
          surface_display->cursor.view->toplevel->virtual_object);
    }
  }

  surface_display->focus_view = view;
}

static void
ray_button(void *data, struct zgn_ray *ray, uint32_t serial, uint32_t time,
    uint32_t button, uint32_t state)
{
  UNUSED(ray);
  struct zsurf_display *surface_display = data;

  surface_display->interaface->pointer_button(
      surface_display->user_data, serial, time, button, state);
}

static const struct zgn_ray_listener ray_listener = {
    .enter = ray_enter,
    .leave = ray_leave,
    .motion = ray_motion,
    .button = ray_button,
};

static void
keyboard_keymap(void *data, struct zgn_keyboard *keyboard, uint32_t format,
    int32_t fd, uint32_t size)
{
  UNUSED(keyboard);
  struct zsurf_display *surface_display = data;

  // TODO: Handle the case wayland keymap format enum and zigen keymap format
  // enum are not same.

  surface_display->interaface->keyboard_keymap(
      surface_display->user_data, format, fd, size);
}

static void
keyboard_enter(void *data, struct zgn_keyboard *keyboard, uint32_t serial,
    struct zgn_virtual_object *virtual_object, struct wl_array *keys)
{
  UNUSED(keyboard);
  struct zsurf_display *surface_display = data;
  struct zsurf_toplevel *toplevel;
  uint32_t key_count = keys->size / (sizeof(uint32_t));

  toplevel = zgn_virtual_object_get_user_data(virtual_object);

  // FIXME: use zsurf_display.focus_view instead of toplevel->view if
  // focus_view.toplevel == toplevel. we need to keep keyboard focus view then.
  surface_display->interaface->keyboard_enter(surface_display->user_data,
      serial, toplevel->view, keys->data, key_count);
}

static void
keyboard_leave(void *data, struct zgn_keyboard *keyboard, uint32_t serial,
    struct zgn_virtual_object *virtual_object)
{
  UNUSED(keyboard);
  struct zsurf_display *surface_display = data;
  struct zsurf_toplevel *toplevel;

  toplevel = zgn_virtual_object_get_user_data(virtual_object);

  surface_display->interaface->keyboard_leave(
      surface_display->user_data, serial, toplevel->view);
}

static void
keyboard_key(void *data, struct zgn_keyboard *keyboard, uint32_t serial,
    uint32_t time, uint32_t key, uint32_t state)
{
  UNUSED(keyboard);
  struct zsurf_display *surface_display = data;

  surface_display->interaface->keyboard_key(
      surface_display->user_data, serial, time, key, state);
}

static void
keyboard_modifiers(void *data, struct zgn_keyboard *keyboard, uint32_t serial,
    uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked,
    uint32_t group)
{
  UNUSED(data);
  UNUSED(keyboard);
  UNUSED(serial);
  UNUSED(mods_depressed);
  UNUSED(mods_latched);
  UNUSED(mods_locked);
  UNUSED(group);
}

static const struct zgn_keyboard_listener keyboard_listener = {
    .keymap = keyboard_keymap,
    .enter = keyboard_enter,
    .leave = keyboard_leave,
    .key = keyboard_key,
    .modifiers = keyboard_modifiers,
};

static void
seat_capabilities(void *data, struct zgn_seat *seat, uint32_t capabilities)
{
  struct zsurf_display *surface_display = data;

  if (capabilities & ZGN_SEAT_CAPABILITY_RAY) {
    if (surface_display->ray == NULL) {
      surface_display->ray = zgn_seat_get_ray(seat);
      zgn_ray_add_listener(
          surface_display->ray, &ray_listener, surface_display);
    }
  } else {
    if (surface_display->ray) {
      zgn_ray_destroy(surface_display->ray);
      surface_display->ray = NULL;
    }

    if (surface_display->focus_view) {
      surface_display->interaface->pointer_leave(
          surface_display->user_data, surface_display->focus_view);
      wl_list_remove(&surface_display->focus_view_destroy_listener.link);
      wl_list_init(&surface_display->focus_view_destroy_listener.link);
      surface_display->focus_view = NULL;
    }

    if (surface_display->focus_toplevel) {
      wl_list_remove(&surface_display->focus_toplevel_destroy_listener.link);
      wl_list_init(&surface_display->focus_toplevel_destroy_listener.link);
      surface_display->focus_toplevel = NULL;
    }

    if (surface_display->cursor.view) {
      zsurf_view_destroy(surface_display->cursor.view);
      surface_display->cursor.view = NULL;
    }
  }

  if (capabilities & ZGN_SEAT_CAPABILITY_KEYBOARD) {
    if (surface_display->keyboard == NULL) {
      surface_display->keyboard = zgn_seat_get_keyboard(seat);
      zgn_keyboard_add_listener(
          surface_display->keyboard, &keyboard_listener, surface_display);
    }
  } else {
    if (surface_display->keyboard) {
      zgn_keyboard_destroy(surface_display->keyboard);
      surface_display->keyboard = NULL;
    }
  }

  surface_display->interaface->seat_capabilities(
      surface_display->user_data, capabilities);
}

static const struct zgn_seat_listener seat_listener = {
    .capabilities = seat_capabilities,
};

static void
zsurf_global_registry_handler(void *data, struct wl_registry *registry,
    uint32_t id, const char *interface, uint32_t version)
{
  struct zsurf_display *surface_display = data;

  if (strcmp(interface, "zgn_compositor") == 0) {
    surface_display->compositor =
        wl_registry_bind(registry, id, &zgn_compositor_interface, version);
  } else if (strcmp(interface, "zgn_seat") == 0) {
    surface_display->seat =
        wl_registry_bind(registry, id, &zgn_seat_interface, version);
    zgn_seat_add_listener(
        surface_display->seat, &seat_listener, surface_display);
  } else if (strcmp(interface, "zgn_shell") == 0) {
    surface_display->shell =
        wl_registry_bind(registry, id, &zgn_shell_interface, version);
  } else if (strcmp(interface, "wl_shm") == 0) {
    surface_display->shm =
        wl_registry_bind(registry, id, &wl_shm_interface, version);
    wl_shm_add_listener(surface_display->shm, &shm_listener, surface_display);
  } else if (strcmp(interface, "zgn_opengl") == 0) {
    surface_display->opengl =
        wl_registry_bind(registry, id, &zgn_opengl_interface, version);
  }
}

static void
zsurf_global_registry_remover(
    void *data, struct wl_registry *registry, uint32_t id)
{
  UNUSED(data);
  UNUSED(registry);
  UNUSED(id);
}

static const struct wl_registry_listener registry_listener = {
    .global = zsurf_global_registry_handler,
    .global_remove = zsurf_global_registry_remover,
};

WL_EXPORT void
zsurf_display_set_cursor(struct zsurf_display *surface_display,
    struct zsurf_color_bgra *data, uint32_t width, uint32_t height,
    int32_t hotspot_x, int32_t hotspot_y)
{
  if (!surface_display->cursor.view) {
    if (!surface_display->focus_view) return;
    surface_display->cursor.view = zsurf_view_create(surface_display,
        surface_display->focus_view->toplevel, surface_display->focus_view,
        NULL);
  } else if (data == NULL) {
    zsurf_view_destroy(surface_display->cursor.view);
    surface_display->cursor.view = NULL;
    return;
  }

  surface_display->cursor.hotspot_x = hotspot_x;
  surface_display->cursor.hotspot_y = hotspot_y;

  zsurf_view_set_texture(surface_display->cursor.view, data, width, height);

  zsurf_view_update_surface_pos(surface_display->cursor.view,
      surface_display->cursor.local_coord[0] - hotspot_x,
      surface_display->cursor.local_coord[1] - hotspot_y);

  zsurf_view_update_space_geom(surface_display->cursor.view);

  zsurf_view_commit(surface_display->cursor.view);
}

WL_EXPORT struct zsurf_display *
zsurf_display_create(const char *socket,
    const struct zsurf_display_interface *interface, void *user_data)
{
  struct zsurf_display *surface_display;

  surface_display = zalloc(sizeof *surface_display);
  if (surface_display == NULL) goto err;

  surface_display->interaface = interface;
  surface_display->user_data = user_data;

  surface_display->display = wl_display_connect(socket);
  if (surface_display->display == NULL) goto err_display;

  surface_display->registry = wl_display_get_registry(surface_display->display);
  if (surface_display->registry == NULL) goto err_registry;

  surface_display->ray = NULL;
  surface_display->keyboard = NULL;
  surface_display->focus_toplevel = NULL;
  surface_display->focus_toplevel_destroy_listener.notify =
      focus_toplevel_destroy_handler;
  wl_list_init(&surface_display->focus_toplevel_destroy_listener.link);

  surface_display->focus_view = NULL;
  surface_display->focus_view_destroy_listener.notify =
      focus_view_destroy_handler;
  wl_list_init(&surface_display->focus_view_destroy_listener.link);

  wl_registry_add_listener(
      surface_display->registry, &registry_listener, surface_display);

  wl_display_roundtrip(surface_display->display);

  if (!(surface_display->compositor && surface_display->seat &&
          surface_display->shell && surface_display->shm &&
          surface_display->opengl)) {
    goto err_globals;
  }

  return surface_display;

err_globals:
err_registry:
  wl_display_disconnect(surface_display->display);

err_display:
  free(surface_display);

err:
  return NULL;
}

WL_EXPORT void
zsurf_display_destroy(struct zsurf_display *surface_display)
{
  if (surface_display->cursor.view)
    zsurf_view_destroy(surface_display->cursor.view);
  wl_list_remove(&surface_display->focus_toplevel_destroy_listener.link);
  wl_list_remove(&surface_display->focus_view_destroy_listener.link);
  wl_display_disconnect(surface_display->display);
  free(surface_display);
}

WL_EXPORT int
zsurf_display_prepare_read(struct zsurf_display *surface_display)
{
  return wl_display_prepare_read(surface_display->display);
}

WL_EXPORT int
zsurf_display_dispatch_pending(struct zsurf_display *surface_display)
{
  return wl_display_dispatch_pending(surface_display->display);
}

WL_EXPORT int
zsurf_display_flush(struct zsurf_display *surface_display)
{
  return wl_display_flush(surface_display->display);
}

WL_EXPORT void
zsurf_display_cancel_read(struct zsurf_display *surface_display)
{
  wl_display_cancel_read(surface_display->display);
}

WL_EXPORT int
zsurf_display_read_events(struct zsurf_display *surface_display)
{
  return wl_display_read_events(surface_display->display);
}

WL_EXPORT int
zsurf_display_get_fd(struct zsurf_display *surface_display)
{
  return wl_display_get_fd(surface_display->display);
}

WL_EXPORT int
zsurf_display_dispatch(struct zsurf_display *surface_display)
{
  return wl_display_dispatch(surface_display->display);
}
