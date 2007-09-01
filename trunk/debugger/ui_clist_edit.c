/*
 * (SLIK) SimpLIstic sKin functions
 * (C) 2002 John Ellis
 *
 * Author: John Ellis
 *
 * This software is released under the GNU General Public License (GNU GPL).
 * Please read the included file COPYING for more information.
 * This software comes with no warranty of any kind, use at your own risk!
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "ui_clist_edit.h"


#define POPUP_H_PADDING 3
#define COLUMN_PADDING 3	/* stolen from gtkclist.c */
#define COLUMN_SPACING 1	/* same */

static void clist_edit_get_geom(ClistEditData *ced, gint *x, gint *y, gint *w, gint *h)
{
	*x = GTK_CLIST(ced->clist)->column[(ced->column)].area.x + GTK_CLIST(ced->clist)->hoffset - COLUMN_SPACING - COLUMN_PADDING;
	*y = GTK_CLIST(ced->clist)->row_height * (ced->row) + (((ced->row) + 1) * 1) + GTK_CLIST(ced->clist)->voffset;

	*w = GTK_CLIST(ced->clist)->column[(ced->column)].area.width + (COLUMN_SPACING + COLUMN_PADDING) * 2;
	*h = GTK_CLIST(ced->clist)->row_height;
}

static void clist_edit_make_row_visible(ClistEditData *ced)
{
	gint nrow, ncol;
	gfloat frow, fcol;
	gint x, y, w, h;

	nrow = ncol = -1;
	frow = fcol = 0.0;

	if (gtk_clist_row_is_visible(ced->clist, ced->row) != GTK_VISIBILITY_FULL)
		{
		nrow = ced->row;
		if (ced->row != 0 && gtk_clist_row_is_visible(ced->clist, ced->row - 1) != GTK_VISIBILITY_NONE)
			{
			frow = 1.0;
			}
		}

	clist_edit_get_geom(ced, &x, &y, &w, &h);
	if (x < 0)
		{
		ncol = ced->column;
		}
	else if (x + w > ced->clist->clist_window_width)
		{
		ncol = ced->column;
		fcol = 1.0;
		}

	if (ncol >= 0 || nrow >= 0) gtk_clist_moveto(ced->clist, nrow, ncol, frow, fcol);
}

static void clist_edit_close(ClistEditData *ced)
{
	gtk_grab_remove(ced->window);
	gdk_pointer_ungrab(GDK_CURRENT_TIME);

	gtk_widget_destroy(ced->window);

	g_free(ced->old_name);
	g_free(ced->new_name);

	g_free(ced);
}

static void clist_edit_do(ClistEditData *ced)
{
	ced->new_name = g_strdup(gtk_entry_get_text(GTK_ENTRY(ced->entry)));

	if (strcmp(ced->new_name, ced->old_name) != 0)
		{
		if (ced->edit_func)
			{
			if (ced->edit_func(ced, ced->old_name, ced->new_name, ced->edit_data))
				{
				gtk_clist_set_text(GTK_CLIST(ced->clist), ced->row, ced->column, ced->new_name);
				}
			}
		}
}

static gint clist_edit_click_end_cb(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	ClistEditData *ced = data;

	clist_edit_do(ced);
	clist_edit_close(ced);

	return TRUE;
}

static gint clist_edit_click_cb(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	ClistEditData *ced = data;

	gint x, y;
	gint w, h;

	gint xr, yr;

	xr = (gint)event->x_root;
	yr = (gint)event->y_root;

	gdk_window_get_origin(ced->window->window, &x, &y);
	gdk_window_get_size(ced->window->window, &w, &h);

	if (xr < x || yr < y || xr > x + w || yr > y + h)
		{
		/* gobble the release event, so it does not propgate to an underlying widget */
		gtk_signal_connect(GTK_OBJECT(ced->window), "button_release_event",
				   GTK_SIGNAL_FUNC(clist_edit_click_end_cb), ced);
		return TRUE;
		}
	return FALSE;
}

static gint clist_edit_key_press_cb(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	ClistEditData *ced = data;

	switch (event->keyval)
		{
		case GDK_Return:
		case GDK_KP_Enter:
		case GDK_Tab: 		/* ok, we are going to intercept the focus change
					   from keyboard and act like return was hit */
		case GDK_ISO_Left_Tab:
		case GDK_Up:
		case GDK_Down:
		case GDK_KP_Up:
		case GDK_KP_Down:
		case GDK_KP_Left:
		case GDK_KP_Right:
			clist_edit_do(ced);
			clist_edit_close(ced);
			break;
		case GDK_Escape:
			clist_edit_close(ced);
			break;
		default:
			break;
		}

	return FALSE;
}

gint clist_edit_by_row(GtkCList *clist, gint row, gint column,
		       gint (*edit_func)(ClistEditData *, const gchar *, const gchar *, gpointer), gpointer data)
{
	ClistEditData *ced;
	gint x, y, w, h;	/* geometry of cell within clist */
	gint wx, wy;		/* geometry of clist from root window */
	gchar *text = NULL;
	guint8 spacing = 0;
	gint pix_width = 0;
	gint offset = 0;
	GdkPixmap *pixmap = NULL;
	GdkBitmap *mask;	/* bah, ...get_pixtext() is broke, can't be a NULL*  */

	if (row < 0 || row >= clist->rows) return FALSE;
	if (column < 0 || column >= clist->columns) return FALSE;
	if (!edit_func) return FALSE;
	if (!GTK_WIDGET_VISIBLE(clist)) return FALSE;

	if (!gtk_clist_get_text(clist, row, column, &text))
		{
		gtk_clist_get_pixtext(clist, row, column, &text, &spacing, &pixmap, &mask);
		}

	if (!text) text = "";

	if (pixmap) gdk_window_get_size(pixmap, &pix_width, NULL);

	ced = g_new0(ClistEditData, 1);

	ced->old_name = g_strdup(text);
	ced->new_name = NULL;

	ced->edit_func = edit_func;
	ced->edit_data = data;

	ced->clist = clist;
	ced->row = row;
	ced->column = column;

	clist_edit_make_row_visible(ced);

	/* figure the position of the rename window
	   (some borrowed from clist internals, may break in future ?) */

	if (GTK_IS_CTREE(clist))
		{
		GtkCTreeNode *node;
		GtkCTree *ctree;

		ctree = GTK_CTREE(clist);

		node = gtk_ctree_node_nth(ctree, (guint)row);
		if (node)
			{
			offset = GTK_CTREE_ROW(node)->level * ctree->tree_indent + ctree->tree_spacing;
			}
		}

	x = clist->column[column].area.x + clist->hoffset + pix_width + spacing - COLUMN_SPACING - COLUMN_PADDING + offset;
	y = clist->row_height * (row) + (((row) + 1) * 1) + clist->voffset;
  
	w = clist->column[column].area.width - pix_width - spacing - offset + (COLUMN_SPACING + COLUMN_PADDING) * 2;
	h = clist->row_height;
  
	gdk_window_get_origin(clist->clist_window, &wx, &wy);

	x += wx;
	y += wy;

	/* create the window */

	ced->window = gtk_window_new(GTK_WINDOW_POPUP);
	gtk_window_set_policy(GTK_WINDOW(ced->window), FALSE, FALSE, FALSE);
	gtk_signal_connect(GTK_OBJECT(ced->window), "button_press_event",
			   GTK_SIGNAL_FUNC(clist_edit_click_cb), ced);
	gtk_signal_connect(GTK_OBJECT(ced->window), "key_press_event",
			   GTK_SIGNAL_FUNC(clist_edit_key_press_cb), ced);

	ced->entry = gtk_entry_new_with_max_length(255);
	gtk_entry_set_text(GTK_ENTRY(ced->entry), ced->old_name);
	gtk_entry_select_region(GTK_ENTRY(ced->entry), 0, strlen(ced->old_name));
	gtk_container_add(GTK_CONTAINER(ced->window), ced->entry);
	gtk_widget_show(ced->entry);

	/* now show it */
	gtk_widget_set_uposition (ced->window, x, y - POPUP_H_PADDING);
	gtk_widget_set_usize (ced->window, w, h + (POPUP_H_PADDING * 2));
	gtk_widget_realize (ced->window);
	gdk_window_resize (ced->window->window, w, h - POPUP_H_PADDING);
	gtk_widget_show (ced->window);

	/* grab it */
	gtk_widget_grab_focus(ced->entry);
	/* explicitely set the focus flag for the entry, for some reason on popup windows this
	 * is not set, and causes no edit cursor to appear ( popups not allowed focus? )
	 */
	GTK_WIDGET_SET_FLAGS(ced->entry, GTK_HAS_FOCUS);
	gtk_grab_add(ced->window);
	gdk_pointer_grab(ced->window->window, TRUE,
			 GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_MOTION_MASK,
			 NULL, NULL, GDK_CURRENT_TIME);

	return TRUE;
}

/* adjusts highlights (for use when highlighting a right-clicked row when showing the pop-menu) */

void clist_edit_set_highlight(GtkWidget *clist, gint row, gint set)
{
	if (row < 0) return;

	if (set)
		{
		gtk_clist_set_background(GTK_CLIST(clist), row,
			&GTK_WIDGET(clist)->style->bg[GTK_STATE_ACTIVE]);
		gtk_clist_set_foreground(GTK_CLIST(clist), row,
			&GTK_WIDGET(clist)->style->fg[GTK_STATE_ACTIVE]);
		}
	else
		{
		gtk_clist_set_background(GTK_CLIST(clist), row, NULL);
		gtk_clist_set_foreground(GTK_CLIST(clist), row, NULL);
		}
}

static void shift_color(GdkColor *src)
{
	gshort cs = 0xffff / 100 * 10;	/* 10% shift */

	/* up or down ? */
	if (((gint)src->red + (gint)src->green + (gint)src->blue) / 3 > 0xffff / 2)
		{
		src->red = MAX(0 , src->red - cs);
		src->green = MAX(0 , src->green - cs);
		src->blue = MAX(0 , src->blue - cs);
		}
	else
		{
		src->red = MIN(0xffff, src->red + cs);
		src->green = MIN(0xffff, src->green + cs);
		src->blue = MIN(0xffff, src->blue + cs);
		}
}

/* darkens or lightens color, so that rows can be grouped in the list
 * esp. useful for alternating dark/light in lists
 */
void clist_edit_shift_color(GtkStyle *style)
{
	if (!style) return;

	shift_color(&style->base[GTK_STATE_NORMAL]);
	shift_color(&style->bg[GTK_STATE_SELECTED]);
}

GList *uig_list_insert_link(GList *list, GList *link, gpointer data)
{
	GList *new_list;

	if (!list || link == list) return g_list_prepend(list, data);
	if (!link) return g_list_append(list, data);

	new_list = g_list_alloc ();
	new_list->data = data;

	if (link->prev)
		{
		link->prev->next = new_list;
		new_list->prev = link->prev;
		}
	else
		{
		list = new_list;
		}
	link->prev = new_list;
	new_list->next = link;

	return list;
}

GList *uig_list_insert_list(GList *parent, GList *insert_link, GList *list)
{
	GList *end;

	if (!insert_link) return g_list_concat(parent, list);
	if (insert_link == parent) return g_list_concat(list, parent);
	if (!parent) return list;
	if (!list) return parent;

	end  = g_list_last(list);

	if (insert_link->prev) insert_link->prev->next = list;
	list->prev = insert_link->prev;
	insert_link->prev = end;
	end->next = insert_link;

	return parent;
}

