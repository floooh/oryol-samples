#pragma once
//------------------------------------------------------------------------------
/** 
    @class VisTree
    @brief sparse quad-tree for LOD and visibility detection
*/
#include "Core/Types.h"
#include "Core/Containers/Array.h"
#include "glm/vec3.hpp"
#include "GeomPool.h"
#include "GeomMesher.h"
#include "VisNode.h"
#include "VisBounds.h"
#include "Camera.h"

class VisTree {
public:
    /// number of levels, the most detailed level is 0
    static const int NumLevels = 8;

    /// setup the vistree
    void Setup(int displayWidth, float fov);
    /// discard the vistree
    void Discard();

    /// get node by index
    VisNode& NodeAt(int16_t nodeIndex);
    /// allocate and init a node
    int16_t AllocNode();
    /// free any geoms in a node (non-recursive)
    void FreeGeoms(int16_t nodeIndex);
    /// split a node (create child nodes)
    void Split(int16_t nodeIndex);
    /// merge a node, frees all child nodes recursively
    void Merge(int16_t nodeIndex);
    /// compute the screen-space error for a bounding rect and viewer pos x,y
    float ScreenSpaceError(const VisBounds& bounds, int lvl, int x, int y) const;
    /// traverse the tree, deciding which nodes to render
    void Traverse(const Camera& camera);
    /// apply geoms to a node
    void ApplyGeoms(int16_t nodeIndex, int16_t* geoms, int numGeoms);
    /// internal, recursive traversal method
    void traverse(const Camera& camera, int16_t nodeIndex, const VisBounds& bounds, int lvl, int x, int y);
    /// gather a drawable node, prepare for drawing if needed
    void gatherDrawNode(const Camera& camera, int16_t nodeIndex, int lvl, const VisBounds& bounds);
    /// invalidate any child nodes (free geoms, free nodes)
    void invalidateChildNodes(int16_t nodeIndex);

    /// compute minimal distance between position and bounds
    static float MinDist(int x, int y, const VisBounds& bounds);
    /// get a node's bounds
    static VisBounds Bounds(int lvl, int x, int y);
    /// compute translation vector for a bounds
    static glm::vec3 Translation(const VisBounds& bounds);
    /// compute scale vector for a bounds rect
    static glm::vec3 Scale(const VisBounds& bounds);

    struct GeomGenJob {
        GeomGenJob() : NodeIndex(Oryol::InvalidIndex), Level(0) { }
        GeomGenJob(int16_t nodeIndex, int lvl, const VisBounds& bounds, const glm::vec3& scale, const glm::vec3& trans) :
            NodeIndex(nodeIndex), Level(lvl), Bounds(bounds), Scale(scale), Translate(trans) { }

        int16_t NodeIndex;
        int Level;
        VisBounds Bounds;
        glm::vec3 Scale;
        glm::vec3 Translate;
    };

    float K;
    static const int MaxNumNodes = 1024;
    VisNode nodes[MaxNumNodes];
    Oryol::Array<int16_t> freeNodes;
    Oryol::Array<int16_t> drawNodes;
    Oryol::Array<GeomGenJob> geomGenJobs;
    Oryol::Array<int16_t> freeGeoms;
    Oryol::Array<int16_t> traverseStack;
    int16_t rootNode;
};
