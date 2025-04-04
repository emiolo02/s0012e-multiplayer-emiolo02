#pragma once
#include "network/network.h"
#include <unordered_map>

#include "spaceship.h"
#include "render/physics.h"


namespace Game {
    typedef uint32 EntityId;

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

        void SpawnPlayer(EntityId id);

        bool m_Active = false;

        static constexpr uint m_UpdateFrequency = 50;

        std::unordered_map<const ENetPeer *, EntityId> m_Connections;
        std::unordered_map<EntityId, SpaceShipState> m_Players;
        Physics::ColliderMeshId m_ShipColliderMesh = {};
        std::unordered_map<EntityId, Physics::ColliderId> m_PlayerColliders;

        std::unordered_map<EntityId, Laser> m_Lasers;
        std::queue<EntityId> m_LasersToRemove;

        std::vector<Physics::ColliderId> m_AsteroidColliders;

        std::array<SpawnPoint, 32> m_SpawnPoints;

        static Server s_Instance;

        Net::Server m_Server;
        EntityId m_NextEntityId = 0;

        uint64 m_CurrentTime = 0;

        uint32 m_CurrentFrame = 0;

        std::queue<Protocol::Player> m_SpawnPlayerPackets;
        std::queue<EntityId> m_DespawnPlayerPackets;
        std::queue<EntityId> m_RespawnPlayerPackets;

        std::queue<Protocol::Laser> m_SpawnLaserPackets;
        std::queue<EntityId> m_DespawnLaserPackets;

        std::queue<std::pair<EntityId, EntityId> > m_CollisionPackets;
    };
}
