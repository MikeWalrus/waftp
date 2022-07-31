#include "tree.h"
#include "err.h"

enum {
	NAME_COLUMN,
	IS_DIR_COLUMN,
	SIZE_COLUMN,
	PERM_COLUMN,
	MODIFY_COLUMN,
	N_COLUMNS
};

GtkTreeStore *tree_store_new(void)
{
	return gtk_tree_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_BOOLEAN,
	                          G_TYPE_UINT64, G_TYPE_STRING, G_TYPE_UINT64);
}

void cell_data_func_icon(GtkTreeViewColumn *tree_column, GtkCellRenderer *cell,
                         GtkTreeModel *tree_model, GtkTreeIter *iter,
                         gpointer data)
{
	bool is_dir;
	ssize_t size;
	gtk_tree_model_get(tree_model, iter, IS_DIR_COLUMN, &is_dir,
	                   SIZE_COLUMN, &size, -1);
	const char *icon;
	if (size < 0)
		icon = "dialog-question-symbolic";
	else if (is_dir)
		icon = "folder-symbolic";
	else
		icon = "folder-documents-symbolic";

	g_object_set(G_OBJECT(cell), "icon-name", icon, NULL);
}

void cell_data_func_size(GtkTreeViewColumn *tree_column, GtkCellRenderer *cell,
                         GtkTreeModel *tree_model, GtkTreeIter *iter,
                         gpointer data)
{
	GValue value = G_VALUE_INIT;
	gtk_tree_model_get_value(tree_model, iter, SIZE_COLUMN, &value);
	ssize_t size = g_value_get_uint64(&value);
	g_value_unset(&value);
	gchar *size_string;
	if (size < 0) {
		size_string = strdup("Unknown");
	} else {
		size_string = g_format_size(size);
	}
	GValue text = G_VALUE_INIT;
	g_value_init(&text, G_TYPE_STRING);
	g_value_take_string(&text, size_string);
	g_object_set_property(G_OBJECT(cell), "text", &text);
	g_value_unset(&text);
}

static void set_modify_empty(GtkCellRenderer *cell)
{
	GValue modify = G_VALUE_INIT;
	g_value_init(&modify, G_TYPE_STRING);
	g_value_set_static_string(&modify, "");
	g_object_set_property(G_OBJECT(cell), "text", &modify);
}

void cell_data_func_modify(GtkTreeViewColumn *tree_column,
                           GtkCellRenderer *cell, GtkTreeModel *tree_model,
                           GtkTreeIter *iter, gpointer data)
{
	GValue value = G_VALUE_INIT;
	gtk_tree_model_get_value(tree_model, iter, MODIFY_COLUMN, &value);
	guint64 modify = g_value_get_uint64(&value);
	g_value_unset(&value);
	if (modify == 0) {
		set_modify_empty(cell);
		return;
	}
	GDateTime *modify_data_time = g_date_time_new_from_unix_utc(modify);
	if (!modify_data_time) {
		set_modify_empty(cell);
		return;
	}
	char *modify_string =
		g_date_time_format(modify_data_time, "%m/%d/%Y %H:%M");
	g_date_time_unref(modify_data_time);
	if (!modify_string) {
		set_modify_empty(cell);
		return;
	}

	GValue text = G_VALUE_INIT;
	g_value_init(&text, G_TYPE_STRING);
	g_value_take_string(&text, modify_string);
	g_object_set_property(G_OBJECT(cell), "text", &text);
	g_value_unset(&text);
}

static void remove_children(GtkTreeStore *tree, GtkTreeIter *parent)
{
	GtkTreeIter child;
	if (gtk_tree_model_iter_children(GTK_TREE_MODEL(tree), &child,
	                                 parent)) {
		while (gtk_tree_store_remove(tree, &child))
			;
	}
}

bool iter_is_dir(GtkTreeStore *tree, GtkTreeIter *iter)
{
	if (!gtk_tree_store_iter_is_valid(tree, iter))
		return false;
}

void update_children(char *list, enum ListFormat format, GtkTreeStore *tree,
                     GtkTreeIter *parent, GtkWindow *win)
{
	if (parent && !gtk_tree_store_iter_is_valid(tree, parent))
		return;

	remove_children(tree, parent);

	ParseLineListFunc parse_line;
	if (format == FORMAT_LIST) {
		parse_line = parse_line_list_gnu;
	} else {
		return;
	}

	const char *ptr = list;
	while (*ptr) {
		bool ignore;
		struct Fact fact;
		if (parse_line(ptr, &ignore, &ptr, &fact) < 0) {
			report_ftp_error(win, __func__,
			                 "Can't parse the directory listings.");
			return;
		}
		if (ignore)
			continue;
		GtkTreeIter iter;
		gtk_tree_store_append(tree, &iter, parent);
		gtk_tree_store_set(tree, &iter, NAME_COLUMN, fact.name,
		                   IS_DIR_COLUMN, fact.is_dir, SIZE_COLUMN,
		                   fact.size, PERM_COLUMN, fact.perm,
		                   MODIFY_COLUMN, fact.modify, -1);
		free(fact.name);
	}
}

char *iter_to_path(GtkTreeIter *iter, GtkTreeStore *tree)
{
	if (!iter)
		return strdup("");
	GtkTreeIter *i = gtk_tree_iter_copy(iter);
	GQueue *queue = g_queue_new();
	size_t len = 0;
	for (;;) {
		char *name;
		gtk_tree_model_get(GTK_TREE_MODEL(tree), i, NAME_COLUMN, &name,
		                   -1);
		len += strlen(name) + 1;
		g_queue_push_head(queue, name);
		GtkTreeIter parent;
		if (!gtk_tree_model_iter_parent(GTK_TREE_MODEL(tree), &parent,
		                                i))
			break;
		*i = parent;
	}
	char *path = malloc(len + 1);
	char *dest = path;
	for (;;) {
		char *name = g_queue_pop_head(queue);
		if (!name)
			break;
		char *src = name;
		while (*src) {
			*(dest++) = *(src++);
		}
		*(dest++) = '/';
		g_free(name);
	}
	*(dest - 1) = '\0';
	g_queue_free(queue);
	gtk_tree_iter_free(i);
	return path;
}
