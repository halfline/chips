#include "chips-3d-model.h"

static void initable_iface_init       (GInitableIface      *initable_iface);
static void async_initable_iface_init (GAsyncInitableIface *async_initable_iface);
G_DEFINE_TYPE_WITH_CODE (Chips3DModel, chips_3d_model, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, initable_iface_init)
                         G_IMPLEMENT_INTERFACE (G_TYPE_ASYNC_INITABLE, async_initable_iface_init));

typedef struct
{
        float        *vertex_buffer;
        size_t        vertex_buffer_size;
        unsigned int  number_of_vertices;
        unsigned int *vertex_arrangement;
} Chips3DModelPrivate;

#define CHIPS_3D_MODEL_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), CHIPS_TYPE_3D_MODEL, Chips3DModelPrivate))

static gboolean
initable_init (GInitable     *initable,
               GCancellable  *cancellable,
               GError       **error)
{
        Chips3DModel *self = CHIPS_3D_MODEL (initable);
        Chips3DModelPrivate *priv = CHIPS_3D_MODEL_GET_PRIVATE (self);
        size_t arrangement_buffer_size;
        size_t i;
        static float vertices[] = {
                -0.5f, -0.5f, -0.5f,
                -0.5f,  0.5f, -0.5f,
                 0.5f,  0.5f, -0.5f,
                 0.5f,  0.5f, -0.5f,
                 0.5f, -0.5f, -0.5f,
                -0.5f, -0.5f, -0.5f,

                -0.5f, -0.5f,  0.5f,
                 0.5f, -0.5f,  0.5f,
                 0.5f,  0.5f,  0.5f,
                 0.5f,  0.5f,  0.5f,
                -0.5f,  0.5f,  0.5f,
                -0.5f, -0.5f,  0.5f,

                -0.5f,  0.5f,  0.5f,
                -0.5f,  0.5f, -0.5f,
                -0.5f, -0.5f, -0.5f,
                -0.5f, -0.5f, -0.5f,
                -0.5f, -0.5f,  0.5f,
                -0.5f,  0.5f,  0.5f,

                 0.5f,  0.5f,  0.5f,
                 0.5f, -0.5f,  0.5f,
                 0.5f, -0.5f, -0.5f,
                 0.5f, -0.5f, -0.5f,
                 0.5f,  0.5f, -0.5f,
                 0.5f,  0.5f,  0.5f,

                -0.5f, -0.5f, -0.5f,
                 0.5f, -0.5f, -0.5f,
                 0.5f, -0.5f,  0.5f,
                 0.5f, -0.5f,  0.5f,
                -0.5f, -0.5f,  0.5f,
                -0.5f, -0.5f, -0.5f,

                -0.5f,  0.5f, -0.5f,
                -0.5f,  0.5f,  0.5f,
                 0.5f,  0.5f,  0.5f,
                 0.5f,  0.5f,  0.5f,
                 0.5f,  0.5f, -0.5f,
                -0.5f,  0.5f, -0.5f,
        };

        priv->number_of_vertices = G_N_ELEMENTS (vertices) / 3;
        priv->vertex_buffer_size = sizeof (vertices);

        priv->vertex_buffer = g_malloc (priv->vertex_buffer_size);
        memcpy (priv->vertex_buffer,
                vertices,
                priv->vertex_buffer_size);

        arrangement_buffer_size = priv->number_of_vertices * sizeof (unsigned int);
        priv->vertex_arrangement = g_malloc (arrangement_buffer_size);

        for (i = 0; i < priv->number_of_vertices; i++) {
                priv->vertex_arrangement[i] = i;
        }

        return TRUE;
}

static void
initable_iface_init (GInitableIface *initable_iface)
{
        initable_iface->init = initable_init;
}

static void
async_initable_iface_init (GAsyncInitableIface *async_initable_iface)
{
}

static void
chips_3d_model_dispose (GObject *object)
{
        Chips3DModel *self = CHIPS_3D_MODEL (object);
        Chips3DModelPrivate *priv = CHIPS_3D_MODEL_GET_PRIVATE (self);

        g_clear_pointer (&priv->vertex_buffer, g_free);
        g_clear_pointer (&priv->vertex_arrangement, g_free);

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

unsigned int *
chips_3d_model_get_vertex_arrangement (Chips3DModel *self)
{
        Chips3DModelPrivate *priv = CHIPS_3D_MODEL_GET_PRIVATE (self);

        return priv->vertex_arrangement;
}

intptr_t
chips_3d_model_get_vertex_buffer_get_stride (Chips3DModel *self)
{
        return 3 * sizeof (float);
}

intptr_t
chips_3d_model_get_vertex_buffer_get_offset (Chips3DModel *self)
{
        return 0;
}
