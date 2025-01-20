#include "deadreckoning.h"

#include "render/debugrender.h"

void
DeadReckoning::Interpolate(Transform &shipTransform, const float dt) {
    const float latency = static_cast<float>(m_ServerData.latency) / 1000.0f;
    const float normalT = m_TimeSinceUpdate / latency;
    m_TimeSinceUpdate = m_TimeSinceUpdate >= latency ? latency : m_TimeSinceUpdate + dt;

    const vec3 serverPosition = *(vec3 *) &m_ServerData.state.position();
    const vec3 serverVelocity = *(vec3 *) &m_ServerData.state.velocity();
    const vec3 serverAcceleration = *(vec3 *) &m_ServerData.state.acceleration();
    const quat serverOrientation = *(quat *) &m_ServerData.state.direction();

    const vec3 lastServerPosition = *(vec3 *) &m_LastServerData.state.position();
    const vec3 lastServerVelocity = *(vec3 *) &m_LastServerData.state.velocity();
    const vec3 lastServerAcceleration = *(vec3 *) &m_LastServerData.state.acceleration();
    const quat lastServerOrientation = *(quat *) &m_LastServerData.state.direction();

    //LOG(normalT << '\n');
    LOG(latency << '\n');

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
DeadReckoning::SetServerData(Transform &shipTransform, const Protocol::Player &state, const uint64 serverTime) {
    //LOG("Set server data.\n");
    //if (m_ServerData.time == serverTime) {
    //    return;
    //}

    m_LastServerData = m_ServerData;
    m_ServerData = {state, serverTime, Time::Now() - serverTime};
    //LOG(serverTime << '\n');
    m_TimeSinceUpdate = 0.0f;

    //shipTransform.SetPosition(*(vec3 *) &state.position());
    //shipTransform.SetOrientation(*(quat *) &state.direction());
}
