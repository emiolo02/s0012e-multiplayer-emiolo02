#pragma once
#include "enet/enet.h"
#include <functional>

namespace Net {
    struct Sender;
    struct Packet;

    class Client {
    public:
        Client() = default;

        ~Client();

        bool Create();

        bool Connect(const char *ip, uint16 port);

        const char *Status() const;

        void Disconnect() const;

        void SendPacket(const void *data, size_t size) const;

        void Poll() const;


        void SetConnectCallback(const std::function<void(const Packet &)> &func) { m_ConnectCallback = func; }
        void SetReceiveCallback(const std::function<void(const Packet &)> &func) { m_ReceiveCallback = func; }
        void SetDisconnectCallback(const std::function<void(const Packet &)> &func) { m_DisconnectCallback = func; }

    private:
        std::function<void(const Packet &)> m_ConnectCallback;
        std::function<void(const Packet &)> m_ReceiveCallback;
        std::function<void(const Packet &)> m_DisconnectCallback;

        ENetHost *m_Client = nullptr;
        ENetPeer *m_Peer = nullptr;
        ENetAddress m_Address = {};

        bool m_Active = false;

        static const char *s_StatusMsg[10];
    };
}
