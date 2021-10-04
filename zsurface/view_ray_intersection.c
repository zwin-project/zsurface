#include <cglm/cglm.h>

#include "internal.h"

struct view_ray_intersection_result view_ray_intersection(
    vec3 origin, vec3 direction, struct wl_list* view_list)
{
  // FIXME: use z index
  struct view_ray_intersection_result result = {
      .view = NULL,
      .view_x = 0,
      .view_y = 0,
  };

  float mul = -origin[2] / direction[2];
  if (mul <= 0) {
    return result;
  }

  float x = origin[0] + direction[0] * mul;
  float y = origin[1] + direction[1] * mul;

  struct zsurface_view* view;
  wl_list_for_each(view, view_list, link)
  {
    float w = view->width / 2;
    float h = view->height / 2;
    if (-w < x && x < w && -h < y && y < h) {
      result.view = view;
      result.view_x = w + x;
      result.view_y = h - y;
      return result;
    }
  }
  return result;
}
