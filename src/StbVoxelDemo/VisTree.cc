//------------------------------------------------------------------------------
//  VisTree.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "Config.h"
#include "VisTree.h"
#include "glm/trigonometric.hpp"

using namespace Oryol;

//------------------------------------------------------------------------------
void
VisTree::Setup(int displayWidth, float fov) {

    // compute K for screen space error computation
    // (see: http://tulrich.com/geekstuff/sig-notes.pdf )
    this->K = displayWidth / (2.0f * glm::tan(fov*0.5f));

    this->drawNodes.Reserve(MaxNumNodes);
    this->freeNodes.Reserve(MaxNumNodes);
    this->geomGenJobs.Reserve(MaxNumNodes);
    this->freeGeoms.Reserve(MaxNumNodes);
    this->traverseStack.Reserve(NumLevels);
    for (int i = MaxNumNodes-1; i >=0; i--) {
        this->freeNodes.Add(i);
    }
    this->rootNode = this->AllocNode();
}

//------------------------------------------------------------------------------
void
VisTree::Discard() {
    this->freeNodes.Clear();
}

//------------------------------------------------------------------------------
VisNode&
VisTree::NodeAt(int16_t nodeIndex) {
    o_assert_dbg((nodeIndex >= 0) && (nodeIndex < MaxNumNodes));
    return this->nodes[nodeIndex];
}

//------------------------------------------------------------------------------
int16_t
VisTree::AllocNode() {
    int16_t index = this->freeNodes.PopBack();
    VisNode& node = this->nodes[index];
    node.Reset();
    return index;
}

//------------------------------------------------------------------------------
void
VisTree::FreeGeoms(int16_t nodeIndex) {
    VisNode& node = this->NodeAt(nodeIndex);
    for (int geomIndex = 0; geomIndex < VisNode::NumGeoms; geomIndex++) {
        if (node.geoms[geomIndex] >= 0) {
            this->freeGeoms.Add(node.geoms[geomIndex]);
            node.geoms[geomIndex] = VisNode::InvalidGeom;
        }
        else {
            break;
        }
    }
}

//------------------------------------------------------------------------------
void
VisTree::Split(int16_t nodeIndex) {
    // turns a leaf node into an inner node, do NOT free geom
    VisNode& node = this->NodeAt(nodeIndex);
    o_assert_dbg(node.IsLeaf());
    for (int childIndex = 0; childIndex < VisNode::NumChilds; childIndex++) {
        o_assert_dbg(VisNode::InvalidChild == node.childs[childIndex]);
        node.childs[childIndex] = this->AllocNode();
    }
    node.flags &= ~VisNode::GeomPending;
}

//------------------------------------------------------------------------------
void
VisTree::Merge(int16_t nodeIndex) {
    // turns an inner node into a leaf node by recursively removing
    // children and any encountered draw geoms
    VisNode& node = this->NodeAt(nodeIndex);
    for (int childIndex = 0; childIndex < VisNode::NumChilds; childIndex++) {
        if (VisNode::InvalidChild != node.childs[childIndex]) {
            VisNode& childNode = this->NodeAt(node.childs[childIndex]);
            this->FreeGeoms(node.childs[childIndex]);
            this->Merge(node.childs[childIndex]);
            this->freeNodes.Add(node.childs[childIndex]);
            node.childs[childIndex] = VisNode::InvalidChild;
        }
    }
}

//------------------------------------------------------------------------------
float
VisTree::ScreenSpaceError(const VisBounds& bounds, int lvl, int posX, int posY) const {
    // see http://tulrich.com/geekstuff/sig-notes.pdf

    // we just fudge the geometric error of the chunk by doubling it for
    // each tree level
    const float delta = float(1<<lvl);
    const float D = MinDist(posX, posY, bounds)+1.0f;
    float rho = (delta/D) * this->K;
    return rho;
}

//------------------------------------------------------------------------------
void
VisTree::Traverse(const Camera& camera) {
    // traverse the entire tree to find draw nodes
    // split and merge nodes based required LOD,
    int lvl = NumLevels;
    int nodeIndex = this->rootNode;
    int posX = camera.Pos.x;
    int posY = camera.Pos.z;
    VisBounds bounds = VisTree::Bounds(lvl, 0, 0);
    this->drawNodes.Clear();
    this->traverse(camera, nodeIndex, bounds, lvl, posX, posY);
}

//------------------------------------------------------------------------------
void
VisTree::traverse(const Camera& camera, int16_t nodeIndex, const VisBounds& bounds, int lvl, int posX, int posY) {
    this->traverseStack.Add(nodeIndex);
    VisNode& node = this->NodeAt(nodeIndex);
    float rho = this->ScreenSpaceError(bounds, lvl, posX, posY);
    const float tau = 15.0f;
    if ((rho <= tau) || (0 == lvl)) {
        this->gatherDrawNode(camera, nodeIndex, lvl, bounds);
    }
    else {
        if (node.IsLeaf()) {
            this->Split(nodeIndex);
        }
        const int halfX = (bounds.x1 - bounds.x0)/2;
        const int halfY = (bounds.y1 - bounds.y0)/2;
        for (int x = 0; x < 2; x++) {
            for (int y = 0; y < 2; y++) {
                VisBounds childBounds;
                childBounds.x0 = bounds.x0 + x*halfX;
                childBounds.x1 = childBounds.x0 + halfX;
                childBounds.y0 = bounds.y0 + y*halfY;
                childBounds.y1 = childBounds.y0 + halfY;
                const int childIndex = (y<<1)|x;
                this->traverse(camera, node.childs[childIndex], childBounds, lvl-1, posX, posY);
            }
        }
    }
    this->traverseStack.PopBack();
}

//------------------------------------------------------------------------------
void
VisTree::gatherDrawNode(const Camera& camera, int16_t nodeIndex, int lvl, const VisBounds& bounds) {
    VisNode& node = this->NodeAt(nodeIndex);

    // FIXME FIXME FIXME: this code needs a thorough cleanup, esp gathering
    // and releasing the parent/child node placeholder geoms

    bool needsPlaceholder = false;
    if (camera.BoxVisible(bounds.x0, bounds.x1, 0, Config::ChunkSizeZ, bounds.y0, bounds.y1)) {
        if (!node.HasEmptyGeom() && node.NeedsGeom()) {
            // enqueue a new geom-generation job
            node.flags |= VisNode::GeomPending;
            glm::vec3 scale = Scale(bounds);
            glm::vec3 trans = Translation(bounds);
            this->geomGenJobs.Add(GeomGenJob(nodeIndex, lvl, bounds, scale, trans));
            needsPlaceholder = true;
        }
        else if (node.WaitsForGeom()) {
            needsPlaceholder = true;
        }
        if (needsPlaceholder) {
            // prefer child nodes as placeholder
            if ((VisNode::InvalidChild != node.childs[0]) &&
                (VisNode::InvalidGeom != this->NodeAt(node.childs[0]).geoms[0])) {
                for (int i = 0; i < VisNode::NumChilds; i++) {
                    this->drawNodes.Add(node.childs[i]);
                }
            }
            // otherwise check parent node as placeholder
            else if ((this->traverseStack.Size() > 1) &&
                     (VisNode::InvalidGeom != this->NodeAt(this->traverseStack[this->traverseStack.Size()-2]).geoms[0]))
            {
                this->drawNodes.Add(this->traverseStack[this->traverseStack.Size()-2]);
            }
        }
        else {
            if (!node.HasEmptyGeom()) {
                this->drawNodes.Add(nodeIndex);
            }
        }
    }
    // clean up any nodes and geoms that might have been used as placeholder
    if (!needsPlaceholder) {
        if (!node.IsLeaf()) {
            this->Merge(nodeIndex);
        }
        // free any parent node geoms
        // FIXME: doing this each time is terrible!
        int numParents = this->traverseStack.Size() - 2;    // FIXME: this should be -1? but this leaks geoms :/
        for (int i = 0; i < numParents; i++) {
            this->FreeGeoms(this->traverseStack[i]);
        }
    }
}

//------------------------------------------------------------------------------
void
VisTree::ApplyGeoms(int16_t nodeIndex, int16_t* geoms, int numGeoms) {
    VisNode& node = this->NodeAt(nodeIndex);
    if (node.WaitsForGeom()) {
        for (int i = 0; i < VisNode::NumGeoms; i++) {
            o_assert_dbg(VisNode::InvalidGeom == node.geoms[i]);
            if (i < numGeoms) {
                node.geoms[i] = geoms[i];
            }
            else {
                node.geoms[i] = VisNode::InvalidGeom;
            }
        }
        node.flags &= ~VisNode::GeomPending;
    }
    else {
        // if the node didn't actually wait for geoms any longer,
        // immediately kill the geoms
        for (int i = 0; i < numGeoms; i++) {
            o_assert_dbg(VisNode::InvalidGeom != geoms[i]);
            this->freeGeoms.Add(geoms[i]);
        }
    }
}

//------------------------------------------------------------------------------
float
VisTree::MinDist(int x, int y, const VisBounds& bounds) {
    int dx;
    int dx0 = x - bounds.x0;
    int dx1 = bounds.x1 - x;
    if ((dx0 >= 0) && (dx1 >= 0)) {
        dx = 0;     // inside
    }
    else {
        dx0 *= dx0;
        dx1 *= dx1;
        dx = dx0<dx1 ? dx0:dx1;
    }

    int dy;
    int dy0 = y - bounds.y0;
    int dy1 = bounds.y1 - y;
    if ((dy0 >= 0) && (dy1 >= 0)) {
        dy = 0;
    }
    else {
        dy0 *= dy0;
        dy1 *= dy1;
        dy = dy0<dy1 ? dy0:dy1;
    }
    float d = glm::sqrt(dx+dy);
    return d;
}

//------------------------------------------------------------------------------
VisBounds
VisTree::Bounds(int lvl, int x, int y) {
    o_assert_dbg(lvl <= NumLevels);
    // level 0 is most detailed, level == NumLevels is the root node
    int dim = (1<<lvl) * Config::ChunkSizeXY;
    VisBounds bounds;
    bounds.x0 = (x>>lvl) * dim;
    bounds.x1 = bounds.x0 + dim;
    bounds.y0 = (y>>lvl) * dim;
    bounds.y1 = bounds.y0 + dim;
    return bounds;
}

//------------------------------------------------------------------------------
glm::vec3
VisTree::Translation(const VisBounds& bounds) {
    return glm::vec3(bounds.x0 - (bounds.x1-bounds.x0)/Config::ChunkSizeXY,
                     bounds.y0 - (bounds.y1-bounds.y0)/Config::ChunkSizeXY,
                     0.0f);
}

//------------------------------------------------------------------------------
glm::vec3
VisTree::Scale(const VisBounds& bounds) {
    return glm::vec3((bounds.x1-bounds.x0)/Config::ChunkSizeXY,
                     (bounds.y1-bounds.y0)/Config::ChunkSizeXY,
                     1.0f);
}
