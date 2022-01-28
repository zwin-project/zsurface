#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <zsurface.h>

#define WIDTH 256
#define HEIGHT 256

struct app {
  struct zsurf_display *display;
  struct zsurf_toplevel *toplevel;
  struct zsurf_color_bgra texture[WIDTH][HEIGHT];
  struct {
    uint32_t x;
    uint32_t y;
    bool enter;
    bool button;
  } pointer;
};

static void draw(struct app *app);
static void next(struct app *app);

static void
seat_capability(void *data, uint32_t capabilities)
{
  (void)data;
  (void)capabilities;
}

static void
pointer_enter(void *data, struct zsurf_view *view, float x, float y)
{
  (void)view;
  struct app *app = data;
  app->pointer.enter = true;
  app->pointer.x = x;
  app->pointer.y = y;
}

static void
pointer_motion(void *data, uint32_t time, float x, float y)
{
  (void)time;
  struct app *app = data;
  app->pointer.x = x;
  app->pointer.y = y;
}

static void
pointer_leave(void *data, struct zsurf_view *view)
{
  (void)view;
  struct app *app = data;
  app->pointer.enter = false;
  app->pointer.button = false;
}

static void
pointer_button(void *data, uint32_t serial, uint32_t time, uint32_t button,
    enum zsurf_pointer_button_state state)
{
  struct app *app = data;
  (void)serial;
  (void)time;
  (void)button;
  if (state == ZSURF_POINTER_BUTTON_STATE_PRESSED)
    app->pointer.button = true;
  else
    app->pointer.button = false;

  int32_t dx = app->pointer.x - WIDTH / 2;
  int32_t dy = app->pointer.y - HEIGHT / 2;
  if (dx * dx + dy * dy < 1600) {
    zsurf_toplevel_move(app->toplevel, serial);
  }
}

static void
keyboard_keymap(void *data, uint32_t format, int fd, uint32_t size)
{
  (void)data;
  (void)format;
  (void)fd;
  (void)size;
}

static void
keyboard_enter(void *data, uint32_t serial, struct zsurf_view *view,
    uint32_t *keys, uint32_t key_count)
{
  (void)data;
  (void)serial;
  (void)view;
  (void)keys;
  (void)key_count;
}

static void
keyboard_leave(void *data, uint32_t serial, struct zsurf_view *view)
{
  (void)data;
  (void)serial;
  (void)view;
}

static void
keyboard_key(
    void *data, uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
  (void)data;
  (void)serial;
  (void)time;
  (void)key;
  (void)state;
}

static void
keyboard_modifiers(void *data, uint32_t serial, uint32_t mods_depressed,
    uint32_t mods_latched, uint32_t mods_locked, uint32_t group)
{
  (void)data;
  (void)serial;
  (void)mods_depressed;
  (void)mods_latched;
  (void)mods_locked;
  (void)group;
}

static void
frame(void *data, uint32_t callback_time)
{
  (void)callback_time;
  struct app *app = data;
  draw(app);
  next(app);
}

static void
draw_first(struct app *app)
{
  for (int y = 0; y < HEIGHT; y++) {
    for (int x = 0; x < WIDTH; x++) {
      app->texture[y][x].a = UINT8_MAX;
      app->texture[y][x].r = UINT8_MAX;
      app->texture[y][x].g = UINT8_MAX;
      app->texture[y][x].b = UINT8_MAX;
    }
  }
}

static void
draw(struct app *app)
{
  struct zsurf_view *view = zsurf_toplevel_get_view(app->toplevel);

  struct zsurf_color_bgra *pixel = app->texture[0];
  for (int y = 0; y < HEIGHT; y++) {
    for (int x = 0; x < WIDTH; x++) {
      float dx = (float)app->pointer.x - x;
      float dy = (float)app->pointer.y - y;
      if (app->pointer.enter && (dx * dx + dy * dy) < 64) {
        if (app->pointer.button) {
          pixel->g = 0;
          pixel->b = 0;
        } else {
          pixel->r = 0;
          pixel->g = 0;
          pixel->b = 0;
        }
      } else {
        pixel->r = pixel->r == UINT8_MAX ? UINT8_MAX : pixel->r + 1;
        pixel->g = pixel->g == UINT8_MAX ? UINT8_MAX : pixel->g + 1;
        pixel->b = pixel->b == UINT8_MAX ? UINT8_MAX : pixel->b + 1;
      }
      pixel++;
    }
  }

  zsurf_view_set_texture(view, app->texture[0], WIDTH, HEIGHT);
}

static void
next(struct app *app)
{
  struct zsurf_view *view = zsurf_toplevel_get_view(app->toplevel);

  zsurf_view_add_frame_callback(view, frame, app);
  zsurf_view_commit(view);
}

static const struct zsurf_display_interface display_interface = {
    .seat_capabilities = seat_capability,
    .pointer_enter = pointer_enter,
    .pointer_leave = pointer_leave,
    .pointer_motion = pointer_motion,
    .pointer_button = pointer_button,
    .keyboard_keymap = keyboard_keymap,
    .keyboard_enter = keyboard_enter,
    .keyboard_leave = keyboard_leave,
    .keyboard_key = keyboard_key,
    .keyboard_modifiers = keyboard_modifiers,
};

static int
init(struct app *app)
{
  app->display = zsurf_display_create("zigen-0", &display_interface, app);

  if (app->display == NULL) {
    fprintf(stderr, "failed to connect to zigen server\n");
    return -1;
  }

  app->toplevel = zsurf_toplevel_create(app->display, &app);

  app->pointer.enter = false;
  app->pointer.button = false;
  app->pointer.x = 0;
  app->pointer.y = 0;
  return 0;
}

int
main()
{
  struct app app;

  if (init(&app) != 0) {
    return EXIT_FAILURE;
  }

  draw_first(&app);
  draw(&app);
  next(&app);

  while (zsurf_display_dispatch(app.display) != -1)
    ;

  return EXIT_SUCCESS;
}
