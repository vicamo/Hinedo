/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * hinedo-applet.c
 * Copyright (C) You-Sheng Yang 2009 <vicamo@gmail.com>
 *
 * hinedo-applet is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * hinedo-applet is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <string.h>

#include "hinedo-applet.h"

G_DEFINE_TYPE (HinedoApplet, hinedo_applet, PANEL_TYPE_APPLET);

static void
popup_menu_position (GtkMenu   *menu,
                     int       *x,
                     int       *y,
                     gboolean  *push_in,
                     GtkWidget *widget)
{
    GtkRequisition requisition;
    gint menu_xpos;
    gint menu_ypos;

    gtk_widget_size_request (GTK_WIDGET (menu), &requisition);

    gdk_window_get_origin (widget->window, &menu_xpos, &menu_ypos);

    menu_xpos += widget->allocation.x;
    menu_ypos += widget->allocation.y;

    switch (panel_applet_get_orient (PANEL_APPLET (widget)))
    {
    case PANEL_APPLET_ORIENT_DOWN:
    case PANEL_APPLET_ORIENT_UP:
        if (menu_ypos > gdk_screen_get_height (gtk_widget_get_screen (widget)) / 2)
            menu_ypos -= requisition.height;
        else
            menu_ypos += widget->allocation.height;
        break;

    case PANEL_APPLET_ORIENT_RIGHT:
    case PANEL_APPLET_ORIENT_LEFT:
        if (menu_xpos > gdk_screen_get_width (gtk_widget_get_screen (widget)) / 2)
            menu_xpos -= requisition.width;
        else
            menu_xpos += widget->allocation.width;
        break;

    default:
            g_assert_not_reached ();
    }

    *x = menu_xpos;
    *y = menu_ypos;
    *push_in = TRUE;
}

static void
on_radio_menu_item_signal_group_changed (GtkRadioMenuItem *menu_item,
                                         HinedoApplet     *applet)
{
}

static gboolean
on_widget_signal_button_press_event (GtkWidget      *widget,
                                     GdkEventButton *event,
                                     HinedoApplet   *applet)
{
    /* React only to single left mouse button click */
    if ((event->button == 1)
        && (event->type != GDK_2BUTTON_PRESS)
        && (event->type != GDK_3BUTTON_PRESS))
    {
        gtk_menu_popup (applet->playlist_menu, NULL, NULL,
                        (GtkMenuPositionFunc) popup_menu_position,
                        applet, event->button, event->time);
        return TRUE;
    }

    return FALSE;
}

static void
on_panel_applet_signal_change_background (GtkWidget                *applet,
                                          PanelAppletBackgroundType type,
                                          GdkColor                 *color,
                                          GdkPixmap                *pixmap,
                                          gpointer                  user_data)
{
    /* taken from the AlarmClockApplet */
    GtkRcStyle *rc_style;
    GtkStyle *style;

    /* reset style */
    gtk_widget_set_style (applet, NULL);
    rc_style = gtk_rc_style_new ();
    gtk_widget_modify_style (applet, rc_style);
    gtk_rc_style_unref (rc_style);

    switch (type)
    {
    case PANEL_COLOR_BACKGROUND:
        gtk_widget_modify_bg (applet, GTK_STATE_NORMAL, color);
        break;

    case PANEL_PIXMAP_BACKGROUND:
        style = gtk_style_copy (applet->style);
        if (style->bg_pixmap[GTK_STATE_NORMAL])
            g_object_unref (style->bg_pixmap[GTK_STATE_NORMAL]);
        style->bg_pixmap[GTK_STATE_NORMAL] = g_object_ref (pixmap);
        gtk_widget_set_style (applet, style);
        g_object_unref (style);
        break;

    case PANEL_NO_BACKGROUND:
    default:
        break;
    }
}

static void
on_panel_applet_signal_change_orient (PanelApplet *panelapplet,
                                      guint        arg1,
                                      gpointer     user_data)
{
}

static void
on_panel_applet_signal_change_size (PanelApplet *panelapplet,
                                    guint        arg1,
                                    gpointer     user_data)
{
}

static void
on_panel_applet_signal_move_focus_out_of_applet (PanelApplet     *panelapplet,
                                                 GtkDirectionType arg1,
                                                 gpointer         user_data)
{
}

static void
hinedo_applet_setup_ui (HinedoApplet *applet)
{
    GtkWidget *menu;
    GtkWidget *menu_item;
    GtkWidget *image;
    GSList *group;

    /* playlist popup menu */
    menu = gtk_menu_new ();
    applet->playlist_menu = GTK_MENU (menu);
    g_object_ref (G_OBJECT (menu));

    gtk_widget_show (menu);

    /* the stop menu item */
    menu_item = gtk_radio_menu_item_new_with_mnemonic (NULL, "Stop");
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

    group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (menu_item));
    gtk_widget_show (menu_item);
    g_signal_connect (G_OBJECT (menu_item), "group-changed",
                      G_CALLBACK (on_radio_menu_item_signal_group_changed), applet);

    /* separator, not shown at beginning. */
    menu_item = gtk_separator_menu_item_new ();
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

    /* applet icon */
    image = gtk_image_new_from_file (PACKAGE_DATA_DIR "/pixmaps/hinedo.png");
    gtk_container_add (GTK_CONTAINER (applet), image);
    gtk_widget_show (image);

    /* connect signals */
    g_signal_connect (G_OBJECT (applet), "button-press-event",
                      G_CALLBACK (on_widget_signal_button_press_event), applet);
    g_signal_connect (G_OBJECT (applet), "change-background",
                      G_CALLBACK (on_panel_applet_signal_change_background), applet);
    g_signal_connect (G_OBJECT (applet), "change-orient",
                      G_CALLBACK (on_panel_applet_signal_change_orient), applet);
    g_signal_connect (G_OBJECT (applet), "change-size",
                      G_CALLBACK (on_panel_applet_signal_change_size), applet);
    g_signal_connect (G_OBJECT (applet), "move-focus-out-of-applet",
                      G_CALLBACK (on_panel_applet_signal_move_focus_out_of_applet), applet);
}

static void
hinedo_applet_init (HinedoApplet *applet)
{
    /* panel applet */
    panel_applet_set_flags (PANEL_APPLET (applet), PANEL_APPLET_EXPAND_MINOR);

    /* Set up applet */
    hinedo_applet_setup_ui (applet);
}

static void
hinedo_applet_finalize (HinedoApplet *applet)
{
    g_object_unref (G_OBJECT (applet->playlist_menu));

    G_OBJECT_CLASS (hinedo_applet_parent_class)->finalize (G_OBJECT (applet));
}

static void
hinedo_applet_class_init (HinedoAppletClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = (GObjectFinalizeFunc) hinedo_applet_finalize;
}

static void
on_panel_applet_callback_preference (BonoboUIComponent *component,
                                     HinedoApplet      *applet,
                                     const gchar       *cname)
{
}

static void
on_panel_applet_callback_about (BonoboUIComponent *component,
                                HinedoApplet      *applet,
                                const gchar       *cname)
{
    static const gchar* const authors[] = {
        "PCMan <pcman.tw@gmail.com>",
        "You-Sheng Yang <vicamo@gmail.com>",
        NULL
    };
    static const gchar* const documenters[] = {
        "PCMan <pcman.tw@gmail.com>",
        "You-Sheng Yang <vicamo@gmail.com>",
        NULL
    };
    static const gchar* const artists[] = {
        "PCMan <pcman.tw@gmail.com>",
        NULL
    };

    gtk_show_about_dialog (NULL,
                          "program-name",   PACKAGE_NAME,
                          "title",          _("About " PACKAGE_NAME),
                          "version",        PACKAGE_VERSION,
                          "copyright",      "\xC2\xA9 2009 Vicamo Yang\n\xC2\xA9 2007 PCMan",
                          "website",        PACKAGE_URL,
                          "authors",        authors,
                          "documenters",    documenters,
                          "artists",        artists,
                          "logo-icon-name", "hinedo",
                          "wrap-license",   TRUE,
                          "license",
                          "This program is free software; you can redistribute "
                          "it and/or modify it under the terms of the GNU "
                          "General Public License as published by the Free "
                          "Software Foundation; either version 2 of the "
                          "License, or (at your option) any later version.\n\n"
                          "This program is distributed in the hope that it "
                          "will be useful, but WITHOUT ANY WARRANTY; without "
                          "even the implied warranty of MERCHANTABILITY or "
                          "FITNESS FOR A PARTICULAR PURPOSE. See the GNU "
                          "General Public License for more details.\n\n"
                          "You should have received a copy of the GNU General "
                          "Public License along with this program; if not, "
                          "write to the Free Software Foundation, Inc., 51 "
                          "Franklin Street, Fifth Floor, Boston, MA "
                          "02110-1301, USA.",
                          NULL);
}

static void
setup_menu (HinedoApplet *applet)
{
    static const gchar *xml =
        "<popup name=\"button3\">\n"
            "<menuitem name=\"Preferences Item\" "
                      "verb=\"Preferences\" "
                      "_label=\"_Preferences...\" "
                      "pixtype=\"stock\" "
                      "pixname=\"gtk-properties\"/>\n"
            "<menuitem name=\"About Item\" "
                      "verb=\"About\" "
                      "_label=\"_About...\" "
                      "pixtype=\"stock\" "
                      "pixname=\"gtk-about\"/>\n"
        "</popup>\n";

    static const BonoboUIVerb verbs [] = {
        BONOBO_UI_VERB ("Preferences", (BonoboUIVerbFn) on_panel_applet_callback_preference),
        BONOBO_UI_VERB ("About",       (BonoboUIVerbFn) on_panel_applet_callback_about),
        BONOBO_UI_VERB_END
    };

    panel_applet_setup_menu (PANEL_APPLET (applet), xml, verbs, applet);
}

static gboolean
applet_factory (HinedoApplet *applet,
                const gchar  *iid,
                gpointer      user_data)
{
    if (strcmp (iid, "OAFIID:hinedo-appletApplet") != 0)
        return FALSE;

    /* add preference, about menu */
    setup_menu (applet);

    gtk_widget_show_all (GTK_WIDGET (applet));

    return TRUE;
}

PANEL_APPLET_BONOBO_FACTORY ("OAFIID:hinedo-appletApplet_Factory",
                             HINEDO_TYPE_APPLET,
                             "Hinedo Applet",
                             "0",
                             (PanelAppletFactoryCallback) applet_factory,
                             NULL);
