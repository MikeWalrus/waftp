#ifndef _FTPAPPWIN_H
#define _FTPAPPWIN_H

#include <gtk/gtk.h>

#include "ftpapp.h"

#define FTP_APP_WINDOW_TYPE (ftp_app_window_get_type())
G_DECLARE_FINAL_TYPE(FtpAppWindow, ftp_app_window, FTP, APP_WINDOW,
                     GtkApplicationWindow)

FtpAppWindow *ftp_app_window_new(FtpApp *app);

void ftp_app_window_open(FtpAppWindow *win, GFile *file);

typedef struct _MainBox MainBox;
void remove_tab(FtpAppWindow *win, MainBox *tab);

#endif
