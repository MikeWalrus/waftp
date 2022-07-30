#include <string.h>

#include <gtk/gtk.h>

#include "err.h"
#include "ftpapp.h"
#include "ftpappwin.h"
#include "logindialog.h"

struct ConnData {
	struct UserPI *user_pi;
	char *name;
	char *service;
	struct LoginInfo *login;
};

struct _LoginDialog {
	GtkDialog parent;
	GtkButton *button_ok;
	GtkEntry *host;
	GtkEntry *user;
	GtkPasswordEntry *password;
	GtkPasswordEntry *account;
	GtkEntry *port;
	GtkSpinner *spinner;

	GCancellable *cancellable;

	FtpAppWindow *win;
	MainBox *box;

	struct ConnData conn_data;
};

G_DEFINE_TYPE(LoginDialog, login_dialog, GTK_TYPE_DIALOG)

static void conn_data_free_strings(struct ConnData *data)
{
	free((void *)data->login->account_info);
	free((void *)data->login->password);
	free((void *)data->login->username);
	free(data->name);
	free(data->service);
}

static void conn_data_free(struct ConnData *data)
{
	g_free(data);
}

static void login_thread(GTask *task, gpointer source_object,
                         gpointer task_data, GCancellable *cancellable)
{
	struct ConnData *data = task_data;
	GError *error = ftp_error_fail_new();
	struct ErrMsg *err = ftp_error_get_err(error);

	struct UserPI *result = user_pi_init(data->name, data->service,
	                                     data->login, data->user_pi, err);

	if (!result) {
		g_task_return_error(task, error);
		return;
	}

	if (g_task_set_return_on_cancel(task, FALSE)) {
		g_task_return_int(task, 0);
		return;
	}
	// We've been cancelled, so we are the last one to hold these variables.
	conn_data_free_strings(data);
	// TODO: Clean up user_pi
	g_free(data->user_pi);
	g_print("Cancelled login thread finished.");
}

static void user_pi_init_async(LoginDialog *dialog, GCancellable *cancellable,
                               GAsyncReadyCallback callback, gpointer user_data)
{
	GTask *task = g_task_new(dialog, cancellable, callback, user_data);
	struct ConnData *data = g_new(struct ConnData, 1);
	*data = dialog->conn_data;
	g_task_set_task_data(task, data, (GDestroyNotify)conn_data_free);
	g_task_set_return_on_cancel(task, TRUE);
	g_task_run_in_thread(task, login_thread);
}

void conn_data_copy_strings_from_dialog(struct ConnData *data,
                                        LoginDialog *dialog)
{
	const char *host = gtk_editable_get_text(GTK_EDITABLE(dialog->host));
	const char *user = gtk_editable_get_text(GTK_EDITABLE(dialog->user));
	const char *password =
		gtk_editable_get_text(GTK_EDITABLE(dialog->password));
	const char *account =
		gtk_editable_get_text(GTK_EDITABLE(dialog->account));
	const char *port = gtk_editable_get_text(GTK_EDITABLE(dialog->port));

	data->name = strdup(host);
	data->service = *port ? strdup(port) : strdup("21");

	struct LoginInfo *login = dialog->conn_data.login;
	*login = (struct LoginInfo){ .username =
		                             strdup(*user ? user : "anonymous"),
		                     .password = strdup(password),
		                     .account_info = strdup(account) };
}

void user_pi_init_ready(GObject *source_object, GAsyncResult *res,
                        gpointer user_data)
{
	LoginDialog *dialog = LOGIN_DIALOG(source_object);

	GTask *task = G_TASK(res);
	struct ConnData *data = user_data;

	GError *e = NULL;
	if (g_task_propagate_int(task, &e) < 0) {
		if (g_error_matches(e, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
			g_print("Login cancelled");
			remove_tab(dialog->win, dialog->box);
			gtk_window_destroy(GTK_WINDOW(dialog));
			return;
		}
		if (g_error_matches(e, FTP_ERROR, FTP_ERROR_FAIL)) {
			struct ErrMsg *err = ftp_error_get_err(e);
			report_ftp_error(GTK_WINDOW(dialog), err);
			conn_data_free_strings(data);
			gtk_widget_set_visible(GTK_WIDGET(dialog->spinner),
			                       false);
			return;
		}
		g_assert(false && "Unexpected GError");
	}
	main_box_ftp_init(dialog->box);
	gtk_window_destroy(GTK_WINDOW(dialog));
};

static bool is_busy(LoginDialog *dialog)
{
	return gtk_widget_get_visible(GTK_WIDGET(dialog->spinner));
}

static void set_busy(LoginDialog *dialog, bool busy)
{
	gtk_widget_set_visible(GTK_WIDGET(dialog->spinner), busy);
}

static void login_do(LoginDialog *dialog)
{
	if (is_busy(dialog))
		return;
	set_busy(dialog, true);
	gtk_spinner_start(dialog->spinner);

	conn_data_copy_strings_from_dialog(&dialog->conn_data, dialog);

	g_cancellable_reset(dialog->cancellable);
	user_pi_init_async(dialog, dialog->cancellable, user_pi_init_ready,
	                   &dialog->conn_data);
}

static void login_cancel(LoginDialog *dialog)
{
	if (is_busy(dialog)) {
		g_cancellable_cancel(dialog->cancellable);
		return;
	}
	remove_tab(dialog->win, dialog->box);
	gtk_window_destroy(GTK_WINDOW(dialog));
}

void login_response(LoginDialog *dialog, gint response_id, LoginDialog *_)
{
	g_print("response_id: %d\n", response_id);
	switch (response_id) {
	case GTK_RESPONSE_OK:
		login_do(dialog);
		break;
	case GTK_RESPONSE_CANCEL:
	case GTK_RESPONSE_CLOSE:
		login_cancel(dialog);
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
	LoginDialog *d = LOGIN_DIALOG(object);
	g_object_unref(d->cancellable);

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

LoginDialog *login_dialog_new(FtpAppWindow *win, MainBox *box,
                              struct UserPI *user_pi, struct LoginInfo *login)
{
	LoginDialog *self = g_object_new(LOGIN_DIALOG_TYPE, "transient-for",
	                                 win, "use-header-bar", TRUE, NULL);
	self->win = win;
	self->box = box;
	self->conn_data.user_pi = user_pi;
	self->conn_data.login = login;
	self->cancellable = g_cancellable_new();
	return self;
}
