#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <zsurface.h>

typedef struct {
  struct zsurface* surface;
  struct zsurface_toplevel* toplevel;
  struct zsurface_view* enter_view;
  float view_width, view_height;
  uint32_t texture_width, texture_height;
  uint32_t pointer_x, pointer_y;
  int has_pointer;
} App;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
static void seat_capability(
    void* data, struct zsurface* surface, uint32_t capability)
{}

static void pointer_leave(void* data, struct zsurface_view* view)
{
  App* app = data;
  app->has_pointer = 0;
  app->enter_view = NULL;
}

static void pointer_enter(
    void* data, struct zsurface_view* view, uint32_t x, uint32_t y)
{
  App* app = data;
  app->has_pointer = 1;
  app->pointer_x = x;
  app->pointer_y = y;
  app->enter_view = view;
}

static void pointer_motion(void* data, uint32_t x, uint32_t y)
{
  App* app = data;
  app->pointer_x = x;
  app->pointer_y = y;
}

static void pointer_button(void* data, uint32_t button, uint32_t state) {}

static void keyboard_keymap(void* data, uint32_t format, int fd, uint32_t size)
{}

static void keyboard_enter(
    void* data, struct zsurface_view* view, struct wl_array* keys)
{}

static void keyboard_leave(void* data, struct zsurface_view* view) {}

static void keyboard_key(void* data, uint32_t key, uint32_t state) {}
#pragma GCC diagnostic pop

static struct zsurface_interface interface = {
    .seat_capability = seat_capability,

    .pointer_leave = pointer_leave,
    .pointer_enter = pointer_enter,
    .pointer_motion = pointer_motion,
    .pointer_button = pointer_button,

    .keyboard_keymap = keyboard_keymap,
    .keyboard_enter = keyboard_enter,
    .keyboard_leave = keyboard_leave,
    .keyboard_key = keyboard_key,
};

static App* app_create(uint32_t width, uint32_t height)
{
  App* app;

  app = calloc(1, sizeof *app);
  if (app == NULL) goto out;
  app->texture_width = width;
  app->texture_height = height;
  app->view_width = (float)width / 10.0;
  app->view_height = (float)height / 10.0;
  app->has_pointer = 0;

  app->surface = zsurface_create("z11-0", app, &interface);
  if (app->surface == NULL) goto out_app;

  if (zsurface_check_globals(app->surface) != 0) goto out_surface;

  app->toplevel = zsurface_create_toplevel_view(app->surface);
  if (app->toplevel == NULL) goto out_surface;

  zsurface_toplevel_resize(app->toplevel, app->view_width, app->view_height);

  app->enter_view = NULL;

  return app;

out_surface:
  zsurface_destroy(app->surface);

out_app:
  free(app);

out:
  return NULL;
}

void app_paint(App* app)
{
  uint32_t cursor = 0;
  struct zsurface_view* view = zsurface_toplevel_get_view(app->toplevel);
  struct zsurface_color_bgra* data = zsurface_view_get_texture_buffer(
      view, app->texture_width, app->texture_height);

  for (uint32_t y = 0; y < app->texture_height; y++) {
    for (uint32_t x = 0; x < app->texture_width; x++) {
      uint32_t r = (x - app->pointer_x) * (x - app->pointer_x) +
                   (y - app->pointer_y) * (y - app->pointer_y);
      if (app->has_pointer && r < 256 && app->enter_view == view) {
        data[cursor].a = UINT8_MAX;
        data[cursor].b = 3;
        data[cursor].g = 3;
        data[cursor].r = 3;
      } else {
        data[cursor].a = UINT8_MAX;
        data[cursor].b =
            data[cursor].b < UINT8_MAX ? data[cursor].b + 4 : UINT8_MAX;
        data[cursor].g =
            data[cursor].g < UINT8_MAX ? data[cursor].g + 4 : UINT8_MAX;
        data[cursor].r =
            data[cursor].r < UINT8_MAX ? data[cursor].r + 4 : UINT8_MAX;
      }
      cursor++;
    }
  }
}

void app_next_frame(void* data, uint32_t callback_time)
{
  (void)callback_time;
  App* app = data;
  struct zsurface_view* view = zsurface_toplevel_get_view(app->toplevel);
  app_paint(app);
  zsurface_view_add_frame_callback(view, app_next_frame, app);
  zsurface_view_commit(view);
}

int main()
{
  App* app = app_create(512, 256);
  if (app == NULL) return EXIT_FAILURE;
  struct zsurface_view* view = zsurface_toplevel_get_view(app->toplevel);
  app_paint(app);
  zsurface_view_add_frame_callback(view, app_next_frame, app);
  zsurface_view_commit(view);

  int ret;
  struct pollfd pfd[1];
  pfd[0].fd = zsurface_get_fd(app->surface);
  pfd[0].events = POLLIN;

  while (1) {
    while (zsurface_prepare_read(app->surface) == -1) {
      if (errno != EAGAIN) return EXIT_FAILURE;
      if (zsurface_dispatch_pending(app->surface) == -1) return EXIT_FAILURE;
    }

    while (zsurface_flush(app->surface) == -1) {
      if (errno != EAGAIN) {
        zsurface_cancel_read(app->surface);
        return EXIT_FAILURE;
      }
    }

    // you can add other file descriptors to poll if needed
    do {
      ret = poll(pfd, 1, -1);
    } while (ret == -1 && errno == EINTR);

    if (ret == -1) {
      zsurface_cancel_read(app->surface);
      return EXIT_FAILURE;
    }

    if (zsurface_read_events(app->surface) == -1) return EXIT_FAILURE;
    if (zsurface_dispatch_pending(app->surface) == -1) return EXIT_FAILURE;
  }

  // almost the same loop if polling only the zsurface display file descriptor
  // while (zsurface_dispatch(app->surface) != -1)
  //   ;
}
