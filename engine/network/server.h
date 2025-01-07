#pragma once
#include "enet/enet.h"
#include <functional>
#include <queue>

namespace Net {
    struct Sender;
    struct Packet;

    class Server {
    public:
        Server() = default;

        ~Server();

        bool Create(uint16 port);

        void Poll();

        void BroadCast(const uint8 *data, size_t size) const;

        void Send(ENetPeer *peer, const uint8 *data, size_t size) const;

        void SetConnectCallback(const std::function<void(const Packet &)> &func) { m_ConnectCallback = func; }
        void SetReceiveCallback(const std::function<void(const Packet &)> &func) { m_ReceiveCallback = func; }
        void SetDisconnectCallback(const std::function<void(const Packet &)> &func) { m_DisconnectCallback = func; }

    private:
        std::function<void(const Packet &)> m_ConnectCallback;
        std::function<void(const Packet &)> m_ReceiveCallback;
        std::function<void(const Packet &)> m_DisconnectCallback;

        ENetHost *m_Server = nullptr;
        ENetPeer *m_Peer = nullptr;
        ENetAddress m_Address = {};

        bool m_Active = false;
    };
}
