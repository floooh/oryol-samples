//------------------------------------------------------------------------------
//  GeomMesher.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#define STB_VOXEL_RENDER_IMPLEMENTATION
#include "GeomMesher.h"

//------------------------------------------------------------------------------
void
GeomMesher::Setup() {
    stbvox_init_mesh_maker(&this->meshMaker);
    stbvox_set_default_mesh(&this->meshMaker, 0);
}

//------------------------------------------------------------------------------
void
GeomMesher::Discard() {
    // nothing to so here
}

//------------------------------------------------------------------------------
void
GeomMesher::Start() {
    stbvox_reset_buffers(&this->meshMaker);
    stbvox_set_buffer(&this->meshMaker, 0, 0, this->vertices, sizeof(this->vertices));
}

//------------------------------------------------------------------------------
void
GeomMesher::StartVolume(const Volume& vol) {
    const int strideX = vol.ArraySizeY * vol.ArraySizeZ;
    const int strideY = vol.ArraySizeZ;
    stbvox_set_input_stride(&this->meshMaker, strideX, strideY);
    stbvox_set_input_range(&this->meshMaker,
        vol.OffsetX, vol.OffsetY, vol.OffsetZ,
        vol.OffsetX + vol.SizeX,
        vol.OffsetY + vol.SizeY,
        vol.OffsetZ + vol.SizeZ);
    stbvox_input_description* desc = stbvox_get_input_description(&this->meshMaker);
    desc->blocktype = vol.Blocks;
    desc->color = vol.Blocks;
}

//------------------------------------------------------------------------------
GeomMesher::Result
GeomMesher::Meshify() {
    Result result;
    int res = stbvox_make_mesh(&this->meshMaker);

    result.NumQuads = stbvox_get_quad_count(&this->meshMaker, 0);
    result.NumBytes = result.NumQuads * 4 * sizeof(vertex);
    result.Vertices = this->vertices;
    float transform[3][3];
    stbvox_get_transform(&this->meshMaker, transform);
    result.Scale.x = transform[0][0];
    result.Scale.y = transform[0][1];
    result.Scale.z = transform[0][2];
    result.Translate.x = transform[1][0];
    result.Translate.y = transform[1][1];
    result.Translate.z = transform[1][2];
    result.TexTranslate.x = transform[2][0];
    result.TexTranslate.y = transform[2][1];
    result.TexTranslate.z = transform[2][2];

    if (0 == res) {
        result.BufferFull = true;
        stbvox_reset_buffers(&this->meshMaker);
        stbvox_set_buffer(&this->meshMaker, 0, 0, this->vertices, sizeof(this->vertices));
    }
    else {
        result.VolumeDone = true;
    }
    return result;
}
