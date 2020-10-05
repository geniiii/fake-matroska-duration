project_headers += files('endian.h', 'mem.h', 'fs.h')

conf_data = configuration_data()
conf_data.set('version', meson.project_version())
configure_file(input: 'config.h.in', output: 'config.h', configuration: conf_data)
