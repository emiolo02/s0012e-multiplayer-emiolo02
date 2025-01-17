#include "deadreckoning.h"

#include "render/debugrender.h"

void
DeadReckoning::Interpolate(Transform &shipTransform, const float dt) {
    m_TimeSinceUpdate += dt;

    const vec3 serverPosition = *(vec3 *) &m_ServerData.state.position();
    const vec3 serverVelocity = *(vec3 *) &m_ServerData.state.velocity();
    const vec3 serverAcceleration = *(vec3 *) &m_ServerData.state.acceleration();
    const quat serverOrientation = *(quat *) &m_ServerData.state.direction();

    const vec3 lastServerPosition = *(vec3 *) &m_LastServerData.state.position();
    const vec3 lastServerVelocity = *(vec3 *) &m_LastServerData.state.velocity();
    const vec3 lastServerAcceleration = *(vec3 *) &m_LastServerData.state.acceleration();
    const quat lastServerOrientation = *(quat *) &m_LastServerData.state.direction();

    const float latency = static_cast<float>(m_ServerData.latency) / 1000.0f;
    const float normalT = clamp(m_TimeSinceUpdate / latency, 0.0f, 1.0f);
    LOG(normalT << '\n');

    const float halfTimeSinceUpdateSq = m_TimeSinceUpdate * m_TimeSinceUpdate * 0.5f;

    const vec3 blendVelocity = mix(lastServerVelocity, serverVelocity, normalT);
    const vec3 prevProjection = lastServerPosition
                                + blendVelocity * m_TimeSinceUpdate
                                + lastServerAcceleration * halfTimeSinceUpdateSq;

    const vec3 latestProjection = serverPosition
                                  + serverVelocity * m_TimeSinceUpdate
                                  + serverAcceleration * halfTimeSinceUpdateSq;

    shipTransform.SetPosition(mix(prevProjection, latestProjection, normalT));

    shipTransform.SetOrientation(mix(lastServerOrientation, serverOrientation, normalT));

    //Debug::DrawDebugText("#######", serverPosition, vec4(1.0f, 1.0f, 1.0f, 1.0f));
    //LOG(glm::to_string(*(vec3*)&serverVelocity) << '\n');
    //LOG(m_TimeSinceUpdate << '\n');
}

void
DeadReckoning::SetServerData(Transform &shipTransform, const Protocol::Player &state, const uint64 time) {
    //LOG("Set server data.\n");
    m_LastServerData = m_ServerData;
    m_ServerData = {state, time};
    m_TimeSinceUpdate = 0.0f;

    shipTransform.SetPosition(*(vec3 *) &state.position());
    shipTransform.SetOrientation(*(quat *) &state.direction());
}
