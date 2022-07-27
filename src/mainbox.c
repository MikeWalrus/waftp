#include <gtk/gtk.h>

#include "mainbox.h"

struct _MainBox {
	GtkBox parent;
};

G_DEFINE_TYPE(MainBox, main_box, GTK_TYPE_BOX);

static void main_box_init(MainBox *b)
{
	gtk_widget_init_template(GTK_WIDGET(b));
}

static void main_box_class_init(MainBoxClass *class)
{
	gtk_widget_class_set_template_from_resource(
		GTK_WIDGET_CLASS(class), "/walrus/ftp/ui/main_box.ui");
}

MainBox *main_box_new(FtpAppWindow *win)
{
	MainBox *ret = g_object_new(MAIN_BOX_TYPE, NULL);
	return ret;
}
