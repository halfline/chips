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

        GtkGesture *zoom_gesture;
        double      scale;

        GtkGesture *drag_gesture;

        Chips3DModel *model;
        GCancellable *model_init_cancellable;

        graphene_vec3_t camera_position;
        graphene_vec3_t camera_focal_point;
        graphene_vec3_t camera_up_direction;

        float field_of_view;
        float aspect_ratio;
        float near_plane;
        float far_plane;

        unsigned int vertex_array_id;

        unsigned int vertex_buffer_id;
        unsigned int vertex_arrangement_id;

        unsigned int shader_program_id;
        unsigned int vertex_shader_id;
        unsigned int fragment_shader_id;

        unsigned int position_attribute_id;

        graphene_matrix_t model_matrix;
        unsigned int model_matrix_id;

        graphene_matrix_t view_matrix;
        unsigned int view_matrix_id;

        graphene_matrix_t projection_matrix;
        unsigned int projection_matrix_id;

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
"in vec3 position;\n"
"out vec3 color;\n"
"uniform mat4 model_matrix;\n"
"uniform mat4 view_matrix;\n"
"uniform mat4 projection_matrix;\n"
"void\n"
"main ()\n"
"{\n"
"        gl_Position = projection_matrix * view_matrix * model_matrix * vec4 (position, 1.0);\n"
"        color = vec3 (1.0 - gl_Position.z/10.0, 1.0 - gl_Position.z/10.0, 1.0 - gl_Position.z/10.0);\n"
"}\n";

static const char *fragment_shader =
"#version 330\n"
"in vec3 color;\n"
"out vec4 fragment_color;\n"
"void main ()\n"
"{\n"
"        fragment_color = vec4 (color, 1.0);\n"
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

        glEnable (GL_DEPTH_TEST);
        glEnable (GL_CULL_FACE);

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
place_camera (ChipsMainWindow   *self,
              float              x,
              float              y,
              float              z,
              graphene_matrix_t *view_matrix)
{
        graphene_vec3_t position, direction, front;
        const graphene_vec3_t *top;

        graphene_vec3_init (&position, x, y, z);

        graphene_vec3_negate (graphene_vec3_z_axis (), &direction);
        graphene_vec3_add (&position, &direction, &front);
        graphene_vec3_normalize (&front, &front);

        top = graphene_vec3_y_axis ();

        graphene_matrix_init_look_at (view_matrix,
                                      &position,
                                      &front,
                                      top);
}

static void
load_matrices (ChipsMainWindow *self)
{
        GtkAllocation gl_area_allocation;

        graphene_matrix_init_identity (&self->model_matrix);

        place_camera (self, 1.5, 1.0, 5.0, &self->view_matrix);

        gtk_widget_get_allocation (self->gl_area, &gl_area_allocation);

        self->field_of_view = 45;
        self->aspect_ratio = (1.0 * gl_area_allocation.width) / gl_area_allocation.height;
        self->near_plane = 1.0;
        self->far_plane = 10;

        graphene_matrix_init_perspective (&self->projection_matrix,
                                          self->field_of_view,
                                          self->aspect_ratio,
                                          self->near_plane,
                                          self->far_plane);
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
        self->model_matrix_id = glGetUniformLocation (self->shader_program_id, "model_matrix");
        self->view_matrix_id = glGetUniformLocation (self->shader_program_id, "view_matrix");
        self->projection_matrix_id = glGetUniformLocation (self->shader_program_id, "projection_matrix");
}

static void
upload_model_to_shaders (ChipsMainWindow *self)
{
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
upload_matrix_to_shaders (ChipsMainWindow   *self,
                          unsigned int       matrix_id,
                          graphene_matrix_t *matrix)
{
        float matrix_values[16];

        glUseProgram (self->shader_program_id);
        graphene_matrix_to_float (matrix, matrix_values);
        glUniformMatrix4fv(matrix_id,
                           1,
                           GL_FALSE,
                           matrix_values);
}

static void
upload_data_to_shaders (ChipsMainWindow *self)
{
        upload_model_to_shaders (self);
        upload_matrix_to_shaders (self,
                                  self->model_matrix_id,
                                  &self->model_matrix);
        upload_matrix_to_shaders (self,
                                  self->view_matrix_id,
                                  &self->view_matrix);
        upload_matrix_to_shaders (self,
                                  self->projection_matrix_id,
                                  &self->projection_matrix);
}

static void
load_model_if_ready (ChipsMainWindow *self)
{
        if (self->model == NULL || self->model_loaded) {
                return;
        }

        load_matrices (self);
        load_vertices (self);
        load_shaders (self);
        upload_data_to_shaders (self);

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
        glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
on_gl_area_pinched (ChipsMainWindow *self)
{
#if 0
        graphene_matrix_t model_matrix;
        double scale;

        scale = gtk_gesture_zoom_get_scale_delta (GTK_GESTURE_ZOOM (self->zoom_gesture));

        graphene_matrix_init_from_matrix (&model_matrix, &self->model_matrix);

        graphene_matrix_scale (&model_matrix, scale, scale, scale);
        upload_matrix_to_shaders (self,
                                  self->model_matrix_id,
                                  &model_matrix);
#endif
        graphene_matrix_t view_matrix;
        graphene_vec3_t scale_vector, camera_vector;
        double scale;

        scale = gtk_gesture_zoom_get_scale_delta (GTK_GESTURE_ZOOM (self->zoom_gesture));

        graphene_matrix_init_from_matrix (&view_matrix, &self->view_matrix);
        graphene_vec3_init (&scale_vector, 0.0, 0.0, scale);
        graphene_vec3_add (&self->camera_position, &scale_vector, &camera_vector);
        graphene_matrix_init_look_at (&self->view_matrix,
                                      &camera_vector,
                                      &self->camera_focal_point,
                                      &self->camera_up_direction);

        upload_matrix_to_shaders (self,
                                  self->view_matrix_id,
                                  &view_matrix);
        gtk_widget_queue_draw (self->gl_area);
}

static void
on_gl_area_dragged (ChipsMainWindow *self,
                    double           offset_x,
                    double           offset_y)
{
        graphene_matrix_t model_matrix;
        graphene_matrix_t model_view_matrix;
        graphene_matrix_t model_view_projection_matrix;
        graphene_matrix_t inverse_matrix;

        graphene_vec4_t rotation_vector, transformed_vector, normalized_vector;
        graphene_matrix_init_from_matrix (&model_matrix, &self->model_matrix);
        graphene_matrix_multiply (&self->view_matrix, &self->model_matrix, &model_view_matrix);
        graphene_matrix_multiply (&self->projection_matrix, &model_view_matrix, &model_view_projection_matrix);
        graphene_matrix_inverse (&model_view_projection_matrix, &inverse_matrix);

        graphene_vec4_init (&rotation_vector,
                            offset_x,
                            offset_y,
                            0.0,
                            0.0);

        graphene_matrix_transform_vec4 (&inverse_matrix, &rotation_vector, &transformed_vector);
        graphene_vec4_normalize (&transformed_vector, &normalized_vector);

        graphene_matrix_rotate_y (&model_matrix,
                                  graphene_vec4_get_x (&transformed_vector));
        graphene_matrix_rotate_x (&model_matrix,
                                  graphene_vec4_get_y (&transformed_vector));

        upload_matrix_to_shaders (self,
                                  self->model_matrix_id,
                                  &model_matrix);

        gtk_widget_queue_draw (self->gl_area);
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

        gtk_widget_add_events (self->gl_area,
                               GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_TOUCH_MASK);

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

        self->scale = 1.0;

        self->zoom_gesture = gtk_gesture_zoom_new (self->gl_area);
        g_signal_connect_swapped (self->zoom_gesture,
                                  "scale-changed",
                                  G_CALLBACK (on_gl_area_pinched),
                                  self);
        gtk_event_controller_set_propagation_phase (GTK_EVENT_CONTROLLER (self->zoom_gesture),
                                                    GTK_PHASE_BUBBLE);

        self->drag_gesture = gtk_gesture_drag_new (self->gl_area);
        g_signal_connect_swapped (self->drag_gesture,
                                  "drag-update",
                                  G_CALLBACK (on_gl_area_dragged),
                                  self);
        gtk_event_controller_set_propagation_phase (GTK_EVENT_CONTROLLER (self->drag_gesture),
                                                    GTK_PHASE_BUBBLE);

        self->model_init_cancellable = g_cancellable_new ();
        g_async_initable_new_async (CHIPS_TYPE_3D_MODEL,
                                    G_PRIORITY_DEFAULT,
                                    self->model_init_cancellable,
                                    (GAsyncReadyCallback)
                                    on_3d_model_initialized,
                                    self,
                                    NULL);
}
