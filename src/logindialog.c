#include <string.h>

#include <gtk/gtk.h>

#include "ftpapp.h"
#include "ftpappwin.h"
#include "logindialog.h"

struct _LoginDialog {
	GtkDialog parent;
	GtkButton *button_ok;
	GtkEntry *host;
	GtkEntry *user;
	GtkPasswordEntry *password;
	GtkPasswordEntry *account;
	GtkEntry *port;
	GtkSpinner *spinner;

	struct UserPI *user_pi;
	struct LoginInfo *login;
};

G_DEFINE_TYPE(LoginDialog, login_dialog, GTK_TYPE_DIALOG)

void fill_in_login_info(LoginDialog *dialog, char **name, char **service)
{
	const char *host = gtk_editable_get_text(GTK_EDITABLE(dialog->host));
	const char *user = gtk_editable_get_text(GTK_EDITABLE(dialog->user));
	const char *password =
		gtk_editable_get_text(GTK_EDITABLE(dialog->password));
	const char *account =
		gtk_editable_get_text(GTK_EDITABLE(dialog->account));
	const char *port = gtk_editable_get_text(GTK_EDITABLE(dialog->port));

	*name = strdup(host);
	*service = *port ? strdup(port) : strdup("21");

	struct LoginInfo *login = dialog->login;
	*login = (struct LoginInfo){ .username =
		                             strdup(*user ? user : "anonymous"),
		                     .password = strdup(password),
		                     .account_info = strdup(account) };
}

static void login_do(LoginDialog *dialog)
{
	bool busy = gtk_widget_get_visible(GTK_WIDGET(dialog->spinner));
	if (busy)
		return;
	gtk_widget_set_visible(GTK_WIDGET(dialog->spinner), true);

	char *name;
	char *service;

	fill_in_login_info(dialog, &name, &service);
	struct UserPI *user_pi = dialog->user_pi;
	struct ErrMsg err;
	if (user_pi_init(name, service, dialog->login, user_pi, &err) == NULL) {
		GtkWidget *msg = gtk_message_dialog_new(
			GTK_WINDOW(dialog),
			GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL |
				GTK_DIALOG_USE_HEADER_BAR,
			GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "%s: %s",
			err.where, err.msg);
		g_signal_connect(msg, "response",
		                 G_CALLBACK(gtk_window_destroy), NULL);
		gtk_window_present(GTK_WINDOW(msg));
	}
	gtk_widget_set_visible(GTK_WIDGET(dialog->spinner), false);
}

void login_response(LoginDialog *dialog, gint response_id, LoginDialog *_)
{
	g_print("response_id: %d\n", response_id);
	switch (response_id) {
	case GTK_RESPONSE_OK:
		login_do(dialog);
		break;
	default:
		break;
	}
}

static void login_dialog_init(LoginDialog *dialog)
{
	gtk_widget_init_template(GTK_WIDGET(dialog));
	g_object_bind_property(dialog->spinner, "visible", dialog->button_ok,
	                       "sensitive", G_BINDING_INVERT_BOOLEAN);
}

static void login_dialog_dispose(GObject *object)
{
	/* LoginDialog *d = LOGIN_DIALOG(object); */

	G_OBJECT_CLASS(login_dialog_parent_class)->dispose(object);
}

static void login_dialog_class_init(LoginDialogClass *class)
{
	G_OBJECT_CLASS(class)->dispose = login_dialog_dispose;

	gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class),
	                                            "/walrus/ftp/ui/login.ui");
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
	                                     LoginDialog, button_ok);
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
	                                     LoginDialog, host);
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
	                                     LoginDialog, user);
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
	                                     LoginDialog, password);
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
	                                     LoginDialog, account);
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
	                                     LoginDialog, port);
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
	                                     LoginDialog, spinner);

	gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(class),
	                                        login_response);
}

LoginDialog *login_dialog_new(FtpAppWindow *win, struct UserPI *user_pi,
                              struct LoginInfo *login)
{
	LoginDialog *self = g_object_new(LOGIN_DIALOG_TYPE, "transient-for",
	                                 win, "use-header-bar", TRUE, NULL);
	self->user_pi = user_pi;
	self->login = login;
	return self;
}
