gtkdep = dependency('gtk4')
libwaftpdep = dependency('libwaftp-1')

sources = [
'err.c',
'ftpapp.c',
'ftpappwin.c',
'logindialog.c',
'main.c',
'mainbox.c',
]
executable('waftp',
           sources,
           resources,
           dependencies: [gtkdep, libwaftpdep]
           )
