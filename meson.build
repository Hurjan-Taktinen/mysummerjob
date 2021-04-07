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

## Subprojects ##
spdlog = subproject('spdlog')

## Build dependencies ##
build_deps = []
build_deps += spdlog.get_variable('spdlog_dep')

## Include directories ##
inc = []
inc += include_directories('include')

cpp_compile_args = []

sources = []
subdir('data')
subdir('src')

executable('mysummerjob', sources,
  include_directories : inc,
  dependencies : build_deps,
  cpp_args : cpp_compile_args)

