#ifndef _TASKBOX_H
#define _TASKBOX_H

#include <gtk/gtk.h>

#include "ftpappwin.h"
#include "libwaftp.h"

#define TASK_BOX_TYPE (task_box_get_type())

G_DECLARE_FINAL_TYPE(TaskBox, task_box, TASK, BOX, GtkBox)

TaskBox *task_box_new(FtpAppWindow *win, const struct UserPI *user_pi,
                      const struct LoginInfo *login, char *path, char *name,
                      ssize_t size);

bool update_task_boxes(FtpAppWindow *win);

#endif
