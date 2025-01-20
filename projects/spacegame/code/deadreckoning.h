#pragma once

#include "proto.h"
#include "transform.h"

class DeadReckoning {
public:
    void SetServerData(Transform &shipTransform, const Protocol::Player &state, uint64 serverTime);

    void Interpolate(Transform &shipTransform, float dt);

private:
    struct ServerData {
        Protocol::Player state = {};
        uint64 time = 0;
        uint64 latency = 0;
    };

    ServerData m_ServerData, m_LastServerData;

    float m_TimeSinceUpdate = 0.0f;
};
