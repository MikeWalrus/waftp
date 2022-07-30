#include <string.h>

#include "err.h"

typedef struct {
	struct ErrMsg err;
} FtpErrorPrivate;

static void ftp_error_private_init(FtpErrorPrivate *priv)
{
}

static void ftp_error_private_copy(const FtpErrorPrivate *src_priv,
                                   FtpErrorPrivate *dest_priv)
{
	strcpy(dest_priv->err.msg, src_priv->err.msg);
	strcpy(dest_priv->err.where, src_priv->err.where);
}

static void ftp_error_private_clear(FtpErrorPrivate *priv)
{
}

G_DEFINE_EXTENDED_ERROR(FtpError, ftp_error)

struct ErrMsg *ftp_error_get_err(GError *error)
{
	FtpErrorPrivate *priv = ftp_error_get_private(error);
	g_return_val_if_fail(priv != NULL, NULL);
	return &priv->err;
}

void report_ftp_error(GtkWindow *win, struct ErrMsg *err)
{
	GtkWidget *msg = gtk_message_dialog_new(
		win,
		GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL |
			GTK_DIALOG_USE_HEADER_BAR,
		GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "%s", err->msg);
	g_signal_connect(msg, "response", G_CALLBACK(gtk_window_destroy), NULL);
	gtk_window_set_title(GTK_WINDOW(msg), err->where);
	gtk_window_present(GTK_WINDOW(msg));
}
