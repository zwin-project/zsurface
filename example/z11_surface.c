#include <stdio.h>
#include <stdlib.h>
#include <zsurface.h>

typedef struct {
  struct zsurface* surface;
  struct zsurface_view* top_level;
  float view_width, view_height;
  uint32_t texture_width, texture_height;
  uint8_t blue;
} App;

static void seat_capability(
    struct zsurface* surface, void* data, uint32_t capability)
{
  App* app = data;
  (void)app;
  (void)surface;
  (void)capability;
}

static struct zsurface_interface interface = {
    .seat_capability = seat_capability,
};

static App* app_create(uint32_t width, uint32_t height)
{
  App* app;
  struct zsurface_toplevel_option option;

  app = calloc(1, sizeof *app);
  if (app == NULL) return NULL;
  app->texture_width = width;
  app->texture_height = height;
  app->view_width = (float)width / 10.0;
  app->view_height = (float)height / 10.0;
  app->blue = 0;

  option.width = app->view_width;
  option.height = app->view_height;
  app->surface = zsurface_create("z11-0", app, &interface);
  if (app->surface == NULL) return NULL;
  if (zsurface_check_globals(app->surface) != 0) {
    zsurface_destroy(app->surface);
    return NULL;
  }
  app->top_level = zsurface_create_toplevel_view(app->surface, option);
  zsurface_view_resize_texture(
      app->top_level, app->texture_width, app->texture_height);

  return app;
}

void app_paint(App* app)
{
  uint32_t cursor = 0;
  struct zsurface_color_bgra* data =
      zsurface_view_get_texture_data(app->top_level);

  for (uint32_t y = 0; y < app->texture_height; y++) {
    for (uint32_t x = 0; x < app->texture_width; x++) {
      data[cursor].a = UINT8_MAX;
      data[cursor].b = app->blue;
      data[cursor].g = x * UINT8_MAX / app->texture_width;
      data[cursor].r = UINT8_MAX - x * UINT8_MAX / app->texture_width;
      cursor++;
    }
  }
}

void app_next_frame(void* data, uint32_t callback_time)
{
  (void)callback_time;
  App* app = data;
  if (app->blue == UINT8_MAX) {
    app->blue = 0;
  } else {
    app->blue += 1;
  }
  app_paint(app);
  zsurface_view_add_frame_callback(app->top_level, app_next_frame, app);
  zsurface_view_commit(app->top_level);
}

int main()
{
  App* app = app_create(512, 256);
  if (app == NULL) return EXIT_FAILURE;
  app_paint(app);
  zsurface_view_add_frame_callback(app->top_level, app_next_frame, app);
  zsurface_view_commit(app->top_level);
  zsurface_run(app->surface);
  return 0;
}
