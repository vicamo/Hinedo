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

#include <stdlib.h>
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

static const gchar *selopt_baseurl = \
        "http://hichannel.hinet.net/ajax/house/selectOptions.jsp";
static const gchar *selopt_house = "house_id";
static const gchar *selopt_category = "scat_id";
static const gchar *option_value = "ptionValu";
static const gchar *option_display = "ptionDispla";

static const gchar *hkey = "hinedo_house_id";
static const gchar *ckey = "hinedo_category_id";
static const gchar *mkey = "hinedo_media_id";
static const gchar *ukey = "hinedo_uri";

G_DEFINE_TYPE (HinedoApplet, hinedo_applet, G_TYPE_OBJECT);

static void
hinedo_applet_fix_hichannel_json_string (gchar *haystack)
{
    static const gchar *value = "optionValue";
    static const gchar *display = "optionDisplay";
    const gint vlen = strlen (value);
    const gint dlen = strlen (display);
    gchar *str, *s;

    str = haystack;
    while ((s = strstr (str, value)) != NULL)
    {
        *s = '\''; *(s + vlen - 1) = '\'';
        str += vlen;
    }

    str = haystack;
    while ((s = strstr (str, display)) != NULL)
    {
        *s = '\''; *(s + dlen - 1) = '\'';
        str += dlen;
    }
}

/**
 * @name Error dialogs
 */
/*@{*/

static void
hinedo_applet_show_string (const gchar *message)
{
    GtkWidget *dialog;

    dialog = gtk_message_dialog_new (NULL,
                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                     GTK_MESSAGE_ERROR,
                                     GTK_BUTTONS_CLOSE,
                                     "%s", message);

    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
}

static void
hinedo_applet_show_error (GError *error)
{
    hinedo_applet_show_string (error->message);
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

    if (json_parser_load_from_data (parser, data, length, error))
    {
        JsonNode *root;

        root = json_parser_get_root (parser);
        if (json_node_get_node_type (root) == JSON_NODE_ARRAY)
        {
            JsonArray *array;

            array = json_node_get_array (root);
            json_array_foreach_element (array, func, user_data);

            result = TRUE;
        }
    }

    g_object_unref (G_OBJECT (parser));

    return result;
}

/*@}*/

/**
 * @name Play media streams
 */
/*@{*/

static gboolean
on_gst_bus_watch_callback (GstBus       *bus,
                           GstMessage   *message,
                           HinedoApplet *hinedo)
{
    GError *error;

    switch (GST_MESSAGE_TYPE (message))
    {
    case GST_MESSAGE_EOS:
        gst_element_set_state (hinedo->pipeline, GST_STATE_NULL);
        break;

    case GST_MESSAGE_ERROR:
        gst_message_parse_error (message, &error, NULL);

        hinedo_applet_show_error (error);
        g_error_free (error);

        gst_element_set_state (hinedo->pipeline, GST_STATE_NULL);
        break;

    default:
        break;
    }

    return TRUE;
}

static void
hinedo_applet_play (HinedoApplet *hinedo,
                    const gchar  *uri)
{
    g_object_set (G_OBJECT (hinedo->pipeline), "uri", uri, NULL);
    gst_element_set_state (GST_ELEMENT (hinedo->pipeline), GST_STATE_PLAYING);
}

static void
hinedo_applet_stop (HinedoApplet *hinedo)
{
    gst_element_set_state (hinedo->pipeline, GST_STATE_NULL);
}

/*@}*/

/**
 * @name Media playback prepare
 */
/*@{*/

typedef gboolean (*HinedoPlayFunc) (HinedoApplet *hinedo,
                                    GObject      *menu_item,
                                    gint          house_id,
                                    GError      **error);

typedef struct _HinedoHouse
{
    gint           id;
    gchar         *name;
    HinedoPlayFunc play_func;
} HinedoHouse;

static gboolean
hinedo_applet_play_radio (HinedoApplet *hinedo,
                          GObject      *menu_item,
                          gint          house_id,
                          GError      **error);

static const HinedoHouse houses[] = {
    { 1, "Movie",              NULL},
    { 2, "Theater",            NULL},
    { 4, "Entertainment",      NULL},
    { 5, "Travel",             NULL},
    { 3, "Sport",              NULL},
    { 6, "News",               NULL},
    { 7, "Music",              NULL},
    { 8, "Radio",              hinedo_applet_play_radio},
    {12, "Optical Generation", NULL}
};

static const gchar *radio_murl_format = \
        "http://hichannel.hinet.net/player/radio/index.jsp?radio_id=%d";

static void
on_play_radio_soup_query_callback (HinedoApplet *hinedo,
                                   SoupMessage  *message,
                                   GObject      *menu_item)
{
    SoupBuffer *buffer;
    GRegex *regex;
    GError *error;

    if (message->status_code != SOUP_STATUS_OK)
    {
        hinedo_applet_show_soup_error (message);

        g_object_unref (menu_item);
        return;
    }

    buffer = soup_message_body_flatten (message->response_body);
    error = NULL;

    /* "stream_group.jsp?data=mms://bcr.media.hinet.net/RA000042&id=RADIO:308" */
    regex = g_regex_new ("mms://[^&]+", 0, 0, &error);
    if (!regex)
    {
        hinedo_applet_show_error (error);
        g_error_free (error);
    }
    else
    {
        GMatchInfo *match_info;

        g_regex_match (regex, buffer->data, 0, &match_info);
        if (g_match_info_matches (match_info))
        {
            gchar *uri;

            uri = g_match_info_fetch (match_info, 0);
            hinedo_applet_play (hinedo, uri);

            g_object_set_data_full (menu_item, ukey, uri, g_free);
        }

        g_match_info_free (match_info);
        g_regex_unref (regex);
    }

    soup_buffer_free (buffer);
    g_object_unref (menu_item);
}

static gboolean
hinedo_applet_play_radio (HinedoApplet *hinedo,
                          GObject      *menu_item,
                          gint          house_id,
                          GError      **error)
{
    gchar *uri;

    if ((uri = (gchar*) g_object_get_data (menu_item, ukey)) != NULL)
        hinedo_applet_play (hinedo, uri);
    else
    {
        gint media_id;

        media_id = GPOINTER_TO_INT (g_object_get_data (menu_item, mkey));
        uri = g_strdup_printf (radio_murl_format, media_id);

        hinedo_applet_soup_query (
                hinedo, uri,
                (HinedoSoupCallback)on_play_radio_soup_query_callback,
                g_object_ref (menu_item));

        g_free (uri);
    }

    return TRUE;
}

static void
on_radio_menu_item_signal_toggled (GObject      *menu_item,
                                   HinedoApplet *hinedo)
{
    GError *error;
    gint house_id;
    gint ii;

    if (!gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menu_item)))
        return;

    hinedo_applet_stop (hinedo);

    house_id = GPOINTER_TO_INT (g_object_get_data (menu_item, hkey));
    error = NULL;

    for (ii = 0; ii < G_N_ELEMENTS (houses); ++ii)
    {
        if ((houses[ii].id == house_id) && houses[ii].play_func)
        {
            if (!houses[ii].play_func (hinedo, menu_item, house_id, &error))
            {
                hinedo_applet_show_error (error);
                g_error_free (error);
            }

            break;
        }
    }
}

/*@}*/

/**
 * @name Category menu update
 */
/*@{*/

typedef struct _HinedoJsonCallbackData
{
    HinedoApplet    *hinedo;
    GtkMenu         *menu;
    JsonArrayForeach foreach;
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
    g_object_set_data (G_OBJECT (menu_item), hkey,
                       g_object_get_data (G_OBJECT (data->menu), hkey));
    g_object_set_data (G_OBJECT (menu_item), ckey,
                       g_object_get_data (G_OBJECT (data->menu), ckey));
    g_object_set_data (G_OBJECT (menu_item), mkey,
                       GINT_TO_POINTER (atoi (json_node_get_string (id_node))));

    /* automatic query items */
    g_signal_connect (G_OBJECT (menu_item), "toggled",
                      G_CALLBACK (on_radio_menu_item_signal_toggled),
                      data->hinedo);

    gtk_widget_show (menu_item);
}

static void
on_menu_soup_query_callback (HinedoApplet           *hinedo,
                             SoupMessage            *message,
                             HinedoJsonCallbackData *data)
{
    GList *list;

    /* remove the retrieving item */
    list = gtk_container_get_children (GTK_CONTAINER (data->menu));
    gtk_container_remove (GTK_CONTAINER (data->menu), GTK_WIDGET (list->data));
    g_list_free (list);

    if (message->status_code != SOUP_STATUS_OK)
    {
        hinedo_applet_show_soup_error (message);
    }
    else
    {
        SoupBuffer *buffer;
        gchar *str;
        GError *error;

        buffer = soup_message_body_flatten (message->response_body);
        str = strdup (buffer->data);
        error = NULL;

        hinedo_applet_fix_hichannel_json_string (str);
        if (!hinedo_applet_json_foreach (str, strlen (str),
                                         data->foreach, data, &error))
        {
            hinedo_applet_show_error (error);
        }

        g_free (str);
        soup_buffer_free (buffer);
    }

    list = gtk_container_get_children (GTK_CONTAINER (data->menu));
    if (!list)
    {
        GtkWidget *menu_item;

        /* append a item to show no contents available */
        menu_item = gtk_menu_item_new_with_label ("(None)");
        gtk_widget_show (menu_item);
        gtk_menu_shell_append (GTK_MENU_SHELL (data->menu), menu_item);
    }
    else
    {
        g_list_free (list);
    }

    g_object_unref (G_OBJECT (data->menu));
    g_slice_free (HinedoJsonCallbackData, data);
}

static void
on_category_menu_signal_show (GtkMenu      *menu,
                              HinedoApplet *hinedo)
{
    GtkWidget *menu_item;
    gchar *url;
    gpointer house_data;
    gpointer category_data;
    HinedoJsonCallbackData *data;

    /* disconnect to prevent from being re-updated. */
    g_object_disconnect (G_OBJECT (menu), "any_signal::show",
                         G_CALLBACK (on_category_menu_signal_show), hinedo, NULL);

    /* add a temp item to show it's at retrieving. */
    menu_item = gtk_menu_item_new_with_label ("Retrieving items ...");
    gtk_widget_show (menu_item);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

    /* query hinet jsp */
    house_data = g_object_get_data (G_OBJECT (menu), hkey);
    category_data = g_object_get_data (G_OBJECT (menu), ckey);
    url = g_strdup_printf ("%s?%s=%d&%s=%d", selopt_baseurl,
                           selopt_house, GPOINTER_TO_INT (house_data),
                           selopt_category, GPOINTER_TO_INT (category_data));

    data = g_slice_new0 (HinedoJsonCallbackData);
    data->hinedo = hinedo;
    data->menu = (GtkMenu*) g_object_ref (G_OBJECT (menu));
    data->foreach = (JsonArrayForeach) on_category_menu_json_foreach_callback;

    hinedo_applet_soup_query (
            hinedo, url,
            (HinedoSoupCallback) on_menu_soup_query_callback, data);

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
    g_object_set_data (G_OBJECT (menu), hkey,
                       g_object_get_data (G_OBJECT (data->menu), hkey));
    g_object_set_data (G_OBJECT (menu), ckey,
                       GINT_TO_POINTER (atoi (json_node_get_string (id_node))));

    /* automatic query items */
    g_signal_connect (G_OBJECT (menu), "show",
                      G_CALLBACK (on_category_menu_signal_show), data->hinedo);

    gtk_widget_show_all (menu_item);
}

static void
on_house_menu_signal_show (GtkMenu      *menu,
                           HinedoApplet *hinedo)
{
    GtkWidget *menu_item;
    gchar *url;
    gpointer house_data;
    HinedoJsonCallbackData *data;

    /* disconnect to prevent from being re-updated. */
    g_object_disconnect (G_OBJECT (menu), "any_signal::show",
                         G_CALLBACK (on_house_menu_signal_show), hinedo, NULL);

    /* add a temp item to show it's at retrieving. */
    menu_item = gtk_menu_item_new_with_label ("Retrieving categories ...");
    gtk_widget_show (menu_item);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

    /* query hinet jsp */
    house_data = g_object_get_data (G_OBJECT (menu), hkey);
    url = g_strdup_printf ("%s?%s=%d", selopt_baseurl,
                           selopt_house, GPOINTER_TO_INT (house_data));

    data = g_slice_new0 (HinedoJsonCallbackData);
    data->hinedo = hinedo;
    data->menu = (GtkMenu*) g_object_ref (G_OBJECT (menu));
    data->foreach = (JsonArrayForeach) on_house_menu_json_foreach_callback;

    hinedo_applet_soup_query (
            hinedo, url,
            (HinedoSoupCallback) on_menu_soup_query_callback, data);

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

    /* the stop menu item */
    menu_item = gtk_radio_menu_item_new_with_mnemonic (NULL, "Stop");
    gtk_menu_shell_append (GTK_MENU_SHELL (hinedo->playlist_menu), menu_item);

    hinedo->playlist_group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (menu_item));
    g_signal_connect (G_OBJECT (menu_item), "toggled",
                      G_CALLBACK (on_radio_menu_item_signal_toggled),
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
                           hkey, GINT_TO_POINTER (houses[ii].id));

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
        hinedo->applet = PANEL_APPLET (g_value_dup_object (value));
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
    GstBus *bus;

    /* Set up applet */
    hinedo_applet_setup_ui (hinedo);

    /* soup session */
    hinedo->session = soup_session_async_new ();
#ifdef HAVE_LIBSOUP_GNOME
    soup_session_add_feature_by_type (hinedo->session,
                                      SOUP_TYPE_PROXY_RESOLVER_GNOME);
#endif

    /* GStreamer */
    hinedo->pipeline = gst_element_factory_make ("playbin", "player");

    bus = gst_pipeline_get_bus (GST_PIPELINE (hinedo->pipeline));
    hinedo->watch = gst_bus_add_watch (
            bus, (GstBusFunc) on_gst_bus_watch_callback, hinedo);
    gst_object_unref (bus);
}

static void
hinedo_applet_finalize (HinedoApplet *hinedo)
{
    gst_object_unref (GST_OBJECT (hinedo->pipeline));
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
        break;
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
