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
CameraHelper::Setup(bool usePointerLock) {
    // on left mouse button, lock mouse pointer
    Input::SetPointerLockHandler([this, usePointerLock] (const InputEvent& event) -> PointerLockMode::Code {
        PointerLockMode::Code mode = PointerLockMode::DontCare;
        if (event.Button == MouseButton::Left) {
            if (event.Type == InputEvent::MouseButtonDown) {
                this->Dragging = true;
                if (usePointerLock) {
                    mode = PointerLockMode::Enable;
                }
            }
            else if (event.Type == InputEvent::MouseButtonUp) {
                if (this->Dragging) {
                    this->Dragging = false;
                    if (usePointerLock) {
                        mode = PointerLockMode::Disable;
                    }
                }
            }
        }
        return mode;
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
    if (Input::KeyboardAttached()) {
        if (Input::KeyDown(Key::P)) {
            this->Paused = !this->Paused;
        }
    }
    if (Input::MouseAttached()) {
        if (this->Dragging) {
            if (Input::MouseButtonPressed(MouseButton::Left)) {
                this->Orbital.y -= Input::MouseMovement().x * 0.01f;
                this->Orbital.x = glm::clamp(this->Orbital.x + Input::MouseMovement().y * 0.01f,
                                             glm::radians(this->MinLatitude),
                                             glm::radians(this->MaxLatitude));
            }
        }
        this->Distance = glm::clamp(this->Distance + Input::MouseScroll().y * 0.5f, this->MinCamDist, this->MaxCamDist);
    }
}

//------------------------------------------------------------------------------
void
CameraHelper::updateTransforms() {
    // recompute projection matrix if framebuffer size has changed
    int w = Gfx::DisplayAttrs().FramebufferWidth;
    int h = Gfx::DisplayAttrs().FramebufferHeight;
    if ((w != this->fbWidth) || (h != this->fbHeight)) {
        this->fbWidth = w;
        this->fbHeight = h;
        this->Proj = glm::perspectiveFov(glm::radians(45.0f), float(w), float(h), this->NearZ, this->FarZ);
        this->InvProj = glm::inverse(this->Proj);
    }
    this->EyePos = (glm::euclidean(this->Orbital) * this->Distance) + this->Center;
    this->View = glm::lookAt(this->EyePos, this->Center, glm::vec3(0.0f, 1.0f, 0.0f));
    this->ViewProj = this->Proj * this->View;
}

} // namespace Oryol
