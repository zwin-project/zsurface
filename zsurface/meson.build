deps_zsurface = [
  wayland_client_dep,
  cglm_dep,
]

srcs_zsurface = files([
  'zsurface.c',
  'view.c',
  'util.c',
  'toplevel.c',
]) + [
  z11_protocol_c,
  z11_client_protocol_h,
  z11_opengl_protocol_c,
  z11_opengl_client_protocol_h,
  z11_input_protocol_c,
  z11_input_client_protocol_h,
]

lib_zsurface = static_library(
  'zsurface',
  srcs_zsurface,
  include_directories : public_inc,
  dependencies : deps_zsurface,
)

deps_zsurface_for_users = []

zsurface_public_dep = declare_dependency(
  link_with : lib_zsurface,
  include_directories : public_inc,
  dependencies : deps_zsurface_for_users,
)
