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

#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>

#include "eggtrayicon.h"

#define LIBDIR        PREFIX"/lib/hinedo"

static const char* current = NULL;
static GPid pid = 0;

static void show_error( const char* msg )
{
    GtkWidget* dlg;
    dlg = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
                    GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, msg );
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
    char cmd[1024];
    char* argv[3] = { NULL, NULL, NULL };
    GError* err = NULL;

    argv[0] = get_file( "play" );
    argv[1] = id;

    if( pid )
        kill_player();

    g_spawn_async( NULL, argv, NULL, 0, NULL, NULL, &pid, &err );
    g_free( argv[0] );

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
        gtk_dialog_run( dlg );
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
    int i;
    char* id = (char*)g_object_get_data(item, "id");
    if( ! GTK_CHECK_MENU_ITEM(item)->active )
        return;
    g_free( current );
    current = g_strdup( id );
    play_radio( id );
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

void on_tray_icon_button_press_event( GtkWidget* widget, GdkEventButton* evt, gpointer user_data )
{
    GtkMenu* menu = gtk_menu_new();
    GtkWidget* category = NULL;
    GtkMenuItem* menu_item;
    int i;
    FILE* fi;
    GSList* group = NULL;
    char buf[1024], *name, *id, *tab, *fn;

    fn = get_file( "menu" );
    fi=fopen(fn, "r");
    g_free( fn );

    if( ! fi )
        return;

    menu_item = gtk_radio_menu_item_new_with_label( group, "停止播放" );
    group = gtk_radio_menu_item_get_group( menu_item );
    gtk_menu_shell_append( menu, menu_item );
    g_signal_connect( menu_item, "toggled", on_stop, NULL );

    menu_item = gtk_image_menu_item_new_with_label( "線上更新電台清單" );
    gtk_menu_shell_append( menu, menu_item );
    g_signal_connect( menu_item, "activate", on_update_menu, NULL );

    gtk_menu_shell_append( menu, gtk_separator_menu_item_new() );

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
                menu_item = gtk_radio_menu_item_new_with_label( group, name );
                group = gtk_radio_menu_item_get_group( menu_item );
                g_object_set_data_full( menu_item, "id", g_strdup( id ), g_free );
                if( current && 0 == strcmp( id, current ) )
                    gtk_check_menu_item_set_active( menu_item, TRUE );
                g_signal_connect( menu_item, "toggled", on_menu, NULL );
                gtk_menu_shell_append( category, menu_item );
            }
        }
        else if( *name )    /* category */
        {
            menu_item = gtk_menu_item_new_with_label( name );
            category = gtk_menu_new();
            gtk_menu_item_set_submenu( menu_item, category );
            gtk_menu_shell_append( menu, menu_item );
        }
        else    /* end of category */
        {
            category = NULL;
        }
    }
    fclose( fi );

    gtk_menu_shell_append( menu, gtk_separator_menu_item_new() );

    menu_item = gtk_image_menu_item_new_from_stock( GTK_STOCK_ABOUT, NULL );
    gtk_menu_shell_append( menu, menu_item );
    g_signal_connect( menu_item, "activate", on_about, NULL );

    menu_item = gtk_image_menu_item_new_from_stock( GTK_STOCK_QUIT, NULL );
    gtk_menu_shell_append( menu, menu_item );
    g_signal_connect( menu_item, "activate", gtk_main_quit, NULL );

    g_signal_connect( menu, "selection-done", gtk_widget_destroy, NULL );
    gtk_widget_show_all( menu );
    gtk_menu_popup( menu, NULL, NULL, NULL, NULL, evt->button, evt->time );
}

/* Looking for update of hinedo */
static void check_update()
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
            exit( 1 ); /* abort on error */
        }
        g_free( error );
    }
}

int main( int argc, char** argv )
{
    GtkWidget *icon;
    GtkWidget *evt_box;
    GtkTooltips *tooltips;
    EggTrayIcon *tray_icon;

    gtk_init( &argc, &argv );

    check_update();

    tooltips = gtk_tooltips_new ();
    tray_icon = egg_tray_icon_new ("Hinedo 電台點播器");
    evt_box = gtk_event_box_new ();
    gtk_container_add (GTK_CONTAINER (tray_icon), evt_box);
    g_signal_connect (G_OBJECT (evt_box), "button-press-event",
            G_CALLBACK (on_tray_icon_button_press_event), NULL);
    gtk_tooltips_set_tip (GTK_TOOLTIPS(tooltips), evt_box, "Hinedo 電台點播器", NULL);

    icon = gtk_image_new_from_icon_name( "hinedo", GTK_ICON_SIZE_MENU );
    gtk_container_add (GTK_CONTAINER (evt_box), icon);

    gtk_widget_show_all (GTK_WIDGET (tray_icon));
    gtk_main();
    if( pid > 0 )
        kill_player();

    return 0;
}
