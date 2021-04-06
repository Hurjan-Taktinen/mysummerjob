project('mysummerjob',
  'cpp',
  default_options : [
    'cpp_std=c++20',
    'buildtype=debug',
    'warning_level=3',
    'werror=false',
    'default_library=static'],
  version : '0.0.1',
  license: 'MIT')

build_deps = []

inc = []
inc += include_directories('include')
inc += include_directories('external')

sources = []
cpp_compile_args = []

subdir('data')
subdir('src')

executable('mysummerjob', sources,
  include_directories : inc,
  dependencies : build_deps,
  cpp_args : cpp_compile_args)

