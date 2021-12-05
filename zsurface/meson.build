deps_zsurface = [
  wayland_client_dep,
  cglm_dep,
]

srcs_zsurface = files([
  'display.c',
  'toplevel.c',
  'view.c',
]) + [
  zigen_protocol_c,
  zigen_client_protocol_h,
  zigen_shell_protocol_c,
  zigen_shell_client_protocol_h,
  zigen_opengl_protocol_c,
  zigen_opengl_client_protocol_h,
]

lib_zsurface = library(
  'zsurface',
  srcs_zsurface,
  install : true,
  include_directories : public_inc,
  dependencies : deps_zsurface,
  version : meson.project_version(),
)

deps_zsurface_for_users = [
  cglm_dep,
]

pkg = import('pkgconfig')
pkg.generate(
  name : 'zsurface',
  libraries : lib_zsurface,
  version : meson.project_version(),
  url : 'https://github.com/gray-armor/zsurface',
  description : 'Surface like zigen client library',
)

zsurface_dep = declare_dependency(
  link_with : lib_zsurface,
  dependencies : deps_zsurface_for_users,
)
