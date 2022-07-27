#ifndef _LOGINDIALOG_H
#define _LOGINDIALOG_H

#include <gtk/gtk.h>

#define LOGIN_DIALOG_TYPE (login_dialog_get_type())

G_DECLARE_FINAL_TYPE(LoginDialog, login_dialog, LOGIN, DIALOG, GtkDialog)

#endif
