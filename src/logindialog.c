#include <gtk/gtk.h>

#include "ftpapp.h"
#include "ftpappwin.h"
#include "logindialog.h"

struct _LoginDialog {
	GtkDialog parent;

	GtkWidget *font;
	GtkWidget *transition;
};

G_DEFINE_TYPE(LoginDialog, login_dialog, GTK_TYPE_DIALOG)

static void login_dialog_init(LoginDialog *d)
{
	gtk_widget_init_template(GTK_WIDGET(d));

	/* g_settings_bind (prefs->settings, "font", */
	/*                  prefs->font, "font", */
	/*                  G_SETTINGS_BIND_DEFAULT); */
	/* g_settings_bind (prefs->settings, "transition", */
	/*                  prefs->transition, "active-id", */
	/*                  G_SETTINGS_BIND_DEFAULT); */
}

static void login_dialog_dispose(GObject *object)
{
	/* LoginDialog *d = LOGIN_DIALOG(object); */

	/* g_clear_object(&prefs->settings); */

	G_OBJECT_CLASS(login_dialog_parent_class)->dispose(object);
}

static void login_dialog_class_init(LoginDialogClass *class)
{
	G_OBJECT_CLASS(class)->dispose = login_dialog_dispose;

	gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class),
	                                            "/walrus/ftp/login.ui");
	/* gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), */
	/*                                      FtpAppPrefs, font); */
	/* gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), */
	/*                                      FtpAppPrefs, transition); */
}

LoginDialog *login_dialog_new(FtpAppWindow *win)
{
	return g_object_new(LOGIN_DIALOG_TYPE, "transient-for", win,
	                    "use-header-bar", TRUE, NULL);
}
