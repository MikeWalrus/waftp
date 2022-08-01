#include <gtk/gtk.h>

#include "err.h"
#include "ftpapp.h"
#include "ftpappwin.h"
#include "mainbox.h"
#include "taskbox.h"

struct _FtpAppWindow {
	GtkApplicationWindow parent;

	GtkStack *stack;
	GtkPopover *popover;
	GtkListBox *tasks;

	GAsyncQueue *task_update_queue;
	size_t num_active_tasks;
};

G_DEFINE_TYPE(FtpAppWindow, ftp_app_window, GTK_TYPE_APPLICATION_WINDOW);

void add_new_tab(GtkButton *button, FtpAppWindow *win)
{
	MainBox *box = main_box_new(win);
	GtkStackPage *page = gtk_stack_add_titled(
		GTK_STACK(win->stack), GTK_WIDGET(box), NULL, "New Tab");
	main_box_set_page(box, page);
	gtk_stack_set_visible_child(win->stack, GTK_WIDGET(box));
}

void add_task_box(FtpAppWindow *win, TaskBox *box)
{
	gtk_list_box_insert(win->tasks, GTK_WIDGET(box), 0);
	if (win->num_active_tasks == 0) {
		g_timeout_add(500, G_SOURCE_FUNC(update_task_boxes), win);
	}
	win->num_active_tasks++;
}

GAsyncQueue *get_queue(FtpAppWindow *win)
{
	return win->task_update_queue;
}

size_t *get_active_tasks(FtpAppWindow *win)
{
	return &win->num_active_tasks;
}

void show_popover(GtkButton *button, FtpAppWindow *win)
{
	bool visible = gtk_widget_get_visible(GTK_WIDGET(win->popover));
	if (!visible) {
		gtk_popover_popup(win->popover);
		return;
	}
	gtk_popover_popdown(win->popover);
}

void remove_tab(FtpAppWindow *win, MainBox *tab)
{
	gtk_stack_remove(win->stack, GTK_WIDGET(tab));
}

static void ftp_app_window_init(FtpAppWindow *win)
{
	gtk_widget_init_template(GTK_WIDGET(win));
	win->task_update_queue = g_async_queue_new();
}

static void ftp_app_window_dispose(GObject *object)
{
	FtpAppWindow *win = FTP_APP_WINDOW(object);
	g_async_queue_unref(win->task_update_queue);
	G_OBJECT_CLASS(ftp_app_window_parent_class)->dispose(object);
}

static void ftp_app_window_class_init(FtpAppWindowClass *class)
{
	G_OBJECT_CLASS(class)->dispose = ftp_app_window_dispose;

	gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class),
	                                            "/walrus/ftp/ui/window.ui");
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
	                                     FtpAppWindow, stack);
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
	                                     FtpAppWindow, popover);
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
	                                     FtpAppWindow, tasks);
	gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(class),
	                                        add_new_tab);
	gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(class),
	                                        show_popover);
}

FtpAppWindow *ftp_app_window_new(FtpApp *app)
{
	return g_object_new(FTP_APP_WINDOW_TYPE, "application", app, NULL);
}

void ftp_app_window_open(FtpAppWindow *win, GFile *file)
{
}
