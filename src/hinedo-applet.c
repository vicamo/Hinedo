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
#  include <config.h>
#endif

#include <string.h>

#include <gtk/gtk.h>
#include <panel-applet.h>

static gboolean
applet_factory (PanelApplet *applet, const gchar *iid, gpointer user_data)
{
	GtkWidget *label;

	if (strcmp (iid, "OAFIID:hinedo-appletApplet") != 0)
		return FALSE;

	label = gtk_label_new ("Hello World");
	gtk_container_add (GTK_CONTAINER (applet), label);

	gtk_widget_show_all (GTK_WIDGET (applet));

	return TRUE;
}

PANEL_APPLET_BONOBO_FACTORY ("OAFIID:hinedo-appletApplet_Factory",
                             PANEL_TYPE_APPLET,
                             "Hinedo Applet",
                             "0",
                             applet_factory,
                             NULL);

