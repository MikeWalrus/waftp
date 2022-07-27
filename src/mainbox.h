#ifndef _MAINBOX_H
#define _MAINBOX_H

#include <gtk/gtk.h>

#include "ftpappwin.h"

#define MAIN_BOX_TYPE (main_box_get_type())
G_DECLARE_FINAL_TYPE(MainBox, main_box, MAIN, BOX, GtkBox)

MainBox *main_box_new(FtpAppWindow *win);

#endif
