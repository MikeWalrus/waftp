#include <gtk/gtk.h>

#include "ftpapp.h"
#include "ftpappwin.h"

struct _FtpApp
{
  GtkApplication parent;
};

G_DEFINE_TYPE(FtpApp, ftp_app, GTK_TYPE_APPLICATION);

static void
ftp_app_init (FtpApp *app)
{
}

static void
ftp_app_activate (GApplication *app)
{
  FtpAppWindow *win = ftp_app_window_new (FTP_APP (app));
  gtk_window_present (GTK_WINDOW (win));
}

static void
ftp_app_class_init (FtpAppClass *class)
{
  G_APPLICATION_CLASS (class)->activate = ftp_app_activate;
}

FtpApp *
ftp_app_new (void)
{
  return g_object_new (FTP_APP_TYPE,
                       "application-id", "walrus.ftp",
                       "flags", G_APPLICATION_HANDLES_OPEN,
                       NULL);
}
