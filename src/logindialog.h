#ifndef _LOGINDIALOG_H
#define _LOGINDIALOG_H

#include <gtk/gtk.h>

#include "ftpappwin.h"
#include "mainbox.h"

#include "libwaftp.h"

#define LOGIN_DIALOG_TYPE (login_dialog_get_type())

G_DECLARE_FINAL_TYPE(LoginDialog, login_dialog, LOGIN, DIALOG, GtkDialog)

LoginDialog *login_dialog_new(FtpAppWindow *win, struct UserPI *user_pi,
                              struct LoginInfo *login);

#endif
