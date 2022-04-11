project(
  'zsurface',
  'c',
  license : 'MIT',
  default_options : ['warning_level=3', 'werror=true', 'c_std=gnu11', 'optimization=2'],
  meson_version : '>= 0.57.0',
  version : '0.0.1',
)

public_inc = include_directories('include')

wayland_client_dep = dependency('wayland-client')
wayland_scanner_dep = dependency('wayland-scanner')
cglm_dep = dependency('cglm')

subdir('include')
subdir('protocol')
subdir('zsurface')
subdir('example')
