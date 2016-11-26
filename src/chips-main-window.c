/* chips-main-window.c
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
#include "chips-main-window.h"

struct _ChipsMainWindow
{
        GtkWindow parent_object;

        GtkWidget *gl_area;

        unsigned int vertex_array_id;

        unsigned int vertex_buffer_id;

        unsigned int shader_program_id;
        unsigned int vertex_shader_id;
        unsigned int fragment_shader_id;

        unsigned int position_attribute_id;
};

G_DEFINE_TYPE (ChipsMainWindow, chips_main_window, GTK_TYPE_WINDOW);

typedef struct ChipsVertexBuffer ChipsVertexBuffer;

__attribute__ ((packed))
struct ChipsVertexBuffer
{
        float x;
        float y;
        float z;
};

ChipsVertexBuffer test_vertices[] =
{
        { .x =  0.0, .y =  0.5 },
        { .x =  0.5, .y = -0.5 },
        { .x = -0.5, .y = -0.5 }
};

typedef enum
{
        CHIPS_VERTEX_SHADER = GL_VERTEX_SHADER,
        CHIPS_FRAGMENT_SHADER = GL_FRAGMENT_SHADER
} ChipsShaderType;

static const char *vertex_shader =
"#version 330\n"
"in vec2 position;\n"
"void\n"
"main ()\n"
"{\n"
"        gl_Position = vec4 (position, 0.0, 1.0);\n"
"}\n";

static const char *fragment_shader =
"#version 330\n"
"out vec4 fragment_color;\n"
"void main ()\n"
"{\n"
"        fragment_color = vec4 (1.0, 1.0, 1.0, 1.0);\n"
"}\n";

static void
chips_main_window_dispose (GObject *object)
{
        ChipsMainWindow *self = CHIPS_MAIN_WINDOW (object);

        G_OBJECT_CLASS (chips_main_window_parent_class)->dispose (object);
}

static void
chips_main_window_finalize (GObject *object)
{
        ChipsMainWindow *self = CHIPS_MAIN_WINDOW (object);

        G_OBJECT_CLASS (chips_main_window_parent_class)->finalize (object);
}

static void
chips_main_window_class_init (ChipsMainWindowClass *own_class)
{
        GObjectClass *object_class = G_OBJECT_CLASS (own_class);

        object_class->dispose = chips_main_window_dispose;
        object_class->finalize = chips_main_window_finalize;
}

static gboolean
initialize_gl (ChipsMainWindow  *self,
               GError          **error)
{
        const GError *gl_error;

        gtk_gl_area_make_current (GTK_GL_AREA (self->gl_area));

        gl_error = gtk_gl_area_get_error (GTK_GL_AREA (self->gl_area));

        if (gl_error != NULL) {
                g_propagate_error (error, g_error_copy (gl_error));
                return FALSE;
        }

        return TRUE;
}

static gboolean
load_shader (ChipsMainWindow *self,
             ChipsShaderType  shader_type,
             const char      *shader,
             unsigned int    *shader_id)
{
        int compile_status;

        *shader_id = glCreateShader (shader_type);
        glShaderSource (*shader_id, 1, &shader, NULL);
        glCompileShader (*shader_id);

        glGetShaderiv (*shader_id, GL_COMPILE_STATUS, &compile_status);

        if (!compile_status) {
                char compile_log[4096];

                glGetShaderInfoLog (*shader_id, sizeof (compile_log), NULL, compile_log);
                g_warning ("failed to compile shader: '%s'\n%s",
                           shader, compile_log);
        }

        return compile_status;
}

static void
load_vertices (ChipsMainWindow *self)
{
        glGenVertexArrays(1, &self->vertex_array_id);
        glBindVertexArray(self->vertex_array_id);

        glGenBuffers (1, &self->vertex_buffer_id);
        glBindBuffer (GL_ARRAY_BUFFER, self->vertex_buffer_id);
        glBufferData (GL_ARRAY_BUFFER,
                      sizeof (test_vertices),
                      (float *) &test_vertices,
                      GL_STATIC_DRAW);

}

static void
load_shaders (ChipsMainWindow *self)
{
        load_shader (self,
                     CHIPS_VERTEX_SHADER,
                     vertex_shader,
                     &self->vertex_shader_id);
        load_shader (self,
                     CHIPS_FRAGMENT_SHADER,
                     fragment_shader,
                     &self->fragment_shader_id);

        self->shader_program_id = glCreateProgram ();
        glAttachShader (self->shader_program_id,
                        self->vertex_shader_id);
        glAttachShader (self->shader_program_id,
                        self->fragment_shader_id);

        glBindFragDataLocation (self->shader_program_id,
                                0,
                                "fragment_color");
        glLinkProgram (self->shader_program_id);
        glUseProgram (self->shader_program_id);

        self->position_attribute_id = glGetAttribLocation (self->shader_program_id, "position");

        glEnableVertexAttribArray (self->position_attribute_id);
        glVertexAttribPointer (self->position_attribute_id,
                               3,
                               GL_FLOAT,
                               GL_FALSE,
                               0, 0);
}

static void
on_gl_area_realized (ChipsMainWindow *self)
{
        g_autoptr (GError) error = NULL;

        if (!initialize_gl (self, &error)) {
                g_error ("%s", error->message);
        }

        load_vertices (self);
        load_shaders (self);
}

static void
on_gl_area_unrealized (ChipsMainWindow *self)
{
        g_autoptr (GError) error = NULL;

        if (!initialize_gl (self, &error)) {
                g_warning ("%s", error->message);
                return;
        }
}

static gboolean
on_gl_area_render (ChipsMainWindow *self)
{
        glClearColor (0.5, 0.5, 0.5, 1.0);
        glClear (GL_COLOR_BUFFER_BIT);

        glBindBuffer (GL_ARRAY_BUFFER, self->vertex_buffer_id);
        glEnableVertexAttribArray (self->position_attribute_id);

        glDrawArrays (GL_TRIANGLES, 0, G_N_ELEMENTS (test_vertices));

        glDisableVertexAttribArray (self->position_attribute_id);
        glBindBuffer (GL_ARRAY_BUFFER, 0);

        return TRUE;
}

static void
chips_main_window_init (ChipsMainWindow *self)
{
        gtk_window_set_title (GTK_WINDOW (self), _("Chips"));
        gtk_window_set_default_size (GTK_WINDOW (self), 800, 600);

        self->gl_area = g_object_new (GTK_TYPE_GL_AREA, NULL);

        g_signal_connect_swapped (self->gl_area,
                                  "realize",
                                  G_CALLBACK (on_gl_area_realized),
                                  self);

        g_signal_connect_swapped (self->gl_area,
                                  "unrealize",
                                  G_CALLBACK (on_gl_area_unrealized),
                                  self);

        g_signal_connect_swapped (self->gl_area,
                                  "render",
                                  G_CALLBACK (on_gl_area_render),
                                  self);

        gtk_container_add (GTK_CONTAINER (self), self->gl_area);

        gtk_widget_show (self->gl_area);
}
