glslang = find_program('glslangValidator')

shaders = []

if glslang.found()
  srcs = [
    'simple.frag',
    'simple.vert',
    'ui_shader.frag',
    'ui_shader.vert',
    'obj.frag',
    'obj.vert']

  foreach s : srcs

    message(s)

    shaders += custom_target('shader_@0@'.format(s),
      input : s,
      output : '@PLAINNAME@.spv',
      command : [glslang, '-V', '@INPUT@', '-o', '@OUTPUT@'],
      install : true,
      install_dir : 'data/spirv'
      )

  endforeach
endif
