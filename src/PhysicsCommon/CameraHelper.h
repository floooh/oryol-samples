#pragma once
//------------------------------------------------------------------------------
/**
    @class Oryol::CameraHelper
    @brief orbiting camera helper class
*/
#include "Core/Types.h"
#include "glm/gtx/polar_coordinates.hpp"

namespace Oryol {

class CameraHelper {
public:
    /// setup the camera object
    void Setup();
    /// update the camera
    void Update();

    /// update transform matrices
    void updateTransforms();
    /// handle input
    void handleInput();

    bool Paused = false;
    float MinCamDist = 5.0f;
    float MaxCamDist = 100.0f;
    float MinLatitude = -85.0f;
    float MaxLatitude = 85.0f;
    float Distance = 60.0f;
    glm::vec2 Orbital = glm::vec2(glm::radians(25.0f), glm::radians(180.0f));
    bool Dragging = false;
    glm::vec3 Center;
    glm::vec3 EyePos;
    glm::mat4 View;
    glm::mat4 Proj;
    glm::mat4 ViewProj;

};

} // namespace Oryol
