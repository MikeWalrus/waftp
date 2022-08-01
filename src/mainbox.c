#include <gtk/gtk.h>

#include "err.h"
#include "ftpappwin.h"
#include "logindialog.h"
#include "mainbox.h"
#include "taskbox.h"
#include "tree.h"

#include <libwaftp.h>

struct _MainBox {
	GtkBox parent;
	GtkStackPage *page;
	FtpAppWindow *win;
	LoginDialog *login_dialog;

	GTask *listing_task;
	GAsyncQueue *path_queue;
	GAsyncQueue *list_queue;
	GQueue *iter_queue;

	GtkTreeStore *tree;
	GtkTreeView *tree_view;
	GtkTreeViewColumn *icon_column;
	GtkTreeViewColumn *size_column;
	GtkTreeViewColumn *modify_column;

	struct UserPI *user_pi;
	struct LoginInfo login_info;
};

G_DEFINE_TYPE(MainBox, main_box, GTK_TYPE_BOX);

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
		free(in_msg->path);
		free(in_msg);
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
		report_ftp_error(GTK_WINDOW(args->box->win),
		                 msg->err_msg->where, msg->err_msg->msg);
		msg->list = NULL;
	} else {
		g_print("%s", msg->list);
		g_print("format: %d\n", msg->format);
	}

	update_children(msg->list, msg->format, args->box->tree,
	                g_queue_pop_tail(args->box->iter_queue),
	                GTK_WINDOW(args->box->win));
	g_free(args);
	free(msg->list);
	free(msg);
	return false;
}

static void list_directory_async(MainBox *box, GtkTreeIter *parent)
{
	char *path = iter_to_path(parent, GTK_TREE_MODEL(box->tree));
	if (!path) {
		report_ftp_error(GTK_WINDOW(box->win), __func__,
		                 "Can't get directory path.");
		return;
	}
	g_async_queue_push(box->path_queue, path_msg(path));
	struct Args_get_dir_listing *args =
		g_new(struct Args_get_dir_listing, 1);
	args->box = box;
	g_queue_push_head(box->iter_queue, parent);
	g_idle_add(G_SOURCE_FUNC(get_dir_listing), args);
}

static void download(MainBox *box, GtkTreeIter *iter, GtkTreeModel *tree)
{
	char *name = iter_get_name(iter, tree);
	ssize_t size = iter_get_size(iter, tree);
	char *path = iter_to_path(iter, tree);
	task_box_new(box->win, box->user_pi, &box->login_info, path, name,
	             size);
}

void tree_view_on_row_activated(GtkTreeView *self, GtkTreePath *path,
                                GtkTreeViewColumn *column, gpointer user_data)
{
	MainBox *box = MAIN_BOX(user_data);
	GtkTreeIter *iter = g_new(GtkTreeIter, 1);
	if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(box->tree), iter, path)) {
		report_ftp_error(GTK_WINDOW(box->win), __func__,
		                 "Can't get iterator.");
		return;
	}
	if (iter_is_dir(box->tree, iter)) {
		list_directory_async(box, iter);
		return;
	}
	download(box, iter, GTK_TREE_MODEL(box->tree));
}

static void main_box_init(MainBox *b)
{
	gtk_widget_init_template(GTK_WIDGET(b));
	b->listing_task = g_task_new(b, NULL, NULL, NULL);
	b->list_queue = g_async_queue_new();
	b->path_queue = g_async_queue_new();
	b->iter_queue = g_queue_new();
	b->tree = tree_store_new();
	gtk_tree_view_set_model(b->tree_view, GTK_TREE_MODEL(b->tree));
	g_object_unref(b->tree);
	GtkCellRenderer *renderer;
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(b->icon_column, renderer, true);
	gtk_tree_view_column_set_cell_data_func(
		b->icon_column, renderer, cell_data_func_icon, NULL, NULL);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(b->size_column, renderer, true);
	gtk_tree_view_column_set_cell_data_func(
		b->size_column, renderer, cell_data_func_size, NULL, NULL);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(b->modify_column, renderer, true);
	gtk_tree_view_column_set_cell_data_func(
		b->modify_column, renderer, cell_data_func_modify, NULL, NULL);
}

static void main_box_dispose(GObject *object)
{
	MainBox *self = MAIN_BOX(object);
	g_async_queue_push_front(self->path_queue, shutdown_msg());
	g_async_queue_unref(self->list_queue);
	g_async_queue_unref(self->path_queue);
	g_object_unref(self->listing_task);
	g_queue_free(self->iter_queue);
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
	gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(class),
	                                        tree_view_on_row_activated);
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
	list_directory_async(box, NULL);
}
