#include <gtk/gtk.h>
#include <stdio.h>

#include "err.h"
#include "taskbox.h"

struct _TaskBox {
	GtkBox parent;

	ssize_t size;
	FtpAppWindow *win;
	GtkLabel *file_name;
	GtkLabel *file_size;
	GtkProgressBar *progress;
};

G_DEFINE_TYPE(TaskBox, task_box, GTK_TYPE_BOX);

struct TaskData {
	const struct UserPI *user_pi;
	char *path;
	char *name;
	const struct LoginInfo *login;
	TaskBox *task_box;
	GAsyncQueue *queue;
};

struct Msg {
	ssize_t downloaded;
	struct ErrMsg *err;
	TaskBox *task_box;
	bool finished;
};

static void update(struct Msg *msg, size_t *num_active_tasks)
{
	TaskBox *task_box = msg->task_box;
	if (msg->downloaded == -1) {
		report_ftp_error(GTK_WINDOW(task_box->win), msg->err->where,
		                 msg->err->msg);
		free(msg->err);
		*num_active_tasks = *num_active_tasks - 1;
		return;
	}
	if (msg->finished) {
		gtk_progress_bar_set_fraction(task_box->progress, 1);
		*num_active_tasks = *num_active_tasks - 1;
		return;
	}
	gtk_progress_bar_set_fraction(task_box->progress,
	                              (double)msg->downloaded / task_box->size);
}

bool update_task_boxes(FtpAppWindow *win)
{
	GAsyncQueue *queue = get_queue(win);
	size_t *num_active_tasks = get_active_tasks(win);
	int len = g_async_queue_length(queue);
	for (int i = 0; i < len; i++) {
		struct Msg *msg = g_async_queue_try_pop(queue);
		update(msg, num_active_tasks);
		free(msg);
	}
	g_assert(*num_active_tasks >= 0);
	if (*num_active_tasks == 0)
		return G_SOURCE_REMOVE;
	return G_SOURCE_CONTINUE;
}

static struct Msg *err_msg(struct ErrMsg *err, TaskBox *task_box)
{
	struct Msg *msg = g_new(struct Msg, 1);
	*msg = (struct Msg){ .downloaded = -1,
		             .err = err,
		             .task_box = task_box };
	return msg;
}

static struct Msg *finish_msg(TaskBox *task_box)
{
	struct Msg *msg = g_new(struct Msg, 1);
	*msg = (struct Msg){
		.task_box = task_box,
		.finished = true,
	};
	return msg;
}

static struct Msg *ok_msg(ssize_t downloaded, TaskBox *task_box)
{
	struct Msg *msg = g_new(struct Msg, 1);
	*msg = (struct Msg){ .task_box = task_box, .downloaded = downloaded };
	return msg;
}

void download_thread(GTask *task, GObject *source_object, gpointer task_data,
                     GCancellable *cancellable)
{
	struct TaskData *data = task_data;
	GAsyncQueue *queue = data->queue;
	TaskBox *task_box = data->task_box;
	struct UserPI user_pi;
	struct ErrMsg *err = g_new(struct ErrMsg, 1);
	strcpy(err->where, __func__);

	if (user_pi_clone(data->user_pi, &user_pi, data->login, err) < 0) {
		g_async_queue_push(queue, err_msg(err, task_box));
		return;
	}

	if (download_init(&user_pi, data->path, err) < 0) {
		g_async_queue_push(queue, err_msg(err, task_box));
		return;
	}

	ssize_t downloaded = 0;
	FILE *f = fopen(data->name, "w");
	if (!f) {
		strerror_r(errno, err->msg, ERR_MSG_MAX_LEN);
		g_async_queue_push(queue, err_msg(err, task_box));
		return;
	}

	const size_t chunk_size = 16 * 1024;
	char *buf = malloc(chunk_size);
	ssize_t n;
	for (;;) {
		n = download_chunk(&user_pi, buf, chunk_size, err);
		if (n < 0) {
			g_async_queue_push(queue, err_msg(err, task_box));
			goto finish;
		}
		if (n == 0) {
			g_async_queue_push(queue, finish_msg(task_box));
			goto finish;
		}
		fwrite(buf, 1, n, f);
		downloaded += n;
		g_async_queue_push(queue, ok_msg(downloaded, task_box));
	}
finish:
	fclose(f);
	free(buf);
	free(err);
}

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
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), TaskBox,
	                                     file_name);
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), TaskBox,
	                                     file_size);
	gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), TaskBox,
	                                     progress);
}

TaskBox *task_box_new(FtpAppWindow *win, const struct UserPI *user_pi,
                      const struct LoginInfo *login, char *path, char *name,
                      ssize_t size)
{
	TaskBox *self = g_object_new(TASK_BOX_TYPE, NULL);
	self->win = win;
	self->size = size;
	add_task_box(win, self);
	char *size_str = g_format_size(size);
	gtk_label_set_text(self->file_size, size_str);
	gtk_label_set_text(self->file_name, name);
	g_free(size_str);

	GTask *task = g_task_new(self, NULL, NULL, NULL);
	struct TaskData *data = g_new(struct TaskData, 1);
	*data = (struct TaskData){ .path = path,
		                   .name = name,
		                   .user_pi = user_pi,
		                   .login = login,
		                   .task_box = self,
		                   .queue = get_queue(win) };
	g_task_set_task_data(task, data, g_free);
	g_task_run_in_thread(task, download_thread);
	return self;
}
