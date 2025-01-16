#include "client.h"
#include "proto.h"

using namespace flatbuffers;

namespace Game {
    Client Client::s_Instance;

    void
    Client::Connect(const char *ip, const uint16 port) {
        if (!s_Instance.m_Active) {
            s_Instance.m_Client.Create();
            s_Instance.m_Active = true;

            s_Instance.m_Client.SetReceiveCallback(Receive);
        }
        s_Instance.m_Client.Connect(ip, port);
    }

    void
    Client::Disconnect() {
        s_Instance.m_Client.Disconnect();
    }

    const char *
    Client::Status() {
        return s_Instance.m_Client.Status();
    }

    void
    Client::ReceiveImpl(const Net::Packet &packet) {
        const auto wrapper = Protocol::GetPacketWrapper(&packet.data[0])->UnPack()->packet;

        switch (wrapper.type) {
            /**
            A packet used to notify the client about what ship it will be controlling when it receives
            the initial game state.
            - uuid: Unique identifier of the player that will be controlled by the client.
            - time: The server time when this packet was sent, add the packet transmit time to this
              to get accurate server time on the client (use this to find difference).
             */
            case Protocol::PacketType_ClientConnectS2C: {
                LOG("Received connect packet\n");
                const auto clientConnectS2C = wrapper.AsClientConnectS2C();
                m_ClientId = clientConnectS2C->uuid;
                LOG("Received connect packet. uuid is: " << clientConnectS2C->uuid << '\n');
                break;
            }

            /**
            A packet used to communicate the current game state to a client, used when a player
            connects to the server.
            - players: List of all players on the server at the point of connection.
            - lasers: List of all lasers that exist in the world at point of connection.
             */
            case Protocol::PacketType_GameStateS2C: {
                LOG("Receive 'GameState' packet\n");
                auto gameState = wrapper.AsGameStateS2C();
                for (const auto &player: gameState->players) {
                    m_SpaceShips->emplace(player.uuid(), SpaceShip());
                    SpaceShip &ship = m_SpaceShips->at(player.uuid());
                    ship.id = player.uuid();
                    ship.transform.SetPosition(*(vec3 *) &player.position());
                    ship.transform.SetOrientation(*(quat *) &player.direction());
                    ship.Init();
                }
                //for (auto &player: gameState->players) {
                //    Component *components[] = {
                //        new Game::SpaceShipActor(),
                //        new StaticMesh("assets/space/spaceship.glb")
                //    };
                //    GameObject *gameObject = m_Scene->CreateGameObject(player.uuid(), components, 2);
                //    gameObject->SetPosition(*(vec3 *) &player.position());
                //    gameObject->SetOrientation(*(quat *) &player.direction());
                //}
                break;
            }

            /**
            A packet used to spawn a player, used when a new player enters the game or an
            existing respawns.
            - player: The player that will be spawned, internal player information decides who and
              where.
             */
            case Protocol::PacketType_SpawnPlayerS2C: {
                LOG("Receive 'SpawnPlayer' packet\n");
                auto &player = wrapper.AsSpawnPlayerS2C()->player;
                //m_SpaceShips->at(player->uuid()) = SpaceShip();
                m_SpaceShips->emplace(player->uuid(), SpaceShip());
                SpaceShip &ship = m_SpaceShips->at(player->uuid());
                ship.id = player->uuid();
                ship.transform.SetPosition(*(vec3 *) &player->position());
                ship.transform.SetOrientation(*(quat *) &player->direction());
                ship.Init();

                break;
            }

            /**
            A packet used to despawn a player, used when a player disconnects or an existing dies.
            - uuid: Unique identifier of the player that will be despawned.
             */
            case Protocol::PacketType_DespawnPlayerS2C: {
                LOG("Received 'DespawnPlayer' packet\n");
                const uint32 despawnedId = wrapper.AsDespawnPlayerS2C()->uuid;
                m_SpaceShips->erase(despawnedId);
                break;
            }

            /**
            A packet used to update player movement information, used when the server has
            decided on where the player is in the world every n-th tick.
            - time: The UNIX epoch time when the player information was sent from the server.
            - player: The player that will be updated, internal player information decides who and
              where.
             */
            case Protocol::PacketType_UpdatePlayerS2C: {
                const auto updatePlayer = wrapper.AsUpdatePlayerS2C();
                auto &player = updatePlayer->player;
                SpaceShip &ship = m_SpaceShips->at(player->uuid());
                ship.predictedBody.SetServerData(ship.transform, *player, updatePlayer->time);
                break;
            }

            /**
            A packet used to teleport the player, no dead reckoning should be performed on this
            information. Sent when server cannot smoothly lerp between points.
            - time: The UNIX epoch time when the player information was sent from the server.
            - player: The player that will be teleported, internal player information decides who and
              where.
             */
            case Protocol::PacketType_TeleportPlayerS2C: {
                break;
            }

            /**
            A packet used to spawn a laser, used when a player shoots.
            - laser: The laser that will be spawned, internal laser information decides how and
              where.
             */
            case Protocol::PacketType_SpawnLaserS2C: {
                LOG("Receive spawn laser packet\n");
                auto &laserPacket = wrapper.AsSpawnLaserS2C()->laser;
                const uint32 uuid = laserPacket->uuid();

                Transform laserTransform;
                laserTransform.SetPosition(*(vec3 *) &laserPacket->origin());
                laserTransform.SetOrientation(*(quat *) &laserPacket->direction());
                m_Lasers->emplace(uuid, laserTransform);

                auto &laser = m_Lasers->at(uuid);
                laser.id = uuid;
                laser.startTime = laserPacket->start_time();
                laser.endTime = laserPacket->end_time();

                // Sync the laser with the server
                const float dt = float(Time::Now() - laserPacket->start_time()) / 1000.0f;
                laser.Update(dt);
                break;
            }

            /**
            A packet used to despawn a laser, used when a laser dies.
            - uuid: Unique identifier of the laser that will be despawned.
             */
            case Protocol::PacketType_DespawnLaserS2C: {
                const auto despawnLaser = wrapper.AsDespawnLaserS2C();
                m_Lasers->erase(despawnLaser->uuid);
                LOG("despawned laser with id " << despawnLaser->uuid << '\n');
                break;
            }

            /**
            A packet used to notify clients of a server decision where two entities collided, could be
            used to keep track of lives.
            - uuid_first: Unique identifier of the first entity involved in the collision.
            - uuid_second: Unique identifier of the second entity involved in the collision.
            */
            case Protocol::PacketType_CollisionS2C: {
                break;
            }

            /**
            A packet used to send plaintext from server to clients.
            - text: The text that will be sent.
             */
            case Protocol::PacketType_TextS2C: {
                auto textS2C = wrapper.AsTextS2C();
                LOG("Client received text: " << textS2C->text << '\n');
                break;
            }
            default: break;
        }
    }
}
