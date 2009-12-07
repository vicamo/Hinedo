/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * main.c
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
#include <gst/gst.h>

#include "hinedo-applet.h"

static void
on_panel_applet_callback_preference (BonoboUIComponent *component,
                                     HinedoApplet      *hinedo,
                                     const gchar       *cname)
{
}

static void
on_panel_applet_callback_about (BonoboUIComponent *component,
                                HinedoApplet      *hinedo,
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
                          "logo-icon-name", "hinedo-applet",
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

static gboolean
on_widget_signal_button_press_event (GtkWidget      *widget,
                                     GdkEventButton *event,
                                     HinedoApplet   *hinedo)
{
    return hinedo_applet_popup (hinedo, event);
}

static void
on_panel_applet_signal_change_background (GtkWidget                *widget,
                                          PanelAppletBackgroundType type,
                                          GdkColor                 *color,
                                          GdkPixmap                *pixmap,
                                          gpointer                  user_data)
{
    /* taken from the AlarmClockApplet */
    GtkRcStyle *rc_style;
    GtkStyle *style;

    /* reset style */
    gtk_widget_set_style (widget, NULL);
    rc_style = gtk_rc_style_new ();
    gtk_widget_modify_style (widget, rc_style);
    gtk_rc_style_unref (rc_style);

    switch (type)
    {
    case PANEL_COLOR_BACKGROUND:
        gtk_widget_modify_bg (widget, GTK_STATE_NORMAL, color);
        break;

    case PANEL_PIXMAP_BACKGROUND:
        style = gtk_style_copy (widget->style);
        if (style->bg_pixmap[GTK_STATE_NORMAL])
            g_object_unref (style->bg_pixmap[GTK_STATE_NORMAL]);
        style->bg_pixmap[GTK_STATE_NORMAL] = g_object_ref (pixmap);
        gtk_widget_set_style (widget, style);
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

static gboolean
applet_factory (PanelApplet *applet,
                const gchar *iid,
                gpointer     user_data)
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

    HinedoApplet *hinedo;
    GtkWidget *image;

    if (strcmp (iid, "OAFIID:hinedo-appletApplet") != 0)
        return FALSE;

    gst_init (NULL, NULL);

    hinedo = hinedo_applet_new (applet);

    /* should not expand */
    panel_applet_set_flags (PANEL_APPLET (applet), PANEL_APPLET_EXPAND_MINOR);

    /* applet icon */
    image = gtk_image_new_from_file (PACKAGE_DATA_DIR "/pixmaps/hinedo-applet.png");
    gtk_container_add (GTK_CONTAINER (applet), image);

    /* add preference, about menu */
    panel_applet_setup_menu (applet, xml, verbs, hinedo);

    /* connect signals */
    g_signal_connect (G_OBJECT (applet), "button-press-event",
                      G_CALLBACK (on_widget_signal_button_press_event), hinedo);
    g_signal_connect (G_OBJECT (applet), "change-background",
                      G_CALLBACK (on_panel_applet_signal_change_background), NULL);
    g_signal_connect (G_OBJECT (applet), "change-orient",
                      G_CALLBACK (on_panel_applet_signal_change_orient), applet);
    g_signal_connect (G_OBJECT (applet), "change-size",
                      G_CALLBACK (on_panel_applet_signal_change_size), applet);
    g_signal_connect (G_OBJECT (applet), "move-focus-out-of-applet",
                      G_CALLBACK (on_panel_applet_signal_move_focus_out_of_applet), applet);

    gtk_widget_show_all (GTK_WIDGET (applet));

    return TRUE;
}

PANEL_APPLET_BONOBO_FACTORY ("OAFIID:hinedo-appletApplet_Factory",
                             PANEL_TYPE_APPLET,
                             "Hinedo Applet",
                             "0",
                             (PanelAppletFactoryCallback) applet_factory,
                             NULL);
