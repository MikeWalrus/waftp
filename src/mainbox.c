#include <gtk/gtk.h>

#include "ftpappwin.h"
#include "logindialog.h"
#include "mainbox.h"

#include <libwaftp.h>

struct _MainBox {
	GtkBox parent;
	FtpAppWindow *win;

	struct UserPI user_pi;
	struct LoginInfo login_info;
};

G_DEFINE_TYPE(MainBox, main_box, GTK_TYPE_BOX);

static void main_box_init(MainBox *b)
{
	gtk_widget_init_template(GTK_WIDGET(b));
}

static void main_box_class_init(MainBoxClass *class)
{
	gtk_widget_class_set_template_from_resource(
		GTK_WIDGET_CLASS(class), "/walrus/ftp/ui/mainbox.ui");
}

MainBox *main_box_new(FtpAppWindow *win)
{
	MainBox *self = g_object_new(MAIN_BOX_TYPE, NULL);
	self->win = win;
	LoginDialog *dialog =
		login_dialog_new(win, &self->user_pi, &self->login_info);
	gtk_window_present(GTK_WINDOW(dialog));
	return self;
}
