#pragma once
//------------------------------------------------------------------------------
/**
    @VoxelGenerator
    @brief generate voxel chunk data and meshify them
*/
#include "Volume.h"
#include "Config.h"
#include "VisBounds.h"

class VoxelGenerator {
public:
    static const int VolumeSizeXY = Config::ChunkSizeXY + 2;
    static const int VolumeSizeZ = Config::ChunkSizeZ + 2;

    /// generate simplex noise voxel data
    Volume GenSimplex(const VisBounds& bounds);
    /// generate debug voxel data
    Volume GenDebug(const VisBounds& bounds, int lvl);

    /// initialize a volume object
    Volume initVolume();

    uint8_t voxels[VolumeSizeXY][VolumeSizeXY][VolumeSizeZ];
};