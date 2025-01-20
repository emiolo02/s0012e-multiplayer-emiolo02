#include "config.h"
#include "spaceship.h"

#include "input/inputserver.h"
#include "render/cameramanager.h"
#include "render/physics.h"
#include "render/debugrender.h"
#include "render/particlesystem.h"

using namespace Input;
using namespace glm;
using namespace Render;

namespace Game {
    Laser::Laser(const Transform &transform)
        : origin(transform.GetPosition()),
          direction(transform.GetOrientation() * vec3(0.0f, 0.0f, 1.0f)) {
        this->transform = transform;
    }

    void Laser::Update(const float dt) {
        transform.AddPosition(direction * speed * dt);
    }

    void SpaceShipCamera::Update(const float dt) {
        // update camera view transform
        Camera *cam = CameraManager::GetCamera(CAMERA_MAIN);
        const vec3 targetPosition(target[3]);
        const vec3 desiredCamPos = targetPosition + vec3(target * vec4(0, camOffsetY, -4.0f, 0));

        const vec3 tmp = mix(position, desiredCamPos, dt * cameraSmoothFactor);
        if (!isnan(tmp.x))
            position = tmp;

        cam->view = lookAt(position, position + vec3(target[2]), vec3(target[1]));
    }

    SpaceShip::~SpaceShip() {
        delete particleEmitterLeft;
        delete particleEmitterRight;
    }

    void
    SpaceShip::Init() {
        constexpr uint32_t numParticles = 2048;
        const mat4 transformMat = transform.GetMatrix();
        const vec3 position = transform.GetPosition();
        this->particleEmitterLeft = new ParticleEmitter(numParticles);
        this->particleEmitterLeft->data = {
            .origin = vec4(position + vec3(transformMat[2]) * emitterOffset, 1),
            .dir = vec4(vec3(-transformMat[2]), 0),
            .startColor = vec4(0.38f, 0.76f, 0.95f, 1.0f) * 2.0f,
            .endColor = vec4(0, 0, 0, 1.0f),
            .numParticles = numParticles,
            .theta = radians(0.0f),
            .startSpeed = 1.2f,
            .endSpeed = 0.0f,
            .startScale = 0.025f,
            .endScale = 0.0f,
            .decayTime = 2.58f,
            .randomTimeOffsetDist = 2.58f,
            .looping = 1,
            .emitterType = 1,
            .discRadius = 0.020f
        };
        this->particleEmitterRight = new ParticleEmitter(numParticles);
        this->particleEmitterRight->data = this->particleEmitterLeft->data;

        ParticleSystem::Instance()->AddEmitter(this->particleEmitterLeft);
        ParticleSystem::Instance()->AddEmitter(this->particleEmitterRight);
        init = true;
    }

    void SpaceShip::Update(const float dt) {
        if (!init)
            return;

        predictedBody.Interpolate(transform, dt);

        const vec3 position = transform.GetPosition();
        const mat4 transformMat = transform.GetMatrix();
        constexpr float thrusterPosOffset = 0.365f;

        this->particleEmitterLeft->data.origin = vec4(
            vec3(position + (vec3(transformMat[0]) * -thrusterPosOffset)) + (
                vec3(transformMat[2]) * emitterOffset), 1);
        this->particleEmitterLeft->data.dir = vec4(vec3(-transformMat[2]), 0);
        this->particleEmitterRight->data.origin = vec4(
            vec3(position + (vec3(transformMat[0]) * thrusterPosOffset)) + (
                vec3(transformMat[2]) * emitterOffset), 1);
        this->particleEmitterRight->data.dir = vec4(vec3(-transformMat[2]), 0);

        const float speed = distance(position, prevPos) / 10.0f * dt;
        this->particleEmitterLeft->data.startSpeed = 1.2f + (3.0f * speed);
        this->particleEmitterLeft->data.endSpeed = 0.0f + (3.0f * speed);
        this->particleEmitterRight->data.startSpeed = 1.2f + (3.0f * speed);
        this->particleEmitterRight->data.endSpeed = 0.0f + (3.0f * speed);
        //this->particleEmitter->data.decayTime = 0.16f;//+ (0.01f  * t);
        //this->particleEmitter->data.randomTimeOffsetDist = 0.06f;/// +(0.01f * t);

        prevPos = position;
    }

    void
    SpaceShipState::Update(const float dt) {
        if (input.W()) {
            if (input.Shift())
                this->currentSpeed = mix(this->currentSpeed, this->boostSpeed, std::min(1.0f, dt * 30.0f));
            else
                this->currentSpeed = mix(this->currentSpeed, this->normalSpeed, std::min(1.0f, dt * 90.0f));
        } else {
            this->currentSpeed = 0;
        }

        vec3 desiredVelocity(0, 0, this->currentSpeed * 10.0f);
        desiredVelocity = transform.GetMatrix() * vec4(desiredVelocity, 0.0f);

        this->linearVelocity = mix(this->linearVelocity, desiredVelocity, dt * accelerationFactor);

        const float rotX = input.Left() ? 1.0f : input.Right() ? -1.0f : 0.0f;
        const float rotY = input.Up() ? -1.0f : input.Down() ? 1.0f : 0.0f;
        const float rotZ = input.A() ? -1.0f : input.D() ? 1.0f : 0.0f;

        transform.AddPosition(this->linearVelocity * dt);

        const float rotationSpeed = 1.8f * dt;
        rotXSmooth = mix(rotXSmooth, rotX * rotationSpeed, dt * smoothFactor);
        rotYSmooth = mix(rotYSmooth, rotY * rotationSpeed, dt * smoothFactor);
        rotZSmooth = mix(rotZSmooth, rotZ * rotationSpeed, dt * smoothFactor);
        quat localOrientation = quat(vec3(-rotYSmooth, rotXSmooth, rotZSmooth));
        this->rotationZ -= rotXSmooth;
        this->rotationZ = clamp(this->rotationZ, -45.0f, 45.0f);

        transform.SetOrientation(transform.GetOrientation() * localOrientation);
        //mat4 T = translate(this->position) * (mat4) this->orientation;
        transform.GetMatrix() *= mat4(quat(vec3(0, 0, rotationZ)));
        this->rotationZ = mix(this->rotationZ, 0.0f, dt * smoothFactor);
    }

    bool
    SpaceShipState::CheckCollisions() {
        const mat4 rotation(transform.GetOrientation());
        const vec3 position = transform.GetPosition();
        bool hit = false;
        for (int i = 0; i < 8; i++) {
            const vec3 dir = rotation * vec4(normalize(colliderEndPoints[i]), 0.0f);
            const float len = length(colliderEndPoints[i]);
            const Physics::RaycastPayload payload = Physics::Raycast(position, dir, len);

            // debug draw collision rays
            // Debug::DrawLine(pos, pos + dir * len, 1.0f, glm::vec4(0, 1, 0, 1), glm::vec4(0, 1, 0, 1), Debug::RenderMode::AlwaysOnTop);

            if (payload.hit) {
                Debug::DrawDebugText("HIT", payload.hitPoint, vec4(1, 1, 1, 1));
                hit = true;
            }
        }
        return hit;
    }
}
