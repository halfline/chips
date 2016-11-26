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
#include "chips-3d-model.h"

struct _ChipsMainWindow
{
        GtkWindow parent_object;

        GtkWidget *gl_area;

        Chips3DModel *model;
        GCancellable *model_init_cancellable;

        unsigned int vertex_array_id;

        unsigned int vertex_buffer_id;
        unsigned int vertex_arrangement_id;

        unsigned int shader_program_id;
        unsigned int vertex_shader_id;
        unsigned int fragment_shader_id;

        unsigned int position_attribute_id;

        unsigned int model_loaded : 1;
};

G_DEFINE_TYPE (ChipsMainWindow, chips_main_window, GTK_TYPE_WINDOW);

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

        g_cancellable_cancel (self->model_init_cancellable);
        g_clear_object (&self->model_init_cancellable);

        g_clear_object (&self->model);
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
        size_t vertex_arrangement_size;

        glGenVertexArrays(1, &self->vertex_array_id);
        glBindVertexArray(self->vertex_array_id);

        glGenBuffers (1, &self->vertex_buffer_id);
        glBindBuffer (GL_ARRAY_BUFFER, self->vertex_buffer_id);

        glBufferData (GL_ARRAY_BUFFER,
                      chips_3d_model_get_vertex_buffer_size (self->model),
                      chips_3d_model_get_vertex_buffer (self->model),
                      GL_STATIC_DRAW);

        vertex_arrangement_size = chips_3d_model_get_number_of_vertices (self->model) * sizeof (unsigned int);
        glGenBuffers (1, &self->vertex_arrangement_id);
        glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, self->vertex_arrangement_id);
        glBufferData (GL_ELEMENT_ARRAY_BUFFER,
                      vertex_arrangement_size,
                      chips_3d_model_get_vertex_arrangement (self->model),
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
                               chips_3d_model_get_vertex_buffer_get_stride (self->model),
                               (void *)
                               chips_3d_model_get_vertex_buffer_get_offset (self->model));
}

static void
load_model_if_ready (ChipsMainWindow *self)
{
        if (self->model == NULL || self->model_loaded) {
                return;
        }

        load_vertices (self);
        load_shaders (self);

        self->model_loaded = TRUE;
}

static void
on_gl_area_realized (ChipsMainWindow *self)
{
        g_autoptr (GError) error = NULL;

        if (!initialize_gl (self, &error)) {
                g_error ("%s", error->message);
        }

        load_model_if_ready (self);
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

        if (self->model == NULL) {
                return FALSE;
        }

        glDrawElements (GL_TRIANGLES,
                        chips_3d_model_get_number_of_vertices (self->model),
                        GL_UNSIGNED_INT,
                        0);

        return TRUE;
}

static void
on_3d_model_initialized (GAsyncInitable  *initable,
                         GAsyncResult    *result,
                         ChipsMainWindow *self)
{
        GObject *model;
        g_autoptr (GError) error = NULL;

        model = g_async_initable_new_finish (initable, result, &error);

        if (model == NULL) {
                g_warning ("failed to load model: %s", error->message);
                return;
        }

        self->model = CHIPS_3D_MODEL (model);

        g_clear_object (&self->model_init_cancellable);

        load_model_if_ready (self);
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

        self->model_init_cancellable = g_cancellable_new ();
        g_async_initable_new_async (CHIPS_TYPE_3D_MODEL,
                                    G_PRIORITY_DEFAULT,
                                    self->model_init_cancellable,
                                    (GAsyncReadyCallback)
                                    on_3d_model_initialized,
                                    self,
                                    NULL);
}
