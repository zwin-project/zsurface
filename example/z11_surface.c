#include <stdio.h>
#include <stdlib.h>
#include <zsurface.h>

static void seat_capability(
    struct zsurface* surface, void* data, uint32_t capability)
{
  (void)surface;
  (void)data;
  (void)capability;
}

static struct zsurface_interface interface = {
    .seat_capability = seat_capability,
};

int main()
{
  struct zsurface* surface = zsurface_create("z11-0", NULL, &interface);
  struct zsurface_toplevel_option option = {
      .width = 40.0,
      .height = 30.0,
  };

  if (surface == NULL) return EXIT_FAILURE;
  if (zsurface_check_globals(surface) != 0) {
    zsurface_destroy(surface);
    return EXIT_FAILURE;
  }
  zsurface_create_toplevel_view(surface, option);
  zsurface_run(surface);
  return 0;
}
