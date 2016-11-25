/* chips-application.c
 *
 * Copyright (C) 2016 Ray Strode <rstrode@redhat.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "chips-application.h"

struct _ChipsApplication
{
        GtkApplication parent_object;
        GtkWidget *main_window;
};

G_DEFINE_TYPE (ChipsApplication, chips_application, GTK_TYPE_APPLICATION);

static void
chips_application_dispose (GObject *object)
{
        ChipsApplication *self = CHIPS_APPLICATION (object);

        G_OBJECT_CLASS (chips_application_parent_class)->dispose (object);
}

static void
chips_application_finalize (GObject *object)
{
        ChipsApplication *self = CHIPS_APPLICATION (object);

        G_OBJECT_CLASS (chips_application_parent_class)->finalize (object);
}

static void
chips_application_startup (GApplication *application)
{
        ChipsApplication *self = CHIPS_APPLICATION (application);

        G_APPLICATION_CLASS (chips_application_parent_class)->startup (application);
}

static void
activate_main_window (ChipsApplication *self)
{
        if (self->main_window != NULL) {
                gtk_window_present (GTK_WINDOW (self->main_window));
                return;
        }

        self->main_window = g_object_new (GTK_TYPE_WINDOW, NULL);
        gtk_application_add_window (GTK_APPLICATION (self), GTK_WINDOW (self->main_window));
        gtk_widget_show (self->main_window);
}

static void
chips_application_activate (GApplication *application)
{
        ChipsApplication *self = CHIPS_APPLICATION (application);

        activate_main_window (self);

        G_APPLICATION_CLASS (chips_application_parent_class)->activate (application);
}

static void
chips_application_class_init (ChipsApplicationClass *own_class)
{
        GApplicationClass *application_class = G_APPLICATION_CLASS (own_class);
        GObjectClass *object_class = G_OBJECT_CLASS (own_class);

        object_class->dispose = chips_application_dispose;
        object_class->finalize = chips_application_finalize;

        application_class->activate = chips_application_activate;
        application_class->startup = chips_application_startup;
}

static void
chips_application_init (ChipsApplication *self)
{
}
