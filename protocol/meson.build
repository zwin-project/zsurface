scanner_path = wayland_scanner_dep.get_variable(pkgconfig : 'wayland_scanner')
zigen_xml = files('/usr/local/share/zigen/zigen.xml')
zigen_shell_xml = files('/usr/local/share/zigen/zigen-shell.xml')
zigen_opengl_xml = files('/usr/local/share/zigen/zigen-opengl.xml')

zigen_client_protocol_h = custom_target(
  'zigen-client-protocol',
  output : 'zigen-client-protocol.h',
  input : zigen_xml,
  command : [scanner_path, 'client-header', '@INPUT@', '@OUTPUT@'],
)

zigen_protocol_c = custom_target(
  'zigen-protocol',
  output : 'zigen-protocol.c',
  input : zigen_xml,
  command : [scanner_path, 'public-code', '@INPUT@', '@OUTPUT@'],
)

zigen_shell_client_protocol_h = custom_target(
  'zigen-shell-client-protocol',
  output : 'zigen-shell-client-protocol.h',
  input : zigen_shell_xml,
  command : [scanner_path, 'client-header', '@INPUT@', '@OUTPUT@'],
)

zigen_shell_protocol_c = custom_target(
  'zigen-shell-protocol',
  output : 'zigen-shell-protocol.c',
  input : zigen_shell_xml,
  command : [scanner_path, 'public-code', '@INPUT@', '@OUTPUT@'],
)

zigen_opengl_client_protocol_h = custom_target(
  'zigen-opengl-client-protocol',
  output : 'zigen-opengl-client-protocol.h',
  input : zigen_opengl_xml,
  command : [scanner_path, 'client-header', '@INPUT@', '@OUTPUT@'],
)

zigen_opengl_protocol_c = custom_target(
  'zigen-opengl-protocol',
  output : 'zigen-opengl-protocol.c',
  input : zigen_opengl_xml,
  command : [scanner_path, 'public-code', '@INPUT@', '@OUTPUT@'],
)
