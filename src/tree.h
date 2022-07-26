#ifndef _TREE_H
#define _TREE_H

#include <gtk/gtk.h>

#include "libwaftp.h"

GtkTreeStore *tree_store_new(void);

void cell_data_func_icon(GtkTreeViewColumn *tree_column, GtkCellRenderer *cell,
                         GtkTreeModel *tree_model, GtkTreeIter *iter,
                         gpointer data);

void cell_data_func_size(GtkTreeViewColumn *tree_column, GtkCellRenderer *cell,
                         GtkTreeModel *tree_model, GtkTreeIter *iter,
                         gpointer data);

void cell_data_func_modify(GtkTreeViewColumn *tree_column,
                           GtkCellRenderer *cell, GtkTreeModel *tree_model,
                           GtkTreeIter *iter, gpointer data);

void update_children(char *list, enum ListFormat format, GtkTreeStore *tree,
                     GtkTreeIter *parent, GtkWindow *win);

char *iter_to_path(GtkTreeIter *iter, GtkTreeModel *tree);

char *iter_get_name(GtkTreeIter *iter, GtkTreeModel *tree);

ssize_t iter_get_size(GtkTreeIter *iter, GtkTreeModel *tree);

bool iter_is_dir(GtkTreeStore *tree, GtkTreeIter *iter);

#endif
