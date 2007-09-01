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


#ifndef UI_CLIST_EDIT_H
#define UI_CLIST_EDIT_H


typedef struct _ClistEditData ClistEditData;
struct _ClistEditData
{
	GtkWidget *window;
	GtkWidget *entry;

	gchar *old_name;
	gchar *new_name;

	gint (*edit_func)(ClistEditData *ced, const gchar *oldname, const gchar *newname, gpointer data);
	gpointer edit_data;

	GtkCList *clist;
	gint row;
	gint column;
};


/*
 * edit_func: return TRUE is rename successful, FALSE on failure.
 */
gint clist_edit_by_row(GtkCList *clist, gint row, gint column,
		       gint (*edit_func)(ClistEditData *, const gchar *, const gchar *, gpointer), gpointer data);

/*
 * use this when highlighting a right-click menued or dnd clist row.
 */
void clist_edit_set_highlight(GtkWidget *clist, gint row, gint set);

/*
 * Useful for alternating dark/light rows in lists.
 */
void clist_edit_shift_color(GtkStyle *style);

/*
 * Various g_list utils, do not really fit anywhere, so they are here.
 */
GList *uig_list_insert_link(GList *list, GList *link, gpointer data);
GList *uig_list_insert_list(GList *parent, GList *insert_link, GList *list);


#endif

