deps_zsurface = [
  wayland_client_dep,
  cglm_dep,
]

srcs_zsurface = files([
  'util.c',
  'toplevel.c',
  'view.c',
  'view_ray_intersection.c',
  'zsurface.c',
]) + [
  z11_protocol_c,
  z11_client_protocol_h,
  z11_opengl_protocol_c,
  z11_opengl_client_protocol_h,
  z11_input_protocol_c,
  z11_input_client_protocol_h,
]

lib_zsurface = library(
  'zsurface',
  srcs_zsurface,
  install : true,
  include_directories : public_inc,
  dependencies : deps_zsurface,
  version : meson.project_version(),
)

pkg = import('pkgconfig')
pkg.generate(
  name : 'zsurface',
  libraries : lib_zsurface,
  version : meson.project_version(),
  url : 'https://github.com/gray-armor/zsurface',
  description : 'Surface like z11 client library',
)

deps_zsurface_for_users = []

zsurface_dep = declare_dependency(
  link_with : lib_zsurface,
  include_directories : public_inc,
  dependencies : deps_zsurface_for_users,
)
