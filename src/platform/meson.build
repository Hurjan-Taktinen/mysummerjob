platform = host_machine.system()

if(platform == 'linux')
  env = run_command('env')

  ## Search if WAYLAND_DISPLAY env variable is set
  using_wayland = env.stdout().strip().to_lower().contains('wayland_display')

  if(using_wayland == true)
    subdir(linux_wayland)
  else
    subdir(linux_x11)
  endif
else if(platform == 'windows')
  subdir('windows')
else
  message('UNSUPPORTED PLATFORM')
endif
