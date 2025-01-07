#include "client.h"

#include "network.h"

namespace Net {
    Client::~Client() {
        LOG("Destroyed client.\n");
        Disconnect();
        enet_host_destroy(m_Client);
    }

    bool Client::Create() {
        m_Client = enet_host_create(nullptr, 1, 1, 0, 0);
        if (!m_Client) {
            LOG("Failed to create ENet client host.\n");
            return false;
        }

        LOG("Successfully created ENet client host.\n");
        m_Active = true;
        return true;
    }

    bool
    Client::Connect(const char *ip, const uint16 port) {
        if (!m_Active) {
            LOG("Tried to connect client before client was created.\n");
            return false;
        }

        enet_address_set_host(&m_Address, ip);
        m_Address.port = port;
        LOG("Connecting to " << IP_STREAM(m_Address.host) << ':' << port << '\n');

        m_Peer = enet_host_connect(m_Client, &m_Address, 1, 0);
        if (!m_Peer) {
            LOG("No available peers for initializing connection.\n");
            return false;
        }
        return true;
    }

    const char *
    Client::Status() const {
        if (m_Peer != nullptr) {
            return s_StatusMsg[m_Peer->state];
        }
        return "";
    }

    const char *Client::s_StatusMsg[10] = {
        "Disconnected",
        "Connecting",
        "Acknowledging connect",
        "Connection pending",
        "Connection succeeded",
        "Connected",
        "Disconnect later",
        "Disconnecting",
        "Acknowledging disconnect",
        "Zombie"
    };

    void
    Client::Disconnect() const {
        if (m_Peer != nullptr) {
            enet_peer_disconnect(m_Peer, 0);
        }
    }

    void
    Client::SendPacket(const void *data, const size_t size) const {
        if (m_Peer == nullptr) {
            return;
        }

        ENetPacket *packet = enet_packet_create(data, size, ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(m_Peer, 0, packet);
    }

    void
    Client::Poll() const {
        if (!m_Active) {
            return;
        }

        ENetEvent event;

        while (enet_host_service(m_Client, &event, 0) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_CONNECT: {
                    if (m_ConnectCallback) {
                        m_ConnectCallback(Packet(event.peer));
                    }
                    break;
                }
                case ENET_EVENT_TYPE_RECEIVE: {
                    if (m_ReceiveCallback) {
                        m_ReceiveCallback(Packet(event.peer, event.packet->data, event.packet->dataLength));
                    }

                    enet_packet_destroy(event.packet);
                    break;
                }
                case ENET_EVENT_TYPE_DISCONNECT: {
                    if (m_DisconnectCallback) {
                        m_DisconnectCallback(Packet(event.peer));
                    }

                    enet_peer_reset(event.peer);
                    break;
                }
                default: ;
            }
        }
    }
}
