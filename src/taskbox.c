#include <gtk/gtk.h>

#include "err.h"
#include "taskbox.h"

struct _TaskBox {
	GtkBox parent;

	FtpAppWindow *win;
};

G_DEFINE_TYPE(TaskBox, task_box, GTK_TYPE_BOX);

static void task_box_init(TaskBox *pop)
{
	gtk_widget_init_template(GTK_WIDGET(pop));
}

static void task_box_dispose(GObject *object)
{
	G_OBJECT_CLASS(task_box_parent_class)->dispose(object);
}

static void task_box_class_init(TaskBoxClass *class)
{
	G_OBJECT_CLASS(class)->dispose = task_box_dispose;

	gtk_widget_class_set_template_from_resource(
		GTK_WIDGET_CLASS(class), "/walrus/ftp/ui/taskbox.ui");
}

TaskBox *task_box_new(FtpAppWindow *win)
{
	TaskBox *self = g_object_new(TASK_BOX_TYPE, NULL);
	self->win = win;
	return self;
}
