#pragma once
#include "packets.h"
#include "network/network.h"
#include "spaceship.h"

namespace Game {
    class Client {
    public:
        Client() = default;

        static void Send(const uint8 *data, const size_t size) { s_Instance.m_Client.SendPacket(data, size); }

        static void Update() { s_Instance.UpdateImpl(); }

        static void Connect(const char *ip, uint16 port);

        static void Disconnect();

        static const char *Status();

        static uint32 GetId() { return s_Instance.m_ClientId; }

        static void SetScene(std::unordered_map<uint32, SpaceShip> *spaceShips,
                             std::unordered_map<uint32, Laser> *lasers) {
            s_Instance.m_SpaceShips = spaceShips;
            s_Instance.m_Lasers = lasers;
        }

    private:
        void UpdateImpl();

        static void Receive(const Net::Packet &packet) { s_Instance.ReceiveImpl(packet); }

        void ReceiveImpl(const Net::Packet &packet);

        static Client s_Instance;

        bool m_Active = false;

        std::unordered_map<uint32, SpaceShip> *m_SpaceShips = nullptr;
        std::unordered_map<uint32, Laser> *m_Lasers = nullptr;
        uint32 m_ClientId = 0;
        Net::Client m_Client;

        uint64 m_CurrentTime = 0;
        uint64 m_LastUpdateTime = 0;
    };
}
