#pragma once
#include "network/network.h"
#include <unordered_map>

#include "spaceship.h"
#include "render/physics.h"


namespace Game {
    struct SpawnPoint {
        uint32 playerId = 0;
        vec3 point = vec3();
        bool occupied = false;
    };

    class Server {
    public:
        static void Create(const uint16 port) { s_Instance.CreateImpl(port); }

        static void Update() { s_Instance.UpdateImpl(); }

        static void AddAsteroid(const Physics::ColliderMeshId &colliderMesh, const mat4 &transform) {
            s_Instance.AddAsteroidImpl(colliderMesh, transform);
        }

    private:
        void CreateImpl(uint16 port);

        void UpdateImpl();

        static void Connect(const Net::Packet &packet) { s_Instance.ConnectImpl(packet); }
        static void Receive(const Net::Packet &packet) { s_Instance.ReceiveImpl(packet); }
        static void Disconnect(const Net::Packet &packet) { s_Instance.DisconnectImpl(packet); }

        void ConnectImpl(const Net::Packet &packet);

        void ReceiveImpl(const Net::Packet &packet);

        void DisconnectImpl(const Net::Packet &packet);

        void AddAsteroidImpl(const Physics::ColliderMeshId &colliderMesh, const mat4 &transform);

        void SpawnLaser(const SpaceShipState &shipState);

        void CheckCollisions();

        void RemoveLasers();

        void SpawnPlayer(uint32 id);

        bool m_Active = false;

        float dt = 0.01667f;

        std::unordered_map<const ENetPeer *, SpaceShipState> m_Players;
        Physics::ColliderMeshId m_ShipColliderMesh = {};
        std::unordered_map<uint32, Physics::ColliderId> m_PlayerColliders;

        std::unordered_map<uint32, Laser> m_Lasers;
        std::queue<uint32> m_LasersToRemove;

        std::vector<Physics::ColliderId> m_AsteroidColliders;

        std::array<SpawnPoint, 32> m_SpawnPoints;

        static Server s_Instance;

        Net::Server m_Server;
        uint32 m_NextEntityId = 0;
    };
}
