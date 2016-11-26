#ifndef CHIPS_3D_MODEL_H
#define CHIPS_3D_MODEL_H

#include "chips.h"

#define CHIPS_TYPE_3D_MODEL chips_3d_model_get_type ()
G_DECLARE_DERIVABLE_TYPE (Chips3DModel, chips_3d_model, CHIPS, 3D_MODEL, GObject);

struct _Chips3DModelClass
{
        GObjectClass parent_class;
};

const float * chips_3d_model_get_vertex_buffer      (Chips3DModel *self);
size_t        chips_3d_model_get_vertex_buffer_size (Chips3DModel *self);

unsigned int  chips_3d_model_get_number_of_vertices (Chips3DModel *self);

intptr_t      chips_3d_model_get_vertex_buffer_get_stride (Chips3DModel *self);
intptr_t      chips_3d_model_get_vertex_buffer_get_offset (Chips3DModel *self);

#endif /* CHIPS_3D_MODEL_H */
