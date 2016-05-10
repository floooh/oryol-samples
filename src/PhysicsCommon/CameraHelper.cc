//------------------------------------------------------------------------------
//  CameraHelper.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "CameraHelper.h"
#include "Gfx/Gfx.h"
#include "Input/Input.h"
#include "glm/gtc/matrix_transform.hpp"

namespace Oryol {

//------------------------------------------------------------------------------
void
CameraHelper::Setup() {
    // on left mouse button, lock mouse pointer
    Input::SetMousePointerLockHandler([this] (const Mouse::Event& event) -> Mouse::PointerLockMode {
        if (event.Button == Mouse::LMB) {
            if (event.Type == Mouse::Event::ButtonDown) {
                this->Dragging = true;
                return Mouse::PointerLockModeEnable;
            }
            else if (event.Type == Mouse::Event::ButtonUp) {
                if (this->Dragging) {
                    this->Dragging = false;
                    return Mouse::PointerLockModeDisable;
                }
            }
        }
        return Mouse::PointerLockModeDontCare;
    });
    this->updateTransforms();
}

//------------------------------------------------------------------------------
void
CameraHelper::Update() {
    this->handleInput();
    this->updateTransforms();
}

//------------------------------------------------------------------------------
void
CameraHelper::handleInput() {
    const Keyboard& kbd = Input::Keyboard();
    const Mouse& mouse = Input::Mouse();
    if (kbd.Attached) {
        if (kbd.KeyDown(Key::P)) {
            this->Paused = !this->Paused;
        }
    }
    if (mouse.Attached) {
        if (this->Dragging) {
            if (mouse.ButtonPressed(Mouse::LMB)) {
                this->Orbital.y -= mouse.Movement.x * 0.01f;
                this->Orbital.x = glm::clamp(this->Orbital.x + mouse.Movement.y * 0.01f, glm::radians(this->MinLatitude), glm::radians(this->MaxLatitude));
            }
        }
        this->Distance = glm::clamp(this->Distance + mouse.Scroll.y * 0.1f, this->MinCamDist, this->MaxCamDist);
    }
}

//------------------------------------------------------------------------------
void
CameraHelper::updateTransforms() {
    const float fbWidth = (const float) Gfx::DisplayAttrs().FramebufferWidth;
    const float fbHeight = (const float) Gfx::DisplayAttrs().FramebufferHeight;
    this->Proj = glm::perspectiveFov(glm::radians(45.0f), fbWidth, fbHeight, 0.1f, 400.0f);
    this->EyePos = glm::euclidean(this->Orbital) * this->Distance;
    this->View = glm::lookAt(this->EyePos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    this->ViewProj = this->Proj * this->View;
}

} // namespace Oryol
