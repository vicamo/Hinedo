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
#include <libsoup/soup.h>
#include <json-glib/json-glib.h>

#ifdef HAVE_LIBSOUP_GNOME
#  include <libsoup/soup-gnome.h>
#endif

#include "hinedo-applet.h"

enum
{
    PROP_0,
    PROP_APPLET,

    _LAST_PROP
};

typedef struct _HinedoHouse
{
    gint id;
    gchar *name;
} HinedoHouse;

static const gchar *selopt_baseurl = \
        "http://hichannel.hinet.net/ajax/house/selectOptions.jsp";
static const gchar *selopt_house = "house_id";
static const gchar *selopt_category = "scat_id";
static const gchar *selopt_item = "item_id";
static const gchar *option_value = "optionValue";
static const gchar *option_display = "optionDisplay";
static const HinedoHouse houses[] = {
	{ 1, "Movie"},
	{ 2, "Theater"},
	{ 4, "Entertainment"},
	{ 5, "Travel"},
	{ 3, "Sport"},
	{ 6, "News"},
	{ 7, "Music"},
    { 8, "Radio"},
	{12, "Optical Generation"}
};

G_DEFINE_TYPE (HinedoApplet, hinedo_applet, G_TYPE_OBJECT);

/**
 * @name Error dialogs
 */
/*@{*/

static void
hinedo_applet_show_error (GError *error)
{
    GtkWidget *dialog;

    dialog = gtk_message_dialog_new (NULL,
                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                     GTK_MESSAGE_ERROR,
                                     GTK_BUTTONS_CLOSE,
                                     "%s", error->message);

    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
}

static void
hinedo_applet_show_soup_error (SoupMessage *message)
{
    GtkWidget *dialog;
    gchar *urlstr;

    urlstr = soup_uri_to_string (soup_message_get_uri (message), FALSE);
    dialog = gtk_message_dialog_new (NULL,
                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                     GTK_MESSAGE_ERROR,
                                     GTK_BUTTONS_CLOSE,
                                     "Error update playlist from '%s', %s",
                                     urlstr, message->reason_phrase);

    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);

    g_free (urlstr);
}

/*@}*/

/**
 * @name Soup related functions
 */
/*@{*/

typedef void (*HinedoSoupCallback) (HinedoApplet *hinedo,
                                    SoupMessage  *message,
                                    gpointer      user_data);

typedef struct _HinedoSoupCallbackData
{
    SoupMessage       *message;
    GSource           *timeout_source;

    HinedoApplet      *hinedo;
    HinedoSoupCallback callback;
    gpointer           user_data;
} HinedoSoupCallbackData;

static void
on_soup_query_callback_complete (SoupSession            *session,
                                 SoupMessage            *message,
                                 HinedoSoupCallbackData *data)
{
    /* remove timeout source */
    if (data->timeout_source)
    {
        g_source_destroy (data->timeout_source);
        data->timeout_source = NULL;
    }

    data->callback (data->hinedo, message, data->user_data);

    g_object_unref (G_OBJECT (data->message));
    g_slice_free (HinedoSoupCallbackData, data);
}

static gboolean
on_soup_query_callback_timeout (HinedoSoupCallbackData *data)
{
    soup_session_cancel_message (data->hinedo->session,
                                 data->message,
                                 SOUP_STATUS_CANCELLED);

    /* free this timeout source */
    data->timeout_source = NULL;
    return FALSE;
}

static void
hinedo_applet_soup_query (HinedoApplet      *hinedo,
                          const gchar       *url,
                          HinedoSoupCallback callback,
                          gpointer           user_data)
{
    HinedoSoupCallbackData *data;

    data = g_slice_new (HinedoSoupCallbackData);
    data->hinedo = hinedo;
    data->callback = callback;
    data->user_data = user_data;

    data->message = soup_message_new (SOUP_METHOD_GET, url);
    g_object_ref (G_OBJECT (data->message));

    soup_session_queue_message (
            hinedo->session, data->message,
            (SoupSessionCallback) on_soup_query_callback_complete, data);

    data->timeout_source = soup_add_timeout (
            NULL, 30000, (GSourceFunc) on_soup_query_callback_timeout, data);
}

/*@}*/

/**
 * @name JSON related functions
 */
/*@{*/

static gboolean
hinedo_applet_json_foreach (const gchar     *data,
                            gsize            length,
                            JsonArrayForeach func,
                            gpointer         user_data,
                            GError         **error)
{
    JsonParser *parser;
    gboolean result;

    parser = json_parser_new ();
    result = FALSE;

    if (*error)
        *error = NULL;

    if (json_parser_load_from_data (parser, data, length, error))
    {
        JsonNode *root;

        root = json_parser_get_root (parser);
        if (json_node_get_node_type (root) == JSON_NODE_ARRAY)
        {
            JsonArray *array;

            array = json_node_dup_array (root);
            json_array_foreach_element (array, func, user_data);
            g_object_unref (G_OBJECT (array));

            result = TRUE;
        }
    }

    g_object_unref (G_OBJECT (parser));

    return result;
}

/*@}*/

static void
on_radio_menu_item_signal_group_changed (GtkRadioMenuItem *menu_item,
                                         HinedoApplet     *applet)
{
}

/**
 * @name Category menu update
 */
/*@{*/

typedef struct _HinedoJsonCallbackData
{
    HinedoApplet *hinedo;
    GtkMenu      *menu;
} HinedoJsonCallbackData;

static void
on_category_menu_json_foreach_callback (JsonArray              *array,
                                        guint                   index,
                                        JsonNode               *node,
                                        HinedoJsonCallbackData *data)
{
    JsonObject *object;
    JsonNode *id_node;
    JsonNode *name_node;
    GtkWidget *menu_item;
    GSList *group;

    object = json_node_get_object (node);

    if (((id_node = json_object_get_member (object, option_value)) == NULL) ||
        ((name_node = json_object_get_member (object, option_display)) == NULL))
	{
        return;
    }

    group = data->hinedo->playlist_group;
    menu_item = gtk_radio_menu_item_new_with_label (
            group, json_node_get_string (name_node));
    gtk_menu_shell_append (GTK_MENU_SHELL (data->menu), menu_item);

    /* update playlist radio group */
    group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (menu_item));
    data->hinedo->playlist_group = group;

    /* remember house & category & item id */
    g_object_set_data (G_OBJECT (menu_item),
                       selopt_house,
                       g_object_get_data (G_OBJECT (data->menu),
                                          selopt_house));
    g_object_set_data (G_OBJECT (menu_item),
                       selopt_category,
                       g_object_get_data (G_OBJECT (data->menu),
                                          selopt_category));
    g_object_set_data (G_OBJECT (menu_item),
                       selopt_item,
                       GINT_TO_POINTER (json_node_get_int (id_node)));

    /* automatic query items */
    g_signal_connect (G_OBJECT (menu_item), "group-changed",
                      G_CALLBACK (on_radio_menu_item_signal_group_changed),
                      data->hinedo);

    gtk_widget_show (menu_item);
}

static void
on_category_menu_soup_query_callback (HinedoApplet *hinedo,
                                      SoupMessage  *message,
                                      GtkMenu      *menu)
{
    static const gchar *test_scat1 = "[ {'optionValue':'1001','optionDisplay':'Bravo FM913 台北都會休閒音樂台'}, {'optionValue':'308','optionDisplay':'KISS RADIO 網路音樂台'}, {'optionValue':'156','optionDisplay':'KISS RADIO 大眾廣播電台'}, {'optionValue':'255','optionDisplay':'KISS RADIO 台南知音廣播'}, {'optionValue':'256','optionDisplay':'KISS RADIO 大苗栗廣播'}, {'optionValue':'258','optionDisplay':'KISS RADIO 南投廣播'}, {'optionValue':'206','optionDisplay':'中廣音樂網i radio'}, {'optionValue':'205','optionDisplay':'中廣流行網 i like'}, {'optionValue':'222','optionDisplay':'HitFM聯播網 北部'}, {'optionValue':'88','optionDisplay':'HitFM聯播網 中部'}, {'optionValue':'90','optionDisplay':'HitFM聯播網 南部'}, {'optionValue':'370','optionDisplay':'POP Radio 917台北流行音樂電台'}, {'optionValue':'228','optionDisplay':'台北愛樂'}, {'optionValue':'294','optionDisplay':'奇美古典音樂網'}, {'optionValue':'212','optionDisplay':'BestRadio 台北好事989'}, {'optionValue':'213','optionDisplay':'BestRadio 高雄港都983'}, {'optionValue':'211','optionDisplay':'BestRadio 台中好事903'}, {'optionValue':'303','optionDisplay':'BestRadio 花蓮好事935'}, {'optionValue':'248','optionDisplay':'Apple line 蘋果線上'}, {'optionValue':'321','optionDisplay':'ASIAFM衛星音樂台'}, {'optionValue':'357','optionDisplay':'Flyradio飛揚調頻895'}, {'optionValue':'340','optionDisplay':'佳音聖樂網 CCM'}, {'optionValue':'338','optionDisplay':'全國廣播音樂網'}, {'optionValue':'313','optionDisplay':'台灣之音-音樂'}, {'optionValue':'289','optionDisplay':'太陽電台'} ] ";
    GList *list;

    /* remove the retrieving item */
    list = gtk_container_get_children (GTK_CONTAINER (menu));
    gtk_container_remove (GTK_CONTAINER (menu), GTK_WIDGET (list->data));
    g_list_free (list);

    if (message->status_code != SOUP_STATUS_OK)
    {
        hinedo_applet_show_soup_error (message);
    }
    else
	{
        HinedoJsonCallbackData data;
        SoupBuffer *buffer;
        GError *error;

        buffer = soup_message_body_flatten (message->response_body);
        data.hinedo = hinedo;
        data.menu = menu;
        error = NULL;

        if (!hinedo_applet_json_foreach (
#if 0
                buffer->data, buffer->length,
#else
                test_scat1, strlen (test_scat1),
#endif
                (JsonArrayForeach) on_category_menu_json_foreach_callback,
                &data, &error))
        {
            hinedo_applet_show_error (error);
        }

        soup_buffer_free (buffer);
	}

    list = gtk_container_get_children (GTK_CONTAINER (menu));
    if (!list)
	{
        GtkWidget *menu_item;

        /* append a item to show no contents available */
        menu_item = gtk_menu_item_new_with_label ("(None)");
        gtk_widget_show (menu_item);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
    }
    else
	{
        g_list_free (list);
	}
}

static void
on_category_menu_signal_show (GtkMenu      *menu,
                              HinedoApplet *hinedo)
{
    GtkWidget *menu_item;
    gchar *url;
    gpointer house_data;
    gpointer category_data;

    /* disconnect to prevent from being re-updated. */
    g_object_disconnect (G_OBJECT (menu), "any_signal::show",
                         G_CALLBACK (on_category_menu_signal_show), hinedo, NULL);

    /* add a temp item to show it's at retrieving. */
    menu_item = gtk_menu_item_new_with_label ("Retrieving items ...");
    gtk_widget_show (menu_item);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

    /* query hinet jsp */
    house_data = g_object_get_data (G_OBJECT (menu), selopt_house);
    category_data = g_object_get_data (G_OBJECT (menu), selopt_category);
    url = g_strdup_printf ("%s?%s=%d&%s=%d", selopt_baseurl,
                           selopt_house, GPOINTER_TO_INT (house_data),
                           selopt_category, GPOINTER_TO_INT (category_data));

    hinedo_applet_soup_query (
            hinedo, url,
            (HinedoSoupCallback) on_category_menu_soup_query_callback, menu);

    g_free (url);
}

/*@}*/

/**
 * @name House menu update
 */
/*@{*/

static void
on_house_menu_json_foreach_callback (JsonArray              *array,
                                     guint                   index,
                                     JsonNode               *node,
                                     HinedoJsonCallbackData *data)
{
    JsonObject *object;
    JsonNode *id_node;
    JsonNode *name_node;
    GtkWidget *menu_item;
    GtkWidget *menu;

    object = json_node_get_object (node);

    if (((id_node = json_object_get_member (object, option_value)) == NULL) ||
        ((name_node = json_object_get_member (object, option_display)) == NULL))
	{
        return;
    }

    menu_item = gtk_menu_item_new_with_label (json_node_get_string (name_node));
    gtk_menu_shell_append (GTK_MENU_SHELL (data->menu), menu_item);

    menu = gtk_menu_new ();
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), menu);

    /* remember house & category id */
    g_object_set_data (G_OBJECT (menu),
                       selopt_house,
                       g_object_get_data (G_OBJECT (data->menu),
                                          selopt_house));
    g_object_set_data (G_OBJECT (menu),
                       selopt_category,
                       GINT_TO_POINTER (json_node_get_int (id_node)));

    /* automatic query items */
    g_signal_connect (G_OBJECT (menu), "show",
                      G_CALLBACK (on_category_menu_signal_show), data->hinedo);

    gtk_widget_show_all (menu_item);
}

static void
on_house_menu_soup_query_callback (HinedoApplet *hinedo,
                                   SoupMessage  *message,
                                   GtkMenu      *menu)
{
    static const gchar *test_house8 = "[ {'optionValue':'1','optionDisplay':'音樂'}, {'optionValue':'4','optionDisplay':'生活'}, {'optionValue':'0','optionDisplay':'新聞'}, {'optionValue':'7','optionDisplay':'綜合'}, {'optionValue':'3','optionDisplay':'外語'}, {'optionValue':'5','optionDisplay':'多元文化'}, {'optionValue':'6','optionDisplay':'交通'} ]";
    GList *list;

    /* remove the retrieving item */
    list = gtk_container_get_children (GTK_CONTAINER (menu));
    gtk_container_remove (GTK_CONTAINER (menu), GTK_WIDGET (list->data));
    g_list_free (list);

    if (message->status_code != SOUP_STATUS_OK)
    {
        hinedo_applet_show_soup_error (message);
    }
    else
	{
        HinedoJsonCallbackData data;
        SoupBuffer *buffer;
        GError *error;

        buffer = soup_message_body_flatten (message->response_body);
        data.hinedo = hinedo;
        data.menu = menu;
        error = NULL;

        if (!hinedo_applet_json_foreach (
#if 0
                buffer->data, buffer->length,
#else
                test_house8, strlen (test_house8),
#endif
                (JsonArrayForeach) on_house_menu_json_foreach_callback,
                &data, &error))
        {
            hinedo_applet_show_error (error);
        }

        soup_buffer_free (buffer);
	}

    list = gtk_container_get_children (GTK_CONTAINER (menu));
    if (!list)
	{
        GtkWidget *menu_item;

        /* append a item to show no contents available */
        menu_item = gtk_menu_item_new_with_label ("(None)");
        gtk_widget_show (menu_item);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
    }
    else
	{
        g_list_free (list);
	}
}

static void
on_house_menu_signal_show (GtkMenu      *menu,
                           HinedoApplet *hinedo)
{
    GtkWidget *menu_item;
    gchar *url;
    gpointer data;

    /* disconnect to prevent from being re-updated. */
    g_object_disconnect (G_OBJECT (menu), "any_signal::show",
                         G_CALLBACK (on_house_menu_signal_show), hinedo, NULL);

    /* add a temp item to show it's at retrieving. */
    menu_item = gtk_menu_item_new_with_label ("Retrieving categories ...");
    gtk_widget_show (menu_item);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

    /* query hinet jsp */
    data = g_object_get_data (G_OBJECT (menu), selopt_house);
    url = g_strdup_printf ("%s?%s=%d", selopt_baseurl,
                           selopt_house, GPOINTER_TO_INT (data));

    hinedo_applet_soup_query (
            hinedo, url,
            (HinedoSoupCallback) on_house_menu_soup_query_callback, menu);

    g_free (url);
}

/*@}*/

static void
hinedo_applet_setup_ui (HinedoApplet *hinedo)
{
    GtkWidget *menu;
    GtkWidget *menu_item;
    gint ii;

    /* playlist popup menu */
    hinedo->playlist_menu = GTK_MENU (gtk_menu_new ());
    g_object_ref (G_OBJECT (hinedo->playlist_menu));

    /* the stop menu item */
    menu_item = gtk_radio_menu_item_new_with_mnemonic (NULL, "Stop");
    gtk_menu_shell_append (GTK_MENU_SHELL (hinedo->playlist_menu), menu_item);

    hinedo->playlist_group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (menu_item));
    g_signal_connect (G_OBJECT (menu_item), "group-changed",
                      G_CALLBACK (on_radio_menu_item_signal_group_changed),
                      hinedo);

    /* separator */
    menu_item = gtk_separator_menu_item_new ();
    gtk_menu_shell_append (GTK_MENU_SHELL (hinedo->playlist_menu), menu_item);

    /* per house settings */
    for (ii = 0; ii < G_N_ELEMENTS (houses); ++ii)
    {
        menu_item = gtk_menu_item_new_with_label (houses[ii].name);
        gtk_menu_shell_append (GTK_MENU_SHELL (hinedo->playlist_menu), menu_item);

        menu = gtk_menu_new ();
        gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), menu);

        /* remember house id */
        g_object_set_data (G_OBJECT (menu),
                           selopt_house, GINT_TO_POINTER (houses[ii].id));

        /* automatic query house categories */
        g_signal_connect (G_OBJECT (menu), "show",
                          G_CALLBACK (on_house_menu_signal_show), hinedo);
    }

    gtk_widget_show_all (GTK_WIDGET (hinedo->playlist_menu));
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

    /* soup session */
    hinedo->session = soup_session_async_new ();
    g_object_ref (G_OBJECT (hinedo->session));
#ifdef HAVE_LIBSOUP_GNOME
    soup_session_add_feature_by_type (hinedo->session,
                                      SOUP_TYPE_PROXY_RESOLVER_GNOME);
#endif
}

static void
hinedo_applet_finalize (HinedoApplet *hinedo)
{
    g_object_unref (G_OBJECT (hinedo->session));
    g_object_unref (G_OBJECT (hinedo->playlist_menu));
    g_object_unref (G_OBJECT (hinedo->applet));

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

/**
 * @name Popup
 */
/*@{*/

static void
on_popup_callback_menu_position (GtkMenu   *menu,
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
                        (GtkMenuPositionFunc) on_popup_callback_menu_position,
                        hinedo->applet, event->button, event->time);
        return TRUE;
    }

    return FALSE;
}

/*@}*/
