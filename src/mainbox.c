#include <gtk/gtk.h>

#include "err.h"
#include "ftpappwin.h"
#include "logindialog.h"
#include "mainbox.h"

#include <libwaftp.h>

struct _MainBox {
	GtkBox parent;
	GtkStackPage *page;
	FtpAppWindow *win;
	LoginDialog *login_dialog;

	GTask *listing_task;
	GAsyncQueue *path_queue;
	GAsyncQueue *list_queue;

	GtkTreeStore *tree;
	GtkTreeView *tree_view;
	GtkTreeViewColumn *icon_column;
	GtkTreeViewColumn *size_column;
	GtkTreeViewColumn *modify_column;

	struct UserPI *user_pi;
	struct LoginInfo login_info;
};

G_DEFINE_TYPE(MainBox, main_box, GTK_TYPE_BOX);

enum {
	NAME_COLUMN,
	IS_DIR_COLUMN,
	SIZE_COLUMN,
	PERM_COLUMN,
	MODIFY_COLUMN,
	N_COLUMNS
};

struct ListingData {
	struct UserPI *user_pi;
	GAsyncQueue *path_queue;
	GAsyncQueue *list_queue;
};

struct PathMsg {
	char *path;
	bool shutdown;
};

struct ListMsg {
	char *list;
	enum ListFormat format;
	struct ErrMsg *err_msg;
	bool err;
};

static struct PathMsg *path_msg(char *path)
{
	struct PathMsg *ret = g_new(struct PathMsg, 1);
	ret->shutdown = false;
	ret->path = path;
	return ret;
}

static struct PathMsg *shutdown_msg(void)
{
	struct PathMsg *ret = g_new(struct PathMsg, 1);
	ret->shutdown = true;
	return ret;
}

static struct ListMsg *ok_msg(char *list, enum ListFormat format)
{
	struct ListMsg *ret = g_new(struct ListMsg, 1);
	ret->err = false;
	ret->list = list;
	ret->format = format;
	return ret;
}

static struct ListMsg *err_msg(struct ErrMsg *err)
{
	struct ListMsg *ret = g_new(struct ListMsg, 1);
	ret->err = true;
	ret->err_msg = err;
	return ret;
}

static void listing_thread(GTask *task, gpointer source_object,
                           gpointer task_data, GCancellable *cancellable)
{
	struct ListingData *data = (struct ListingData *)task_data;
	GAsyncQueue *out = data->list_queue;
	GAsyncQueue *in = data->path_queue;
	struct UserPI *user_pi = data->user_pi;
	struct ErrMsg *err = g_new(struct ErrMsg, 1);
	for (;;) {
		struct PathMsg *in_msg =
			(struct PathMsg *)g_async_queue_pop(in); // blocks
		if (in_msg->shutdown)
			break;
		char *list = NULL;
		enum ListFormat format;
		struct ListMsg *out_msg;
		if (list_directory(user_pi, in_msg->path, &list, &format, err) <
		    0) {
			out_msg = err_msg(err);
			err = g_new(struct ErrMsg, 1);
		} else {
			out_msg = ok_msg(list, format);
		}
		g_async_queue_push(out, out_msg);
	}
	g_free(err);
}

static void start_listing_thread(MainBox *box)
{
	GTask *task = box->listing_task;
	struct ListingData *data = g_new(struct ListingData, 1);
	*data = (struct ListingData){ .user_pi = box->user_pi,
		                      .list_queue = box->list_queue,
		                      .path_queue = box->path_queue };
	g_task_set_task_data(task, data, (GDestroyNotify)g_free);
	g_task_run_in_thread(task, listing_thread);
}

struct Args_get_dir_listing {
	MainBox *box;
};

static bool get_dir_listing(struct Args_get_dir_listing *args)
{
	struct ListMsg *msg = g_async_queue_try_pop(args->box->list_queue);
	if (!msg)
		return true;
	if (msg->err) {
		report_ftp_error(GTK_WINDOW(args->box->win), msg->err_msg);
	} else {
		g_print("%s", msg->list);
		g_print("format: %d\n", msg->format);
	}

	g_free(args);
	// TODO: updata UI
	return false;
}

static void list_directory_async(MainBox *box, char *path)
{
	g_async_queue_push(box->path_queue, path_msg(path));
	struct Args_get_dir_listing *args =
		g_new(struct Args_get_dir_listing, 1);
	args->box = box;
	g_idle_add(G_SOURCE_FUNC(get_dir_listing), args);
}

static void main_box_init(MainBox *b)
{
	gtk_widget_init_template(GTK_WIDGET(b));
	b->listing_task = g_task_new(b, NULL, NULL, NULL);
	b->list_queue = g_async_queue_new();
	b->path_queue = g_async_queue_new();
	b->tree =
		gtk_tree_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_BOOLEAN,
	                           G_TYPE_UINT64, G_TYPE_STRING, G_TYPE_UINT64);
	gtk_tree_view_set_model(b->tree_view, GTK_TREE_MODEL(b->tree));
	g_object_unref(b->tree);
}

static void main_box_dispose(GObject *object)
{
	MainBox *self = MAIN_BOX(object);
	g_async_queue_push_front(self->path_queue, shutdown_msg());
	g_object_unref(self->list_queue);
	g_object_unref(self->path_queue);
	g_object_unref(self->listing_task);
	G_OBJECT_CLASS(main_box_parent_class)->dispose(object);
}

static void main_box_class_init(MainBoxClass *class)
{
	G_OBJECT_CLASS(class)->dispose = main_box_dispose;
	gtk_widget_class_set_template_from_resource(
		GTK_WIDGET_CLASS(class), "/walrus/ftp/ui/mainbox.ui");
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), MainBox,
	                                     tree_view);
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), MainBox,
	                                     icon_column);
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), MainBox,
	                                     size_column);
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), MainBox,
	                                     modify_column);
}

MainBox *main_box_new(FtpAppWindow *win)
{
	MainBox *self = g_object_new(MAIN_BOX_TYPE, NULL);
	self->win = win;
	self->user_pi = g_new(struct UserPI, 1);
	self->login_dialog =
		login_dialog_new(win, self, self->user_pi, &self->login_info);
	gtk_window_present(GTK_WINDOW(self->login_dialog));

	return self;
}

void main_box_set_page(MainBox *box, GtkStackPage *page)
{
	box->page = page;
}

void main_box_ftp_init(MainBox *box)
{
	gtk_stack_page_set_title(box->page, box->user_pi->ctrl.name);
	start_listing_thread(box);
	list_directory_async(box, "");
}
