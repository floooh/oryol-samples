#pragma once
//------------------------------------------------------------------------------
/**
    @class VisNode
    @brief a node in the VisTree
*/
#include "Core/Types.h"

class VisNode {
public:
    enum Flags {
        GeomPending = (1<<0),   // geom is currently prepared for drawing
    };
    static const int16_t InvalidGeom = -1;
    static const int16_t EmptyGeom = -2;
    static const int16_t InvalidChild = -1;
    static const int NumGeoms = 3;
    static const int NumChilds = 4;
    uint16_t flags;
    int16_t geoms[NumGeoms];       // up to 3 geoms
    int16_t childs[NumChilds];     // 4 child nodes (or none)

    /// reset the node
    void Reset() {
        this->flags = 0;
        for (int i = 0; i < NumGeoms; i++) {
            this->geoms[i] = InvalidGeom;
        }
        for (int i = 0; i < NumChilds; i++) {
            this->childs[i] = InvalidChild;
        }
    }
    /// return true if this is a leaf node
    bool IsLeaf() const {
        return InvalidChild == this->childs[0];
    }
    /// return true if has node has a draw geom assigned
    bool HasGeom() const {
        return InvalidGeom != this->geoms[0];
    }
    /// return true if the node is an empty volume (doesn't need drawing even if visible)
    bool HasEmptyGeom() const {
        return EmptyGeom == this->geoms[0];
    }
    /// return true if the node needs geoms to be generated
    bool NeedsGeom() const {
        return (InvalidGeom == this->geoms[0]) && !(this->flags & GeomPending);
    }
    /// return true if node is waiting for geom
    bool WaitsForGeom() const {
        return this->flags & GeomPending;
    }
};
