deps_zsurface = [
]

srcs_zsurface = files([
  'zsurface.c'
])

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
