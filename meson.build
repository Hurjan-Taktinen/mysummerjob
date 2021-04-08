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

cmake = import('cmake')
cmake_opt_glfw = cmake.subproject_options()

## GLFW options
cmake_opt_glfw.add_cmake_defines({'GLFW_BUILD_DOCS': false})
cmake_opt_glfw.add_cmake_defines({'GLFW_BUILD_TESTS': false})
cmake_opt_glfw.add_cmake_defines({'GLFW_BUILD_EXAMPLES': false})
cmake_opt_glfw.add_cmake_defines({'GLFW_USE_WAYLAND': false})
cmake_opt_glfw.add_cmake_defines({'GLFW_VULKAN_STATIC': true})
cmake_opt_glfw.add_cmake_defines({'CMAKE_BUILD_TYPE': 'Release'})
cmake_opt_glfw.set_override_option('warning_level', '0')
cmake_opt_glfw.set_override_option('werror', 'false')

## Subprojects ##
catch2 = subproject('catch2')
glfw_subpro = cmake.subproject('glfw', options : cmake_opt_glfw)
glm = subproject('glm')
spdlog = subproject('spdlog')
vulkan_headers = subproject('vulkan-headers')
vulkan_validationlayers = subproject('vulkan-validationlayers')

## Build dependencies ##
build_deps = []
build_deps += catch2.get_variable('catch2_dep')
build_deps += glfw_subpro.dependency('glfw')
build_deps += glm.get_variable('glm_dep')
build_deps += spdlog.get_variable('spdlog_dep')
build_deps += vulkan_headers.get_variable('vulkan_headers_dep')
build_deps += vulkan_validationlayers.get_variable('vulkan_validationlayers_dep')

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

