scanner = dependency('wayland-scanner')
scanner_path = scanner.get_variable(pkgconfig : 'wayland_scanner')
z11_xml = files('/usr/local/share/z11/z11.xml')
z11_opengl_xml = files('/usr/local/share/z11/z11_opengl.xml')

z11_client_protocol_h = custom_target(
  'z11-client-protocol',
  output : 'z11-client-protocol.h',
  input : z11_xml,
  command : [scanner_path, 'client-header', '@INPUT@', '@OUTPUT@'],
)

z11_protocol_c = custom_target(
  'z11-protocol',
  output : 'z11-protocol.c',
  input : z11_xml,
  command : [scanner_path, 'public-code', '@INPUT@', '@OUTPUT@'],
)

z11_opengl_client_protocol_h = custom_target(
  'z11-opengl-client-protocol',
  output : 'z11-opengl-client-protocol.h',
  input : z11_opengl_xml,
  command : [scanner_path, 'client-header', '@INPUT@', '@OUTPUT@'],
)

z11_opengl_protocol_c = custom_target(
  'z11-opengl-protocol',
  output : 'z11-opengl-protocol.c',
  input : z11_opengl_xml,
  command : [scanner_path, 'public-code', '@INPUT@', '@OUTPUT@'],
)
