#ifndef _FTPAPP_H
#define _FTPAPP_H

#include <gtk/gtk.h>

#define FTP_APP_TYPE (ftp_app_get_type())
G_DECLARE_FINAL_TYPE(FtpApp, ftp_app, FTP, APP, GtkApplication)

FtpApp *ftp_app_new(void);

#endif
