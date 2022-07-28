gtkdep = dependency('gtk4')
libwaftpdep = dependency('libwaftp-1')

sources =['main.c',
          'ftpapp.c',
          'ftpappwin.c',
          'mainbox.c',
          'logindialog.c'
          ]
executable('waftp',
           sources,
           resources,
           dependencies: [gtkdep, libwaftpdep]
           )
