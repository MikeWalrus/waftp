#include <gtk/gtk.h>

#include "ftpapp.h"
#include "ftpappwin.h"

struct _FtpAppWindow {
	GtkApplicationWindow parent;
};

G_DEFINE_TYPE(FtpAppWindow, ftp_app_window, GTK_TYPE_APPLICATION_WINDOW);

static void ftp_app_window_init(FtpAppWindow *win)
{
	gtk_widget_init_template(GTK_WIDGET(win));
}

static void ftp_app_window_class_init(FtpAppWindowClass *class)
{
	gtk_widget_class_set_template_from_resource(
		GTK_WIDGET_CLASS(class), "/walrus/ftp/ui/window.ui");
}

FtpAppWindow *ftp_app_window_new(FtpApp *app)
{
	return g_object_new(FTP_APP_WINDOW_TYPE, "application", app, NULL);
}

void ftp_app_window_open(FtpAppWindow *win, GFile *file)
{
}
