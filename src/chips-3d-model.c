#include "chips-3d-model.h"

G_DEFINE_TYPE (Chips3DModel, chips_3d_model, G_TYPE_OBJECT);

typedef struct
{
        float        *vertex_buffer;
        size_t        vertex_buffer_size;
        unsigned int  number_of_vertices;
} Chips3DModelPrivate;

#define CHIPS_3D_MODEL_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), CHIPS_TYPE_3D_MODEL, Chips3DModelPrivate))

static void
chips_3d_model_dispose (GObject *object)
{
        Chips3DModel *self = CHIPS_3D_MODEL (object);
        Chips3DModelPrivate *priv = CHIPS_3D_MODEL_GET_PRIVATE (self);

        g_clear_pointer (&priv->vertex_buffer, g_free);

        G_OBJECT_CLASS (chips_3d_model_parent_class)->dispose (object);
}

static void
chips_3d_model_finalize (GObject *object)
{
        Chips3DModel *self = CHIPS_3D_MODEL (object);

        G_OBJECT_CLASS (chips_3d_model_parent_class)->finalize (object);
}

static void
chips_3d_model_class_init (Chips3DModelClass *own_class)
{
        GObjectClass *object_class = G_OBJECT_CLASS (own_class);

        object_class->dispose = chips_3d_model_dispose;
        object_class->finalize = chips_3d_model_finalize;

        g_type_class_add_private (own_class, sizeof (Chips3DModelClass));
}

static void
chips_3d_model_init (Chips3DModel *self)
{
        Chips3DModelPrivate *priv = CHIPS_3D_MODEL_GET_PRIVATE (self);

        priv->number_of_vertices = 3;
        priv->vertex_buffer_size = priv->number_of_vertices * 3 * sizeof (float);
        priv->vertex_buffer = g_malloc (priv->vertex_buffer_size);
        memcpy (priv->vertex_buffer,
                (float[]) {  0.0,  0.5, 0.0,
                             0.5, -0.5, 0.0,
                            -0.5, -0.5, 0.0  },
                priv->vertex_buffer_size);
}

const float *
chips_3d_model_get_vertex_buffer (Chips3DModel *self)
{
        Chips3DModelPrivate *priv = CHIPS_3D_MODEL_GET_PRIVATE (self);

        return priv->vertex_buffer;
}

size_t
chips_3d_model_get_vertex_buffer_size (Chips3DModel *self)
{
        Chips3DModelPrivate *priv = CHIPS_3D_MODEL_GET_PRIVATE (self);

        return priv->vertex_buffer_size;
}

unsigned int
chips_3d_model_get_number_of_vertices (Chips3DModel *self)
{
        Chips3DModelPrivate *priv = CHIPS_3D_MODEL_GET_PRIVATE (self);

        return priv->number_of_vertices;
}

intptr_t
chips_3d_model_get_vertex_buffer_get_stride (Chips3DModel *self)
{
        return 0;
}

intptr_t
chips_3d_model_get_vertex_buffer_get_offset (Chips3DModel *self)
{
        return 0;
}
