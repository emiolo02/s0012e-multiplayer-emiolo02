#include "server.h"
#include "network.h"

#include <bitset>

namespace Net {
    bool
    Server::Create(const uint16 port) {
        m_Address.host = ENET_HOST_ANY;
        m_Address.port = port;

        m_Server = enet_host_create(&m_Address, 32, 1, 0, 0);
        if (!m_Server) {
            std::cout << "Failed to create ENet client host.\n";
            return false;
        }

        LOG("Successfully created ENet server host.\n");
        m_Active = true;
        return true;
    }

    Server::~Server() {
        LOG("Destroyed server.\n");
        enet_host_destroy(m_Server);
    }

    void
    Server::Poll(const uint32 timeout) const {
        if (!m_Active) {
            return;
        }

        ENetEvent event;
        while (enet_host_service(m_Server, &event, timeout) > 0) {
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


    void
    Server::BroadCast(const uint8 *data, const size_t size) const {
        ENetPacket *packet = enet_packet_create(data, size, ENET_PACKET_FLAG_RELIABLE);
        enet_host_broadcast(m_Server, 0, packet);
    }

    void
    Server::Send(ENetPeer *peer, const uint8 *data, const size_t size) const {
        ENetPacket *packet = enet_packet_create(data, size, ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(peer, 0, packet);
    }
}
