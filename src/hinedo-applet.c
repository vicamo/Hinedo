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

enum
{
    PROP_0,
    PROP_APPLET,

    _LAST_PROP
};

G_DEFINE_TYPE (HinedoApplet, hinedo_applet, G_TYPE_OBJECT);

static void
popup_menu_position (GtkMenu   *menu,
                     int       *x,
                     int       *y,
                     gboolean  *push_in,
                     GtkWidget *widget)
{
    GtkRequisition requisition;
    GtkAllocation allocation;
    
    gint menu_xpos;
    gint menu_ypos;

    gtk_widget_size_request (GTK_WIDGET (menu), &requisition);

    gdk_window_get_origin (gtk_widget_get_window (widget),
                           &menu_xpos, &menu_ypos);

    gtk_widget_get_allocation (widget, &allocation);
    menu_xpos += allocation.x;
    menu_ypos += allocation.y;

    switch (panel_applet_get_orient (PANEL_APPLET (widget)))
    {
    case PANEL_APPLET_ORIENT_DOWN:
    case PANEL_APPLET_ORIENT_UP:
        if (menu_ypos > gdk_screen_get_height (gtk_widget_get_screen (widget)) / 2)
            menu_ypos -= requisition.height;
        else
            menu_ypos += allocation.height;
        break;

    case PANEL_APPLET_ORIENT_RIGHT:
    case PANEL_APPLET_ORIENT_LEFT:
        if (menu_xpos > gdk_screen_get_width (gtk_widget_get_screen (widget)) / 2)
            menu_xpos -= requisition.width;
        else
            menu_xpos += allocation.width;
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

static void
hinedo_applet_setup_ui (HinedoApplet *hinedo)
{
    GtkWidget *menu;
    GtkWidget *menu_item;
    GSList *group;

    /* playlist popup menu */
    menu = gtk_menu_new ();
    hinedo->playlist_menu = GTK_MENU (menu);
    g_object_ref (G_OBJECT (menu));

    gtk_widget_show (menu);

    /* the stop menu item */
    menu_item = gtk_radio_menu_item_new_with_mnemonic (NULL, "Stop");
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

    group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (menu_item));
    gtk_widget_show (menu_item);
    g_signal_connect (G_OBJECT (menu_item), "group-changed",
                      G_CALLBACK (on_radio_menu_item_signal_group_changed), hinedo);

    /* separator, not shown at beginning. */
    menu_item = gtk_separator_menu_item_new ();
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
}

static void
hinedo_applet_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
    HinedoApplet *hinedo = HINEDO_APPLET (object);

    switch (property_id)
    {
    case PROP_APPLET:
        hinedo->applet = PANEL_APPLET (g_value_get_object (value));
        g_object_ref (G_OBJECT (hinedo->applet));
        g_signal_connect (G_OBJECT (hinedo->applet), "unrealize",
                          G_CALLBACK (g_object_unref), hinedo);
        break;

    default:
        /* We don't have any other property... */
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
hinedo_applet_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
    HinedoApplet *hinedo = HINEDO_APPLET (object);

    switch (property_id)
    {
    case PROP_APPLET:
        g_value_set_object (value, hinedo_applet_get_panel_applet (hinedo));
        break;

    default:
        /* We don't have any other property... */
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
hinedo_applet_init (HinedoApplet *hinedo)
{
    /* Set up applet */
    hinedo_applet_setup_ui (hinedo);
}

static void
hinedo_applet_finalize (HinedoApplet *hinedo)
{
    g_object_unref (G_OBJECT (hinedo->applet));
    g_object_unref (G_OBJECT (hinedo->playlist_menu));

    G_OBJECT_CLASS (hinedo_applet_parent_class)->finalize (G_OBJECT (hinedo));
}

static void
hinedo_applet_class_init (HinedoAppletClass *klass)
{
    GObjectClass *object_class;
    GParamSpec *pspec;

    object_class = G_OBJECT_CLASS (klass);
    object_class->set_property = hinedo_applet_set_property;
    object_class->get_property = hinedo_applet_get_property;
    object_class->finalize = (GObjectFinalizeFunc) hinedo_applet_finalize;

    pspec = g_param_spec_object (HINEDO_APPLET_PROP_APPLET,
                                 "HinedoApplet construct prop",
                                 "Set underlying panel applet object",
                                 PANEL_TYPE_APPLET,
                                 G_PARAM_CONSTRUCT_ONLY | \
                                 G_PARAM_READWRITE | \
                                 G_PARAM_STATIC_STRINGS);
    g_object_class_install_property (object_class, PROP_APPLET, pspec);
}

HinedoApplet*
hinedo_applet_new (PanelApplet *applet)
{
    g_return_val_if_fail (PANEL_IS_APPLET (applet), NULL);

    GObject *object = g_object_new (HINEDO_TYPE_APPLET, "applet", applet, NULL);
    return HINEDO_APPLET (object);
}

PanelApplet*
hinedo_applet_get_panel_applet (HinedoApplet *hinedo)
{
    g_return_val_if_fail (HINEDO_IS_APPLET (hinedo), NULL);

    return hinedo->applet;
}

gboolean
hinedo_applet_popup (HinedoApplet   *hinedo,
                     GdkEventButton *event)
{
    g_return_val_if_fail (HINEDO_IS_APPLET (hinedo), FALSE);

    /* React only to single left mouse button click */
    if ((event->button == 1)
        && (event->type != GDK_2BUTTON_PRESS)
        && (event->type != GDK_3BUTTON_PRESS))
    {
        gtk_menu_popup (hinedo->playlist_menu, NULL, NULL,
                        (GtkMenuPositionFunc) popup_menu_position,
                        hinedo->applet, event->button, event->time);
        return TRUE;
    }

    return FALSE;
}
