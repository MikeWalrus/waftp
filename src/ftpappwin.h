#ifndef _FTPAPPWIN_H
#define _FTPAPPWIN_H

#include "ftpapp.h"
#include <gtk/gtk.h>

#define FTP_APP_WINDOW_TYPE (ftp_app_window_get_type())
G_DECLARE_FINAL_TYPE(FtpAppWindow, ftp_app_window, FTP, APP_WINDOW,
                     GtkApplicationWindow)

FtpAppWindow *ftp_app_window_new(FtpApp *app);
void ftp_app_window_open(FtpAppWindow *win, GFile *file);

#endif
