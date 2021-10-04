#define _GNU_SOURCE

#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "internal.h"

static const char *vertex_shader;
static const char *fragment_shader;

static int create_shared_fd(off_t size)
{
  const char *name = "zsurface-base";

  int fd = memfd_create(name, MFD_CLOEXEC | MFD_ALLOW_SEALING);
  if (fd < 0) return fd;
  unlink(name);

  if (ftruncate(fd, size) < 0) {
    close(fd);
    return -1;
  }

  return fd;
}

static void zsurface_view_update_vertex_buffer(struct zsurface_view *view)
{
  struct vertex A = {{-view->width / 2, -view->height / 2, 0}, {0, 1}};
  struct vertex B = {{+view->width / 2, -view->height / 2, 0}, {1, 1}};
  struct vertex C = {{+view->width / 2, +view->height / 2, 0}, {1, 0}};
  struct vertex D = {{-view->width / 2, +view->height / 2, 0}, {0, 0}};

  view->vertex_data->triangles[0].vetices[0] = A;
  view->vertex_data->triangles[0].vetices[1] = B;
  view->vertex_data->triangles[0].vetices[2] = C;
  view->vertex_data->triangles[1].vetices[0] = A;
  view->vertex_data->triangles[1].vetices[1] = D;
  view->vertex_data->triangles[1].vetices[2] = C;
}

void zsurface_view_resize(struct zsurface_view *view, float width, float height)
{
  view->width = width;
  view->height = height;

  zsurface_view_update_vertex_buffer(view);
  z11_opengl_vertex_buffer_attach(
      view->vertex_buffer, view->vertex_raw_buffer, sizeof(struct vertex));
  z11_virtual_object_commit(view->toplevel->virtual_object);
}

struct zsurface_view *zsurface_view_create(
    struct zsurface_toplevel *toplevel, float width, float height)
{
  struct zsurface_view *view;
  uint32_t texture_size = 0;

  view = zalloc(sizeof *view);
  if (view == NULL) goto out;

  view->toplevel = toplevel;
  view->width = width;
  view->height = height;
  view->texture_width = 100;
  view->texture_height = 100;
  texture_size =
      sizeof(struct color_bgra) * view->texture_width * view->texture_height;

  view->fd = create_shared_fd(sizeof(struct view_rect) + texture_size);

  view->shm_data_len = sizeof(struct view_rect) + texture_size;
  view->shm_data = mmap(NULL, view->shm_data_len, PROT_READ | PROT_WRITE,
      MAP_SHARED, view->fd, 0);
  view->vertex_data = view->shm_data;
  view->texture_data =
      (void *)((uint8_t *)view->shm_data + sizeof(struct view_rect));
  memset(view->texture_data, UINT8_MAX, texture_size);

  {
    struct wl_shm_pool *pool = wl_shm_create_pool(
        toplevel->surface->shm, view->fd, view->shm_data_len);
    view->vertex_raw_buffer =
        wl_zext_shm_pool_create_raw_buffer(pool, 0, sizeof(struct view_rect));
    view->texture_raw_buffer = wl_zext_shm_pool_create_raw_buffer(
        pool, sizeof(struct view_rect), texture_size);
    wl_shm_pool_destroy(pool);
  }

  view->render_component =
      z11_opengl_render_component_manager_create_opengl_render_component(
          toplevel->surface->render_component_manager,
          toplevel->virtual_object);
  view->vertex_buffer = z11_opengl_create_vertex_buffer(toplevel->surface->gl);
  view->shader = z11_opengl_create_shader_program(
      toplevel->surface->gl, vertex_shader, fragment_shader);
  view->texture = z11_opengl_create_texture_2d(toplevel->surface->gl);

  z11_opengl_render_component_attach_vertex_buffer(
      view->render_component, view->vertex_buffer);
  z11_opengl_render_component_attach_texture_2d(
      view->render_component, view->texture);
  z11_opengl_render_component_attach_shader_program(
      view->render_component, view->shader);
  z11_opengl_render_component_append_vertex_input_attribute(
      view->render_component, 0,
      Z11_OPENGL_VERTEX_INPUT_ATTRIBUTE_FORMAT_FLOAT_VECTOR3,
      offsetof(struct vertex, point));
  z11_opengl_render_component_append_vertex_input_attribute(
      view->render_component, 1,
      Z11_OPENGL_VERTEX_INPUT_ATTRIBUTE_FORMAT_FLOAT_VECTOR2,
      offsetof(struct vertex, uv));
  z11_opengl_render_component_set_topology(
      view->render_component, Z11_OPENGL_TOPOLOGY_TRIANGLES);

  zsurface_view_update_vertex_buffer(view);

  z11_opengl_texture_2d_set_image(view->texture, view->texture_raw_buffer,
      Z11_OPENGL_TEXTURE_2D_FORMAT_ARGB8888, view->texture_width,
      view->texture_height);
  z11_opengl_vertex_buffer_attach(
      view->vertex_buffer, view->vertex_raw_buffer, sizeof(struct vertex));

  z11_virtual_object_commit(toplevel->virtual_object);

  return view;

out:
  return NULL;
}

void zsurface_view_destroy(struct zsurface_view *view)
{
  wl_raw_buffer_destroy(view->texture_raw_buffer);
  z11_opengl_texture_2d_destroy(view->texture);
  z11_opengl_shader_program_destroy(view->shader);
  wl_raw_buffer_destroy(view->vertex_raw_buffer);
  z11_opengl_vertex_buffer_destroy(view->vertex_buffer);
  z11_opengl_render_component_destroy(view->render_component);
  munmap(view->shm_data, view->shm_data_len);
  close(view->fd);
  free(view);
}

static const char *vertex_shader =
    "#version 410\n"
    "uniform mat4 mvp;\n"
    "layout(location = 0) in vec4 position;\n"
    "layout(location = 1) in vec2 v2UVcoordsIn;\n"
    "layout(location = 2) in vec3 v3NormalIn;\n"
    "out vec2 v2UVcoords;\n"
    "void main()\n"
    "{\n"
    "  v2UVcoords = v2UVcoordsIn;\n"
    "  gl_Position = mvp * position;\n"
    "}\n";

static const char *fragment_shader =
    "#version 410 core\n"
    "uniform sampler2D userTexture;\n"
    "in vec2 v2UVcoords;\n"
    "out vec4 outputColor;\n"
    "void main()\n"
    "{\n"
    "  outputColor = texture(userTexture, v2UVcoords);\n"
    "}\n";
