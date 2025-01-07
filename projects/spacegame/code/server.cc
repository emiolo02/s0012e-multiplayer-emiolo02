#include "server.h"
#include "proto.h"
#include "packets.h"

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

        const uint64 timeStart = Time::Now();

        m_Server.Poll();

        CheckCollisions();

        for (auto &laser: m_Lasers) {
            if (Time::Now() >= laser.second.endTime) {
                m_LasersToRemove.push(laser.first);
            }
            laser.second.Update(dt);
        }

        for (auto &player: m_Players) {
            auto peer = player.first;
            auto &shipState = player.second;

            SetTransform(m_PlayerColliders[shipState.id], shipState.transform.GetMatrix());

            if (shipState.input.Space()) {
                shipState.input.mask &= 0b1101111111;
                SpawnLaser(shipState);
            }

            shipState.Update(dt);

            auto packedPlayer = PackPlayer(shipState);

            const auto fbb = Packet::UpdatePlayerS2C(Time::Now(), &packedPlayer);

            m_Server.BroadCast(fbb.GetBufferPointer(), fbb.GetSize());
        }

        RemoveLasers();

        dt = float(Time::Now() - timeStart) / 1000.0f;
    }

    void
    Server::ConnectImpl(const Net::Packet &packet) {
        LOG(IP_STREAM(packet.sender->address.host) << " connected to the server\n");
        const uint32 uuid = m_NextEntityId++;

        // Client connect

        auto fbb = Packet::ClientConnectS2C(uuid, Time::Now());

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
        m_Players[packet.sender].id = uuid;

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

                m_Players[packet.sender].input = {inputData->bitmap, inputData->time};
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
        const auto disconnectedPlayer = m_Players[packet.sender];

        m_PlayerColliders.erase(disconnectedPlayer.id);

        const auto fbb = Packet::DespawnPlayerS2C(disconnectedPlayer.id);
        m_Server.BroadCast(fbb.GetBufferPointer(), fbb.GetSize());

        m_Players.erase(packet.sender);
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

        const uint64 now = Time::Now();
        laser.startTime = now;
        laser.endTime = now + 10000;

        //FlatBufferBuilder builder;
        const auto packedLaser = PackLaser(laser);
        const auto fbb = Packet::SpawnLaserS2C(&packedLaser);
        m_Server.BroadCast(fbb.GetBufferPointer(), fbb.GetSize());
    }

    void
    Server::CheckCollisions() {
        for (auto &player: m_Players) {
            if (player.second.CheckCollisions()) {
                const auto fbb = Packet::DespawnPlayerS2C(player.second.id);
                m_Server.BroadCast(fbb.GetBufferPointer(), fbb.GetSize());

                SpawnPlayer(player.second.id);
            }
        }

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

                if (playerIt != m_PlayerColliders.end() && playerIt->first != laser.second.senderId) {
                    // Hit player
                    LOG("hit player\n");
                    for (auto &ship: m_Players) {
                        if (ship.second.id == playerIt->first) {
                            const auto fbb = Packet::DespawnPlayerS2C(ship.second.id);
                            m_Server.BroadCast(fbb.GetBufferPointer(), fbb.GetSize());

                            SpawnPlayer(ship.second.id);
                        }
                    }
                } else {
                    // Hit asteroid
                    LOG("hit asteroid\n");
                }

                m_LasersToRemove.push(laser.first);
            }
        }
    }

    void
    Server::RemoveLasers() {
        while (!m_LasersToRemove.empty()) {
            uint32 laserId = m_LasersToRemove.front();

            const auto fbb = Packet::DespawnLaserS2C(laserId);
            m_Server.BroadCast(fbb.GetBufferPointer(), fbb.GetSize());

            m_Lasers.erase(laserId);

            m_LasersToRemove.pop();
        }
    }

    void
    Server::SpawnPlayer(const uint32 id) {
        auto spawnPoint = m_SpawnPoints.begin();
        for (; spawnPoint != m_SpawnPoints.end(); ++spawnPoint) {
            if (spawnPoint->playerId == id) {
                break;
            }
        }

        auto ship = m_Players.begin();
        for (; ship != m_Players.end(); ++ship) {
            if (ship->second.id == id) {
                break;
            }
        }

        Transform transform;
        transform.SetPosition(spawnPoint->point);
        const vec3 dirToOrigin = normalize(-spawnPoint->point);
        transform.SetOrientation(quat(vec3(0.0f, 0.0f, 1.0f), dirToOrigin));

        if (!m_PlayerColliders.contains(id)) {
            m_PlayerColliders[id] = Physics::CreateCollider(m_ShipColliderMesh, transform.GetMatrix());
        } else {
            Physics::SetTransform(m_PlayerColliders[id], transform.GetMatrix());
        }
        ship->second.transform = transform;
        ship->second.linearVelocity = vec3();
        const auto player = PackPlayer(ship->second);
        const auto fbb = Packet::SpawnPlayerS2C(&player);
        m_Server.BroadCast(fbb.GetBufferPointer(), fbb.GetSize());
    }
}


