inc += include_directories('include')
subdir('include')

subdir('src')

argtable3_lib = static_library('argtable3',
  argtable3_sources,
  include_directories: inc,
  extra_files: argtable3_headers
)
