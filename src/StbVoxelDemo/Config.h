#pragma once
//------------------------------------------------------------------------------
/**
    @file Config.h
    @brief global config values
*/

class Config {
public:
    static const int ChunkSizeXY = 32;
    static const int ChunkSizeZ = 32;
    static const int NumLevels = 5;
    static const int MapDimChunks = (1<<(NumLevels-1));   // size of whole map in item chunks    
    static const int MapDimVoxels = MapDimChunks * Config::ChunkSizeXY;    // size of whole map in voxels
    static const int GeomMaxNumVertices = (1<<15);
    static const int GeomMaxNumQuads = GeomMaxNumVertices / 4;
    static const int GeomMaxNumIndices = GeomMaxNumQuads * 6;
};