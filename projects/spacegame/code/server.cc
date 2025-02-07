#include "server.h"
#include "proto.h"
#include "packets.h"
#include <thread>

using namespace flatbuffers;


namespace Game {
    Server Server::s_Instance;

    static Protocol::Player
    PackPlayer(const SpaceShipState &ship) {
        return {
            ship.id,
            *(Protocol::Vec3 *) &ship.transform.GetPosition(),
            *(Protocol::Vec3 *) &ship.linearVelocity,
            Protocol::Vec3(0, 0, 0),
            *(Protocol::Vec4 *) &ship.transform.GetOrientation()
        };
    }

    static Protocol::Laser
    PackLaser(const Laser &laser) {
        return {
            laser.id,
            laser.startTime,
            laser.startTime + 10000,
            *(Protocol::Vec3 *) &laser.origin,
            *(Protocol::Vec4 *) &laser.transform.GetOrientation()
        };
    }

    void
    Server::CreateImpl(const uint16 port) {
        m_Server.Create(port);
        m_Server.SetConnectCallback(Connect);
        m_Server.SetReceiveCallback(Receive);
        m_Server.SetDisconnectCallback(Disconnect);

        m_ShipColliderMesh = Physics::LoadColliderMesh("assets/space/spaceship_physics.glb");
        m_Active = true;

        // Generate spawn points

        for (int i = 0; i < 32; ++i) {
            constexpr float pi2 = M_PI * 2.0f;
            const float x = std::sin(pi2 * static_cast<float>(i) / 32.0f);
            const float y = std::cos(pi2 * static_cast<float>(i) / 32.0f);

            m_SpawnPoints[i].point = vec3(x, 0.0f, y) * 100.0f;
        }
    }


    void
    Server::UpdateImpl() {
        if (!m_Active) {
            return;
        }

        constexpr uint updateTime = 1000 / m_UpdateFrequency;
        constexpr float dt = 1.0f / static_cast<float>(m_UpdateFrequency);

        m_CurrentTime = Time::Now();

        m_Server.Poll(0);

        CheckCollisions();

        for (auto &laser: m_Lasers) {
            if (m_CurrentTime >= laser.second.endTime) {
                m_LasersToRemove.push(laser.first);
                m_DespawnLaserPackets.push(laser.first);
            }
            laser.second.Update(dt);
        }

        for (auto &player: m_Players) {
            auto peer = player.first;
            auto &shipState = player.second;


            if (shipState.input.Space()) {
                shipState.input.mask &= 0b1101111111;
                SpawnLaser(shipState);
            }

            shipState.Update(dt);
            SetTransform(m_PlayerColliders[shipState.id], shipState.transform.GetMatrix());
        }

        RemoveLasers();

        // Only publish state every 10 updates. (5 times / s)
        if (m_CurrentFrame % 10 == 0) {
            // Send packets.

            while (!m_SpawnPlayerPackets.empty()) {
                const auto fbb = Packet::SpawnPlayerS2C(&m_SpawnPlayerPackets.front());
                m_Server.BroadCast(fbb.GetBufferPointer(), fbb.GetSize());
                m_SpawnPlayerPackets.pop();
            }

            while (!m_DespawnPlayerPackets.empty()) {
                const auto fbb = Packet::DespawnPlayerS2C(m_DespawnPlayerPackets.front());
                m_Server.BroadCast(fbb.GetBufferPointer(), fbb.GetSize());
                m_DespawnPlayerPackets.pop();
            }

            while (!m_RespawnPlayerPackets.empty()) {
                const EntityId respawnId = m_RespawnPlayerPackets.front();
                const auto fbb = Packet::DespawnPlayerS2C(respawnId);
                m_Server.BroadCast(fbb.GetBufferPointer(), fbb.GetSize());

                m_RespawnPlayerPackets.pop();

                SpawnPlayer(respawnId);
            }

            while (!m_SpawnLaserPackets.empty()) {
                const auto fbb = Packet::SpawnLaserS2C(&m_SpawnLaserPackets.front());
                m_Server.BroadCast(fbb.GetBufferPointer(), fbb.GetSize());
                m_SpawnLaserPackets.pop();
            }

            while (!m_DespawnLaserPackets.empty()) {
                const auto fbb = Packet::DespawnLaserS2C(m_DespawnLaserPackets.front());
                m_Server.BroadCast(fbb.GetBufferPointer(), fbb.GetSize());
                m_DespawnLaserPackets.pop();
            }

            while (!m_CollisionPackets.empty()) {
                const auto &collision = m_CollisionPackets.front();
                const auto fbb = Packet::CollisionS2C(collision.first, collision.second);
                m_Server.BroadCast(fbb.GetBufferPointer(), fbb.GetSize());
                m_CollisionPackets.pop();
            }

            // Update players.
            for (const auto &player: m_Players) {
                auto packedPlayer = PackPlayer(player.second);
                const auto fbb = Packet::UpdatePlayerS2C(m_CurrentTime, &packedPlayer);
                m_Server.BroadCast(fbb.GetBufferPointer(), fbb.GetSize());
            }
        }

        while (Time::Now() - m_CurrentTime < updateTime) {
            // Wait.
        }
        m_CurrentFrame++;
    }

    void
    Server::ConnectImpl(const Net::Packet &packet) {
        LOG(IP_STREAM(packet.sender->address.host) << " connected to the server\n");
        const EntityId uuid = m_NextEntityId++;

        // Client connect
        m_Connections[packet.sender] = uuid;

        auto fbb = Packet::ClientConnectS2C(uuid, m_CurrentTime);

        m_Server.Send(packet.sender, fbb.GetBufferPointer(), fbb.GetSize());


        // Game state

        std::vector<Protocol::Player> playersVec;
        playersVec.reserve(m_Players.size());

        for (const auto &player: m_Players) {
            playersVec.push_back(PackPlayer(player.second));
        }

        std::vector<Protocol::Laser> laserVec;
        laserVec.reserve(m_Lasers.size());
        for (const auto &laser: m_Lasers) {
            laserVec.push_back(PackLaser(laser.second));
        }


        fbb.Clear();
        fbb = Packet::GameStateS2C(laserVec, playersVec);
        m_Server.Send(packet.sender, fbb.GetBufferPointer(), fbb.GetSize());

        fbb.Clear();


        // Spawn player

        // Claim spawn point
        for (auto &spawnPoint: m_SpawnPoints) {
            if (!spawnPoint.occupied) {
                spawnPoint.occupied = true;
                spawnPoint.playerId = uuid;
                break;
            }
        }

        SpawnPlayer(uuid);
    }

    void
    Server::ReceiveImpl(const Net::Packet &packet) {
        auto wrapper = Protocol::GetPacketWrapper(&packet.data[0])->UnPack()->packet;

        switch (wrapper.type) {
            /**
            A packet used to send input from client to server, should be called every time there is a
            change of input buttons.
            - time: The UNIX epoch time when the input was sent from the client.
            - bitmap: A bitmap of all input keys (16-bit), in binary 1 means pressed and 0 is released.
             */
            case Protocol::PacketType_InputC2S: {
                const auto inputData = wrapper.AsInputC2S();
                auto &player = m_Players[m_Connections[packet.sender]];

                // This check is to make sure a "shoot" command isn't overwritten.
                const uint16 mask = player.input.Space()
                                        ? inputData->bitmap | 0b0010000000
                                        : inputData->bitmap;

                player.input = {mask, inputData->time};
                break;
            }

            /**
            A packet used to send plaintext from client to server.
            - text: The text that will be sent.
             */
            case Protocol::PacketType_TextC2S: {
                const auto textC2S = wrapper.AsTextC2S();

                const auto fbb = Packet::TextS2C(textC2S->text);

                m_Server.BroadCast(fbb.GetBufferPointer(), fbb.GetSize());
                break;
            }

            default: break;
        }
    }

    void
    Server::DisconnectImpl(const Net::Packet &packet) {
        LOG("Player disconnected\n");
        const EntityId disconnectedId = m_Connections[packet.sender];

        m_PlayerColliders.erase(disconnectedId);

        const auto fbb = Packet::DespawnPlayerS2C(disconnectedId);
        m_Server.BroadCast(fbb.GetBufferPointer(), fbb.GetSize());

        m_Players.erase(disconnectedId);
        m_Connections.erase(packet.sender);
    }

    void
    Server::AddAsteroidImpl(const Physics::ColliderMeshId &colliderMesh, const mat4 &transform) {
        m_AsteroidColliders.push_back(Physics::CreateCollider(colliderMesh, transform));
        m_NextEntityId++;
    }

    void
    Server::SpawnLaser(const SpaceShipState &shipState) {
        const uint32 uuid = m_NextEntityId++;

        m_Lasers.emplace(uuid, shipState.transform);

        auto &laser = m_Lasers.at(uuid);
        laser.id = uuid;
        laser.senderId = shipState.id;

        const uint64 now = m_CurrentTime;
        laser.startTime = now;
        laser.endTime = now + 10000;

        m_SpawnLaserPackets.push(PackLaser(laser));
        //FlatBufferBuilder builder;
        //const auto packedLaser = PackLaser(laser);
        //const auto fbb = Packet::SpawnLaserS2C(&packedLaser);
        //m_Server.BroadCast(fbb.GetBufferPointer(), fbb.GetSize());
    }

    void
    Server::CheckCollisions() {
        // Player vs player & player vs asteroid collision
        for (auto &player: m_Players) {
            if (player.second.CheckCollisions()) {
                m_RespawnPlayerPackets.push(player.first);
            }
        }

        // Laser collisions
        for (auto &laser: m_Lasers) {
            Transform &laserTransform = laser.second.transform;
            const vec3 rayStart = laserTransform.GetPosition() - laser.second.direction * 0.5f;
            //Debug::DrawLine(rayStart, rayStart + laser.second.direction, 2, vec4(0, 1, 0, 1), vec4(0, 1, 0, 1),
            //                Debug::AlwaysOnTop);

            Physics::RaycastPayload payload = Physics::Raycast(rayStart, laser.second.direction, 1.0f);

            if (payload.hit) {
                auto playerIt = m_PlayerColliders.begin();
                for (; playerIt != m_PlayerColliders.end(); ++playerIt) {
                    if (playerIt->second == payload.collider)
                        break;
                }
                if (playerIt == m_PlayerColliders.end()) {
                    // hit asteroid
                } else {
                    // hit player
                    if (laser.second.senderId == playerIt->first) {
                        continue; // ignore collisions with sender.
                    }
                    m_CollisionPackets.push({laser.first, playerIt->first});
                    m_RespawnPlayerPackets.push(playerIt->first);
                }

                //if (m_Players.contains(playerIt->first))
                //if (playerIt != m_PlayerColliders.end() && playerIt->first != laser.second.senderId) {
                //    // Hit player
                //    for (auto &ship: m_Players) {
                //        if (ship.second.id == playerIt->first) {
                //            SpawnPlayer(ship.second.id);
                //        }
                //    }
                //} else {
                //    // Hit asteroid
                //}

                m_LasersToRemove.push(laser.first);
            }
        }

        //while (!playersToRespawn.empty()) {
        //    const uint32 id = playersToRespawn.front();
        //    playersToRespawn.pop();

        //    const auto fbb = Packet::DespawnPlayerS2C(id);
        //    m_Server.BroadCast(fbb.GetBufferPointer(), fbb.GetSize());

        //    SpawnPlayer(id);
        //}
    }

    void
    Server::RemoveLasers() {
        while (!m_LasersToRemove.empty()) {
            uint32 laserId = m_LasersToRemove.front();
            m_Lasers.erase(laserId);
            m_DespawnLaserPackets.push(laserId);

            m_LasersToRemove.pop();
        }
    }

    void
    Server::SpawnPlayer(const EntityId id) {
        auto spawnPoint = m_SpawnPoints.begin();
        for (; spawnPoint != m_SpawnPoints.end(); ++spawnPoint) {
            if (spawnPoint->playerId == id) {
                break;
            }
        }

        auto &ship = m_Players[id];

        ship.id = id;
        Transform transform;
        transform.SetPosition(spawnPoint->point);
        const vec3 dirToOrigin = normalize(-spawnPoint->point);
        transform.SetOrientation(quat(vec3(0.0f, 0.0f, 1.0f), dirToOrigin));

        if (!m_PlayerColliders.contains(id)) {
            m_PlayerColliders[id] = Physics::CreateCollider(m_ShipColliderMesh, transform.GetMatrix());
        } else {
            Physics::SetTransform(m_PlayerColliders[id], transform.GetMatrix());
        }
        ship.transform = transform;
        ship.linearVelocity = vec3();
        m_SpawnPlayerPackets.push(PackPlayer(ship));
    }
}


