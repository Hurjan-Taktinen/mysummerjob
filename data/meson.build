subdir('shaders')

# Copy config file to build directory
configure_file(
  input : 'config.ini',
  output : 'config.ini',
  copy : true)

model_source = join_paths(meson.source_root(), 'data/models')
model_dest = join_paths(meson.build_root(), 'data/models')
message('copying @0@ to @1@ ...'.format(model_source, model_dest))
r = run_command('cp', '-r', model_source, model_dest)

font_source = join_paths(meson.source_root(), 'data/fonts')
font_dest = join_paths(meson.build_root(), 'data/fonts')
message('copying @0@ to @1@ ...'.format(font_source, font_dest))
r = run_command('cp', '-r', font_source, font_dest)

