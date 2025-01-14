#include "deadreckoning.h"

#include "detail/_noise.hpp"
#include "detail/_noise.hpp"
#include "detail/_noise.hpp"
#include "detail/_noise.hpp"

void
DeadReckoning::Interpolate(Transform &shipTransform, const float dt) {
    m_TimeSinceUpdate += dt;
    Protocol::Vec3 &serverPosition = m_ServerData.state.mutable_position();
    Protocol::Vec3 serverVelocity = m_ServerData.state.velocity();
    Protocol::Vec3 serverAcceleration = m_ServerData.state.acceleration();
    quat serverOrientation = *(quat *) &m_ServerData.state.direction();

    const float sinceUpdateSq = m_TimeSinceUpdate * m_TimeSinceUpdate;

    shipTransform.SetPosition({
        serverPosition.x() + serverVelocity.x() * m_TimeSinceUpdate + serverAcceleration.x() * sinceUpdateSq / 2.0f,
        serverPosition.y() + serverVelocity.y() * m_TimeSinceUpdate + serverAcceleration.y() * sinceUpdateSq / 2.0f,
        serverPosition.z() + serverVelocity.z() * m_TimeSinceUpdate + serverAcceleration.z() * sinceUpdateSq / 2.0f
    });
    //m_Transform.SetPosition(*(vec3 *) &serverPosition);
    shipTransform.SetOrientation(serverOrientation);
}

void
DeadReckoning::SetServerData(Transform &shipTransform, const Protocol::Player &state, const uint64 time) {
    m_ServerData = {state, time};
    m_TimeSinceUpdate = 0.0f;

    shipTransform.SetPosition(*(vec3 *) &state.position());
    shipTransform.SetOrientation(*(quat *) &state.direction());
}
