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

// This defines the ftp_error_get_private and ftp_error_quark functions.
G_DEFINE_EXTENDED_ERROR(FtpError, ftp_error)

struct ErrMsg *ftp_error_get_err(GError *error)
{
	FtpErrorPrivate *priv = ftp_error_get_private(error);
	g_return_val_if_fail(priv != NULL, NULL);
	return &priv->err;
}
