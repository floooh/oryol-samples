//------------------------------------------------------------------------------
//  Camera.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "Camera.h"
#include "glm/trigonometric.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/matrix_access.hpp"

using namespace Oryol;

//------------------------------------------------------------------------------
void
Camera::Setup(const glm::vec3 pos, float fov, int dispWidth, int dispHeight, float near, float far) {
    this->Pos = pos;
    this->Model = glm::translate(glm::mat4(), pos);
    this->UpdateProj(fov, dispWidth, dispHeight, near, far);
}

//------------------------------------------------------------------------------
void
Camera::UpdateProj(float fov, int dispWidth, int dispHeight, float near, float far) {
    this->Proj = glm::perspectiveFov(fov, float(dispWidth), float(dispHeight), near, far);
    this->updateViewProjFrustum();
}

//------------------------------------------------------------------------------
void
Camera::UpdateModel(const glm::mat4& model) {
    this->Model = model;
    this->updateViewProjFrustum();
}

//------------------------------------------------------------------------------
void
Camera::MoveRotate(const glm::vec3& move, const glm::vec2& rot) {
    const glm::vec3 up(0.0f, 1.0f, 0.0f);
    const glm::vec3 hori(1.0f, 0.0f, 0.0f);
    this->Rot += rot;
    glm::mat4 m = glm::translate(glm::mat4(), this->Pos);
    m = glm::rotate(m, this->Rot.x, up);
    m = glm::rotate(m, this->Rot.y, hori);
    m = glm::translate(m, move);
    this->Model = m;
    this->Pos = glm::vec3(this->Model[3].x, this->Model[3].y, this->Model[3].z);
    this->updateViewProjFrustum();
}

//------------------------------------------------------------------------------
bool
Camera::testPlane(const glm::vec4& p, float x0, float x1, float y0, float y1, float z0, float z1) {
    // see: https://github.com/nothings/stb/blob/master/tests/caveview/cave_render.c
    float d=0.0f;
    d += p.x > 0.0f ? x1 * p.x : x0 * p.x;
    d += p.y > 0.0f ? y1 * p.y : y0 * p.y;
    d += p.z > 0.0f ? z1 * p.z : z0 * p.z;
    d += p.w;
    return d >= 0.0f;
}

//------------------------------------------------------------------------------
bool
Camera::BoxVisible(int x0, int x1, int y0, int y1, int z0, int z1) const {
    // see: https://github.com/nothings/stb/blob/master/tests/caveview/cave_render.c
    for (int i = 0; i < 6; i++) {
        if (!testPlane(this->Frustum[i], x0, x1, y0, y1, z0, z1)) {
            return false;
        }
    }
    return true;
}

//------------------------------------------------------------------------------
void
Camera::updateViewProjFrustum() {
    this->View = glm::inverse(this->Model);
    this->ViewProj = this->Proj * this->View;

    // extract frustum
    // https://fgiesen.wordpress.com/2012/08/31/frustum-planes-from-the-projection-matrix/
    // http://gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf
    glm::vec4 rx = glm::row(this->ViewProj, 0);
    glm::vec4 ry = glm::row(this->ViewProj, 1);
    glm::vec4 rz = glm::row(this->ViewProj, 2);
    glm::vec4 rw = glm::row(this->ViewProj, 3);

    this->Frustum[0] = rw + rx;
    this->Frustum[1] = rw - rx;
    this->Frustum[2] = rw + ry;
    this->Frustum[3] = rw - ry;
    this->Frustum[4] = rw + rz;
    this->Frustum[5] = rw - rz;
}

