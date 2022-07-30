#include "tree.h"

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
	GValue value;
	gtk_tree_model_get_value(tree_model, iter, IS_DIR_COLUMN, &value);
	bool is_dir = g_value_get_boolean(&value);
	g_value_unset(&value);

	GValue icon_name;
	g_value_init(&icon_name, G_TYPE_STRING);
	g_value_set_static_string(&icon_name,
	                          is_dir ? "folder-symbolic" :
	                                   "folder-documents-symbolic");
	g_object_set_property(G_OBJECT(cell), "icon-name", &icon_name);
}

void cell_data_func_size(GtkTreeViewColumn *tree_column, GtkCellRenderer *cell,
                         GtkTreeModel *tree_model, GtkTreeIter *iter,
                         gpointer data)
{
	GValue value;
	gtk_tree_model_get_value(tree_model, iter, SIZE_COLUMN, &value);
	guint64 size = g_value_get_uint64(&value);
	g_value_unset(&value);
	gchar *size_string = g_format_size(size);

	GValue text;
	g_value_init(&text, G_TYPE_STRING);
	g_value_take_string(&text, size_string);
	g_object_set_property(G_OBJECT(cell), "text", &text);
	g_object_unref(&text);
}

static void set_modify_empty(GtkCellRenderer *cell)
{
	GValue modify;
	g_value_init(&modify, G_TYPE_STRING);
	g_value_set_static_string(&modify, "");
	g_object_set_property(G_OBJECT(cell), "text", &modify);
}

void cell_data_func_modify(GtkTreeViewColumn *tree_column,
                           GtkCellRenderer *cell, GtkTreeModel *tree_model,
                           GtkTreeIter *iter, gpointer data)
{
	GValue value;
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
	g_free(modify_data_time);
	if (!modify_string) {
		set_modify_empty(cell);
		return;
	}

	GValue text;
	g_value_init(&text, G_TYPE_STRING);
	g_value_take_string(&text, modify_string);
	g_object_set_property(G_OBJECT(cell), "text", &text);
	g_object_unref(&text);
}
