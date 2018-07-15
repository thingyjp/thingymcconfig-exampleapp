project('thingymcconfig-app', 'c')

src = ['thingymcconfig-app.c']

incs = include_directories(['../thingymcconfig/include/'])
  
deps = [dependency('glib-2.0'),
        dependency('gio-unix-2.0'),
        dependency('liboping')]

executable('thingymcconfig-app', src, include_directories : incs, dependencies : deps)