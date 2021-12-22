#define _GNU_SOURCE

#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <zsurface.h>

#include "internal.h"

static const char* vertex_shader;
static const char* fragment_shader;

struct zsurf_view_callback_data {
  zsurf_view_frame_callback_func_t func;
  void* data;
};

static int
create_shared_fd(loff_t size)
{
  const char* name = "zsurface-base";

  int fd = memfd_create(name, MFD_CLOEXEC | MFD_ALLOW_SEALING);
  if (fd < 0) return fd;
  unlink(name);

  if (ftruncate(fd, size) < 0) {
    close(fd);
    return -1;
  }

  return fd;
}

static int
create_shared_text_fd(const char* text, loff_t size)
{
  int fd = create_shared_fd(size);
  if (fd < 0) return fd;

  void* data = mmap(NULL, size, PROT_WRITE, MAP_SHARED, fd, 0);
  if (data == MAP_FAILED) {
    close(fd);
    return -1;
  }
  memcpy(data, text, size);
  munmap(data, size);

  return fd;
}

static int
zsurf_view_resize_texture(
    struct zsurf_view* view, uint32_t width, uint32_t height)
{
  size_t vertex_buffer_size, texture_size, shm_size;
  if (width == view->surface_geometry.width &&
      height == view->surface_geometry.height)
    return 0;

  if (width * height <=
      view->surface_geometry.width * view->surface_geometry.height) {
    view->surface_geometry.width = width;
    view->surface_geometry.height = height;
  } else {
    texture_size = sizeof(struct zsurf_color_bgra) * width * height;
    vertex_buffer_size = sizeof(struct view_rect);
    shm_size = vertex_buffer_size + texture_size;

    if (ftruncate(view->fd, shm_size) < 0) return -1;

    wl_shm_pool_resize(view->pool, shm_size);

    view->surface_geometry.width = width;
    view->surface_geometry.height = height;
    view->shm_data =
        mremap(view->shm_data, view->shm_size, shm_size, MREMAP_MAYMOVE);
    if (view->shm_data == MAP_FAILED) return -1;

    view->shm_size = shm_size;
    view->vertex_data = view->shm_data;
    view->texture_data = (struct zsurf_color_bgra*)((uint8_t*)view->shm_data +
                                                    vertex_buffer_size);
  }

  wl_buffer_destroy(view->texture_buffer);
  view->texture_buffer =
      wl_shm_pool_create_buffer(view->pool, vertex_buffer_size, width, height,
          sizeof(struct zsurf_color_bgra) * width, WL_SHM_FORMAT_ARGB8888);
  zgn_opengl_texture_attach_2d(view->texture, view->texture_buffer);
  zgn_opengl_component_attach_texture(view->component, view->texture);

  return 0;
}

static void
zsurf_view_parent_geometry_handler(struct zsurf_listener* listener, void* data)
{
  UNUSED(data);
  struct zsurf_view* view;

  view = wl_container_of(listener, view, parent_geometry_listener);

  zsurf_view_update_space_geom(view);
}

WL_EXPORT void
zsurf_view_update_surface_pos(struct zsurf_view* view, int32_t sx, int32_t sy)
{
  view->surface_geometry.sx = sx;
  view->surface_geometry.sy = sy;
}

WL_EXPORT void
zsurf_view_update_space_geom(struct zsurf_view* view)
{
  vec2 half_size, center;
  mat4 rotate;

  glm_quat_mat4(view->toplevel->quaternion, rotate);

  {
    struct wl_array rotate_array;
    wl_array_init(&rotate_array);
    glm_mat4_to_wl_array(rotate, &rotate_array);
    zgn_opengl_shader_program_set_uniform_float_matrix(
        view->shader, "rotate", 4, 4, false, 1, &rotate_array);
    wl_array_release(&rotate_array);
  }

  zgn_opengl_component_attach_shader_program(view->component, view->shader);

  if (view->parent == NULL) {
    glm_vec2_copy(view->toplevel->toplevel_view_half_size, half_size);
    glm_vec2_zero(center);
  } else {
    half_size[0] = view->surface_geometry.width *
                   view->parent->space_geometry.half_size[0] /
                   view->parent->surface_geometry.width;
    half_size[1] = view->surface_geometry.height *
                   view->parent->space_geometry.half_size[1] /
                   view->parent->surface_geometry.height;
    center[0] =
        ((float)view->surface_geometry.sx * 2 + view->surface_geometry.width -
            view->parent->surface_geometry.width) *
        view->parent->space_geometry.half_size[0] /
        view->parent->surface_geometry.width;
    center[1] =
        ((float)view->parent->surface_geometry.height -
            view->surface_geometry.sy * 2 - view->surface_geometry.height) *
        view->parent->space_geometry.half_size[1] /
        view->parent->surface_geometry.height;
  }

  float z = (float)view->z_index / 500;
  struct vertex A = {
      {-half_size[0] + center[0], -half_size[1] + center[1], z}, {0, 1}};
  struct vertex B = {
      {+half_size[0] + center[0], -half_size[1] + center[1], z}, {1, 1}};
  struct vertex C = {
      {+half_size[0] + center[0], +half_size[1] + center[1], z}, {1, 0}};
  struct vertex D = {
      {-half_size[0] + center[0], +half_size[1] + center[1], z}, {0, 0}};

  view->vertex_data->triangles[0].vertices[0] = A;
  view->vertex_data->triangles[0].vertices[1] = C;
  view->vertex_data->triangles[0].vertices[2] = D;
  view->vertex_data->triangles[1].vertices[0] = A;
  view->vertex_data->triangles[1].vertices[1] = C;
  view->vertex_data->triangles[1].vertices[2] = B;

  zgn_opengl_vertex_buffer_attach(
      view->vertex_buffer, view->vertex_buffer_buffer);
  zgn_opengl_component_attach_vertex_buffer(
      view->component, view->vertex_buffer);

  zsurf_signal_emit(&view->geometry_signal, NULL);

  glm_vec2_copy(half_size, view->space_geometry.half_size);
  glm_vec2_copy(center, view->space_geometry.center);
}

WL_EXPORT void*
zsurf_view_get_user_data(struct zsurf_view* view)
{
  return view->user_data;
}

static void
zsurf_view_callback_done(
    void* data, struct wl_callback* callback, uint32_t callback_time)
{
  struct zsurf_view_callback_data* callback_data = data;
  callback_data->func(callback_data->data, callback_time);

  wl_callback_destroy(callback);
  free(callback_data);
}

static const struct wl_callback_listener frame_callback_listener = {
    .done = zsurf_view_callback_done};

WL_EXPORT void
zsurf_view_add_frame_callback(struct zsurf_view* view,
    zsurf_view_frame_callback_func_t done_func, void* data)
{
  struct wl_callback* callback;
  struct zsurf_view_callback_data* callback_data;
  callback = zgn_virtual_object_frame(view->toplevel->virtual_object);

  callback_data = zalloc(sizeof *callback_data);
  callback_data->data = data;
  callback_data->func = done_func;

  wl_callback_add_listener(callback, &frame_callback_listener, callback_data);
}

WL_EXPORT int
zsurf_view_set_texture(struct zsurf_view* view, struct zsurf_color_bgra* data,
    uint32_t width, uint32_t height)
{
  if (zsurf_view_resize_texture(view, width, height) != 0) return -1;

  memcpy(view->texture_data, data,
      sizeof(struct zsurf_color_bgra) * width * height);

  zgn_opengl_texture_attach_2d(view->texture, view->texture_buffer);
  zgn_opengl_component_attach_texture(view->component, view->texture);

  if (view->state == ZSURF_VIEW_STATE_NO_TEXTURE)
    view->state = ZSURF_VIEW_STATE_FIRST_TEXTURE_ATTACHED;
  else if (view->state == ZSURF_VIEW_STATE_TEXTURE_COMMITTED)
    view->state = ZSURF_VIEW_STATE_NEW_TEXTURE_ATTACHED;

  return 0;
}

WL_EXPORT void
zsurf_view_commit(struct zsurf_view* view)
{
  zsurf_signal_emit(&view->commit_signal, NULL);
}

WL_EXPORT struct zsurf_view*
zsurf_view_create(struct zsurf_display* surface_display,
    struct zsurf_toplevel* toplevel, struct zsurf_view* parent, void* user_data)
{
  struct zsurf_view* view;
  int32_t fd, vertex_shader_fd, fragment_shader_fd;
  loff_t vertex_buffer_size, texture_size, shm_size;
  void* shm_data;
  struct wl_shm_pool* pool;
  struct zgn_opengl_component* component;
  struct zgn_opengl_vertex_buffer* vertex_buffer;
  struct wl_buffer *vertex_buffer_buffer, *texture_buffer;
  struct zgn_opengl_shader_program* shader;
  struct zgn_opengl_texture* texture;
  mat4 uniform_rotate = GLM_MAT4_IDENTITY_INIT;

  view = zalloc(sizeof *view);
  if (view == NULL) goto err;

  vertex_buffer_size = sizeof(*view->vertex_data);
  texture_size = sizeof(struct zsurf_color_bgra);  // 1 px at the beginning
  shm_size = vertex_buffer_size + texture_size;

  fd = create_shared_fd(shm_size);
  if (fd < 0) goto err_fd;

  vertex_shader_fd =
      create_shared_text_fd(vertex_shader, strlen(vertex_shader));
  if (vertex_shader_fd < 0) goto err_vertex_shader_fd;

  fragment_shader_fd =
      create_shared_text_fd(fragment_shader, strlen(fragment_shader));
  if (fragment_shader_fd < 0) goto err_fragment_shader_fd;

  shm_data = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (shm_data == NULL) goto err_mmap;

  pool = wl_shm_create_pool(surface_display->shm, fd, shm_size);

  component = zgn_opengl_create_opengl_component(
      surface_display->opengl, toplevel->virtual_object);

  vertex_buffer = zgn_opengl_create_vertex_buffer(surface_display->opengl);

  vertex_buffer_buffer = wl_shm_pool_create_buffer(
      pool, 0, vertex_buffer_size, 1, vertex_buffer_size, 0);

  shader = zgn_opengl_create_shader_program(surface_display->opengl);

  texture = zgn_opengl_create_texture(surface_display->opengl);

  texture_buffer = wl_shm_pool_create_buffer(pool, vertex_buffer_size, 1, 1,
      sizeof(struct zsurf_color_bgra), WL_SHM_FORMAT_ARGB8888);

  zgn_opengl_vertex_buffer_attach(vertex_buffer, vertex_buffer_buffer);
  zgn_opengl_component_attach_vertex_buffer(component, vertex_buffer);

  {
    struct wl_array rotate;
    wl_array_init(&rotate);
    glm_mat4_to_wl_array(uniform_rotate, &rotate);
    zgn_opengl_shader_program_set_uniform_float_matrix(
        shader, "rotate", 4, 4, false, 1, &rotate);
    wl_array_release(&rotate);
  }

  zgn_opengl_shader_program_set_vertex_shader(
      shader, vertex_shader_fd, strlen(vertex_shader));
  zgn_opengl_shader_program_set_fragment_shader(
      shader, fragment_shader_fd, strlen(fragment_shader));

  zgn_opengl_shader_program_link(shader);
  zgn_opengl_component_attach_shader_program(component, shader);

  zgn_opengl_texture_attach_2d(texture, texture_buffer);
  zgn_opengl_component_attach_texture(component, texture);

  zgn_opengl_component_add_vertex_attribute(component, 0, 3,
      ZGN_OPENGL_VERTEX_ATTRIBUTE_TYPE_FLOAT, false, sizeof(struct vertex),
      offsetof(struct vertex, p));
  zgn_opengl_component_add_vertex_attribute(component, 1, 2,
      ZGN_OPENGL_VERTEX_ATTRIBUTE_TYPE_FLOAT, false, sizeof(struct vertex),
      offsetof(struct vertex, uv));
  zgn_opengl_component_set_count(
      component, sizeof(struct view_rect) / sizeof(float));
  zgn_opengl_component_set_topology(component, ZGN_OPENGL_TOPOLOGY_TRIANGLES);

  view->surface_display = surface_display;
  view->user_data = user_data;
  view->toplevel = toplevel;
  view->parent = parent;
  view->z_index = parent ? parent->z_index + 1 : 0;
  view->state = ZSURF_VIEW_STATE_NO_TEXTURE;
  view->space_geometry.half_size[0] = 0;
  view->space_geometry.half_size[1] = 0;
  view->space_geometry.center[0] = 0;
  view->space_geometry.center[1] = 0;
  view->surface_geometry.width = 1;
  view->surface_geometry.height = 1;
  view->fd = fd;
  view->vertex_shader_fd = vertex_shader_fd;
  view->fragment_shader_fd = fragment_shader_fd;
  view->shm_data = shm_data;
  view->shm_size = shm_size;
  view->pool = pool;
  view->component = component;
  view->vertex_buffer = vertex_buffer;
  view->vertex_buffer_buffer = vertex_buffer_buffer;
  view->vertex_data = shm_data;
  view->shader = shader;
  view->texture = texture;
  view->texture_buffer = texture_buffer;
  view->texture_data =
      (struct zsurf_color_bgra*)((uint8_t*)shm_data + vertex_buffer_size);

  zsurf_signal_init(&view->commit_signal);
  zsurf_signal_init(&view->destroy_signal);
  zsurf_signal_init(&view->geometry_signal);

  view->parent_geometry_listener.notify = zsurf_view_parent_geometry_handler;
  if (parent)
    zsurf_signal_add(&parent->geometry_signal, &view->parent_geometry_listener);
  else
    wl_list_init(&view->parent_geometry_listener.link);

  return view;

err_mmap:
  close(fragment_shader_fd);

err_fragment_shader_fd:
  close(vertex_shader_fd);

err_vertex_shader_fd:
  close(fd);

err_fd:
  free(view);

err:
  return NULL;
}

WL_EXPORT void
zsurf_view_destroy(struct zsurf_view* view)
{
  wl_list_remove(&view->parent_geometry_listener.link);
  zsurf_signal_emit(&view->destroy_signal, NULL);
  zgn_opengl_texture_destroy(view->texture);
  zgn_opengl_shader_program_destroy(view->shader);
  zgn_opengl_vertex_buffer_destroy(view->vertex_buffer);
  wl_buffer_destroy(view->texture_buffer);
  wl_buffer_destroy(view->vertex_buffer_buffer);
  zgn_opengl_component_destroy(view->component);
  wl_shm_pool_destroy(view->pool);
  munmap(view->shm_data, view->shm_size);
  close(view->vertex_shader_fd);
  close(view->fragment_shader_fd);
  close(view->fd);
  free(view);
}

static const char* vertex_shader =
    "#version 410\n"
    "uniform mat4 zMVP;\n"
    "uniform mat4 rotate;\n"
    "layout(location = 0) in vec4 position;\n"
    "layout(location = 1) in vec2 v2UVcoordsIn;\n"
    "layout(location = 2) in vec3 v3NormalIn;\n"
    "out vec2 v2UVcoords;\n"
    "void main()\n"
    "{\n"
    "  v2UVcoords = v2UVcoordsIn;\n"
    "  gl_Position = zMVP * rotate * position;\n"
    "}\n";

static const char* fragment_shader =
    "#version 410 core\n"
    "uniform sampler2D userTexture;\n"
    "in vec2 v2UVcoords;\n"
    "out vec4 outputColor;\n"
    "void main()\n"
    "{\n"
    "  outputColor = texture(userTexture, v2UVcoords);\n"
    "}\n";
