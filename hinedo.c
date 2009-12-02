/**
 * Copyright (c) 2007 洪任諭 (PCMan) <pcman.tw@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>

#define LIBDIR        PREFIX"/lib/hinedo"

enum
{
    MARK_BEGIN = 1,
    MARK_END
};

static const char *range_key = "range_key";
static char* current = NULL;
static GPid pid = 0;
static GSList *group;
static gint default_position;

static void show_error( const char* msg )
{
    GtkWidget* dlg;
    dlg = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
                    GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s", msg );
    gtk_dialog_run( (GtkDialog*)dlg );
    gtk_widget_destroy( dlg );
}

static char* get_file( const char* name )
{
    return g_build_filename( g_get_user_config_dir(), "hinedo", name, NULL );
}

static void kill_player()
{
    int status;
    kill ( pid, SIGTERM );
    waitpid( pid, &status, 0 );
    pid = 0;
}

static void play_radio( const char* id )
{
    char* argv[3] = { NULL, NULL, NULL };
    GError* err = NULL;

    g_free (current);
    current = g_strdup (id);

    argv[0] = get_file( "play" );
    argv[1] = current;

    if( pid )
        kill_player();

    g_spawn_async( NULL, argv, NULL, 0, NULL, NULL, &pid, &err );
    g_free (argv[0]);

    if( err )
    {
        show_error( err->message );
        g_error_free( err );
    }
}

static void on_update_menu( GtkWidget* item, gpointer user_data )
{
    GtkWidget* dlg;
    char* cmd = get_file( "update_menu" );
    GError* err = NULL;
    if( g_spawn_command_line_sync( cmd, NULL, NULL, NULL, &err ) )
    {
        dlg = gtk_message_dialog_new( NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "電台清單更新完成！" );
        gtk_dialog_run (GTK_DIALOG (dlg));
        gtk_widget_destroy( dlg );
    }
    else
    {
        show_error( err->message );
        g_error_free( err );
    }

    g_free( cmd );
}

static void on_menu( GtkWidget* item, gpointer user_data )
{
    if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (item)))
        play_radio ((const char*) user_data);
}

void on_stop( GtkCheckMenuItem* item, gpointer user_data )
{
    if( item->active )
    {
        kill_player();
        current = NULL;
    }
}

static void on_about( GtkWidget* item, gpointer user_data )
{
    const gchar *authors[] ={ "洪任諭 (PCMan) <pcman.tw@gmail.com>\n", NULL };
    static GtkWidget * dlg = NULL;

    if( dlg )
        return;

    dlg = gtk_about_dialog_new ();
    gtk_about_dialog_set_logo_icon_name( GTK_ABOUT_DIALOG ( dlg ), "hinedo" );
    gtk_about_dialog_set_version ( GTK_ABOUT_DIALOG ( dlg ), VERSION );
    gtk_about_dialog_set_name ( GTK_ABOUT_DIALOG ( dlg ), _( "Hinedo 網路電台選播器" ) );
    gtk_about_dialog_set_copyright ( GTK_ABOUT_DIALOG ( dlg ), _( "Copyright (C) 2007 by PCMan" ) );
    gtk_about_dialog_set_comments ( GTK_ABOUT_DIALOG ( dlg ), _( "讓你輕鬆收聽 Hinet Radio 網路廣播" ) );
    gtk_about_dialog_set_license ( GTK_ABOUT_DIALOG ( dlg ), "This program is free software; you can redistribute it and/or\nmodify it under the terms of the GNU General Public License\nas published by the Free Software Foundation; either version 2\nof the License, or (at your option) any later version.\n\nThis program is distributed in the hope that it will be useful,\nbut WITHOUT ANY WARRANTY; without even the implied warranty of\nMERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\nGNU General Public License for more details.\n\nYou should have received a copy of the GNU General Public License\nalong with this program; if not, write to the Free Software\nFoundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA." );
    gtk_about_dialog_set_website ( GTK_ABOUT_DIALOG ( dlg ), "http://rt.openfoundry.org/Foundry/Project/?Queue=814" );
    gtk_about_dialog_set_authors ( GTK_ABOUT_DIALOG ( dlg ), authors );

    gtk_dialog_run( GTK_DIALOG( dlg ) );
    gtk_widget_destroy( dlg );
    dlg = NULL;
}

static void
on_status_icon_activate_event (GtkStatusIcon *status_icon,
                               gpointer       menu)
{
    gtk_menu_popup (menu, NULL, NULL,
                    gtk_status_icon_position_menu, status_icon,
                    0, gtk_get_current_event_time ());
}

static void
remove_playlist (GtkMenu *popup)
{
    GList *children;
    GList *list;
    gpointer data;
    gboolean found;

    children = gtk_container_get_children (GTK_CONTAINER (popup));
    list = children;
    found = FALSE;

    while (list)
    {
        data = g_object_get_data (G_OBJECT (list->data), range_key);
        switch (GPOINTER_TO_INT (data))
        {
        case MARK_BEGIN:
            found = TRUE;
            break;

        case MARK_END:
            found = FALSE;
            break;

        default:
            if (found)
            {
                gtk_container_remove (GTK_CONTAINER (popup),
                                      GTK_WIDGET (list->data));
            }
            break;
        }

        list = g_list_next (list);
    }

    g_list_free (children);
}

static void
populate_playlist (GtkMenu *popup)
{
    GtkWidget *category = NULL;
    GtkWidget *item = NULL;
    gint position = default_position;
    FILE* fi;
    char buf[1024], *name, *id, *tab, *fn;

    remove_playlist (popup);

    fn = get_file( "menu" );
    fi=fopen(fn, "r");
    g_free( fn );

    if( ! fi )
        return;

    while( fgets( buf, sizeof(buf), fi ) ) {
        name = strtok( buf, "\r\n");
        if( !name )
            continue;
        tab = strchr( name, '\t' );
        if( tab )    /* channel */
        {
            if(category )
            {
                id = tab + 1;
                *tab = '\0';

                item = gtk_radio_menu_item_new_with_label (group, name);
                group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (item));
                if( current && 0 == strcmp( id, current ) )
                    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), TRUE);
                g_signal_connect_data (item, "toggled", G_CALLBACK (on_menu),
                                       g_strdup (id), (GClosureNotify)g_free, 0);
                gtk_menu_shell_append (GTK_MENU_SHELL (category), item );
                gtk_widget_show (item);
            }
        }
        else if( *name )    /* category */
        {
            item = gtk_menu_item_new_with_label( name );
            category = gtk_menu_new();
            gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), category );
            gtk_menu_shell_insert (GTK_MENU_SHELL (popup),
                                   item, position);
            gtk_widget_show_all (item);

            ++position;
        }
        else    /* end of category */
        {
            category = NULL;
        }
    }
    fclose( fi );
}

/* Looking for update of hinedo */
static gboolean
check_update (GtkMenu *popup)
{
    int status = 0;
    char* error = NULL;
    if( ! g_spawn_command_line_sync( LIBDIR "/update", &error, NULL, &status, NULL ) || status != 0 )
    {
        if( status != 0 )
        {
            if( error )
            {
                show_error( error );
                g_free( error );
            }

            /* exit elegantly */
            gtk_main_quit ();

            return FALSE;
        }
        g_free( error );
    }

    populate_playlist (popup);

    return FALSE;
}

static void
init_ui ()
{
    GtkStatusIcon *status_icon;
    GtkWidget *item;
    GtkMenu *popup;

    /* init popup menu */
    popup = GTK_MENU (gtk_menu_new ());

    item = gtk_radio_menu_item_new_with_label (NULL, "停止播放");
    group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (item));
    gtk_menu_shell_append (GTK_MENU_SHELL (popup), item);
    g_signal_connect (G_OBJECT (item), "toggled",
                      G_CALLBACK (on_stop), NULL);
    ++default_position;

    item = gtk_image_menu_item_new_with_label ("線上更新電台清單");
    gtk_menu_shell_append (GTK_MENU_SHELL (popup), item);
    g_signal_connect (G_OBJECT (item), "activate",
                      G_CALLBACK (on_update_menu), NULL);
    ++default_position;

    item = gtk_separator_menu_item_new ();
    g_object_set_data (G_OBJECT (item), range_key,
                       GINT_TO_POINTER (MARK_BEGIN));
    gtk_menu_shell_append (GTK_MENU_SHELL (popup), item);
    ++default_position;

    item = gtk_separator_menu_item_new ();
    g_object_set_data (G_OBJECT (item), range_key,
                       GINT_TO_POINTER (MARK_END));
    gtk_menu_shell_append (GTK_MENU_SHELL (popup), item);

    item = gtk_image_menu_item_new_from_stock (GTK_STOCK_ABOUT, NULL);
    gtk_menu_shell_append (GTK_MENU_SHELL (popup), item);
    g_signal_connect (G_OBJECT (item), "activate",
                      G_CALLBACK (on_about), NULL);

    item = gtk_image_menu_item_new_from_stock (GTK_STOCK_QUIT, NULL);
    gtk_menu_shell_append (GTK_MENU_SHELL (popup), GTK_WIDGET (item));
    g_signal_connect (G_OBJECT (item), "activate",
                      G_CALLBACK (gtk_main_quit), NULL);

    gtk_widget_show_all (GTK_WIDGET (popup));

    /* init status icon */
    status_icon = gtk_status_icon_new_from_icon_name ("hinedo");
    gtk_status_icon_set_tooltip_text (status_icon, "Hinedo 電台點播器");
    g_signal_connect (G_OBJECT (status_icon), "activate",
                      G_CALLBACK (on_status_icon_activate_event), popup);

    gtk_status_icon_set_visible (status_icon, TRUE);

    /* install idle update */
    g_idle_add ((GSourceFunc) check_update, popup);
}

int main( int argc, char** argv )
{
    gtk_init( &argc, &argv );

    /* MUST init UI before check update */
    init_ui ();

    /* go event loop */
    gtk_main();

    /* clean up */
    if( pid > 0 )
        kill_player();

    return 0;
}
