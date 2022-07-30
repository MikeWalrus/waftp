#ifndef _TREE_H
#define _TREE_H

#include <gtk/gtk.h>

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
#endif
