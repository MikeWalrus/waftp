#ifndef _ERR_H
#define _ERR_H

#include "libwaftp.h"
#include <gtk/gtk.h>

typedef enum {
	FTP_ERROR_FAIL,
} FtpError;
#define FTP_ERROR (ftp_error_quark())
GQuark ftp_error_quark(void);

struct ErrMsg *ftp_error_get_err(GError *error);

static inline GError *ftp_error_fail_new()
{
	return g_error_new_literal(FTP_ERROR, FTP_ERROR_FAIL, "");
}

#endif
