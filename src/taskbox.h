#ifndef _TASKBOX_H
#define _TASKBOX_H

#include <gtk/gtk.h>

#include "ftpappwin.h"

#define TASK_BOX_TYPE (task_box_get_type())

G_DECLARE_FINAL_TYPE(TaskBox, task_box, TASK, BOX, GtkBox)

TaskBox *task_box_new(FtpAppWindow *win);

#endif
