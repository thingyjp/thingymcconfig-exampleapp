project('thingymcconfig-app', 'c')

src = ['thingymcconfig-app.c']
  
deps = [dependency('libthingymcconfig_client_glib'),
        dependency('glib-2.0'),
        dependency('gio-unix-2.0'),
        dependency('liboping')]

executable('thingymcconfig-app', src,
           dependencies : deps,
           install : true)