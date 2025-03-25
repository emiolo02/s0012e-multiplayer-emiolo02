#pragma once
#include "keymap.h"
#include "proto.h"
#include "transform.h"
#include "deadreckoning.h"
#include "render/physics.h"

namespace Render {
    struct ParticleEmitter;
}

namespace Game {
    struct Laser {
        explicit Laser(const Transform &transform);

        void Update(float dt);

        uint64 startTime = 0;
        uint64 endTime = 0;

        uint32 id = 0;
        uint32 senderId = 0;
        Transform transform;
        vec3 origin;
        vec3 direction;
        const float speed = 50.0f;
    };

    struct SpaceShipCamera {
        void Update(float dt);

        const float camOffsetY = 1.0f;
        const float cameraSmoothFactor = 10.0f;
        mat4 target = identity<mat4>();
        vec3 position = vec3(0, 1.0f, -2.0f);
    };

    // The in game representation of the space ship
    struct SpaceShip {
        SpaceShip() = default;

        SpaceShip(uint32 uuid);

        ~SpaceShip();

        void Init();

        void Update(float dt);

        void UserUpdate(KeyMap input, float dt);

        void Interpolate(float dt);

        void SetServerData(const Protocol::Player &data, uint64 time, bool reset);

        uint32 id = 0;
        Transform transform;
        vec3 velocity = vec3();


        bool init = false;

        float emitterOffset = -0.5f;
        //Render::ParticleEmitter *particleEmitterLeft = nullptr;
        //Render::ParticleEmitter *particleEmitterRight = nullptr;

    private:
        float timeSinceUpdate = 0.0f;
        uint64 lastServerUpdate = 0;
        uint64 currentServerUpdate = 0;

        Transform transformStart;
        vec3 velocityStart = vec3();
        Protocol::Player serverState;


        const float normalSpeed = 1.0f;
        const float boostSpeed = normalSpeed * 2.0f;
        float currentSpeed = 0;
        float rotationZ = 0;
        float rotXSmooth = 0;
        float rotYSmooth = 0;
        float rotZSmooth = 0;
    };

    // Server side representation of the space ship
    struct SpaceShipState {
        uint32 id;
        KeyMap input;

        Transform transform;
        vec3 linearVelocity = vec3(0);

        const float normalSpeed = 1.0f;
        const float boostSpeed = normalSpeed * 2.0f;
        const float accelerationFactor = 1.0f;
        const float smoothFactor = 10.0f;

        float currentSpeed = 0.0f;

        float rotationZ = 0;
        float rotXSmooth = 0;
        float rotYSmooth = 0;
        float rotZSmooth = 0;


        void Update(float dt);

        bool CheckCollisions();

        const vec3 colliderEndPoints[8] = {
            vec3(-1.10657, -0.480347, -0.346542), // right wing
            vec3(1.10657, -0.480347, -0.346542), // left wing
            vec3(-0.342382, 0.25109, -0.010299), // right top
            vec3(0.342382, 0.25109, -0.010299), // left top
            vec3(-0.285614, -0.10917, 0.869609), // right front
            vec3(0.285614, -0.10917, 0.869609), // left front
            vec3(-0.279064, -0.10917, -0.98846), // right back
            vec3(0.279064, -0.10917, -0.98846) // right back
        };
    };
}
