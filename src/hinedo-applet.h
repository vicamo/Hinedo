/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * hinedo-applet.h
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

#ifndef __HINEDO_APPLET_H__
#define __HINEDO_APPLET_H__

#include <gtk/gtk.h>
#include <panel-applet.h>

G_BEGIN_DECLS

#define HINEDO_TYPE_APPLET            (hinedo_applet_get_type ())
#define HINEDO_APPLET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), HINEDO_TYPE_APPLET, HinedoApplet))
#define HINEDO_APPLET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), HINEDO_TYPE_APPLET, HinedoAppletClass))
#define HINEDO_IS_APPLET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HINEDO_TYPE_APPLET))
#define HINEDO_IS_APPLET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), HINEDO_TYPE_APPLET))
#define HINEDO_APPLET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), HINEDO_TYPE_APPLET, HinedoAppletClass))

typedef struct _HinedoApplet      HinedoApplet;
typedef struct _HinedoAppletClass HinedoAppletClass;

struct _HinedoApplet
{
    PanelApplet parent;
};

struct _HinedoAppletClass
{
    PanelAppletClass parent_class;
};

GType hinedo_applet_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* end __HINEDO_APPLET_H__ */
