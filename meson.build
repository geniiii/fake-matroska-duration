project('fake-matroska-duration', 'c',
	version: '1.0.0',
	license: ['MIT'],
	meson_version: '>= 0.50.0',
	default_options: ['warning_level=3', 'buildtype=debugoptimized', 'c_std=c99']
)

inc = [include_directories('include')]

project_sources = []
project_headers = []

subdir('external')
subdir('include')
subdir('src')

cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : false)

fmd_bin = executable('fmd',
  main_source,
  include_directories: inc,
  link_with: [argtable3_lib],
  dependencies: [m_dep],
  extra_files: project_headers)
