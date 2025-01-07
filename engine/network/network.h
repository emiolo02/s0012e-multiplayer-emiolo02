#pragma once
#include "server.h"
#include "client.h"

namespace Net {
#define IP_STREAM(IP) (IP & 0xFF) << '.' << ((IP >> 8) & 0xFF) << '.' << ((IP >> 16) & 0xFF) << '.' << ((IP >> 24) & 0xFF)

    struct Sender {
        uint32 ip = 0;
        uint16 port = 0;

        bool operator==(const Sender &rhs) const {
            return ip == rhs.ip && port == rhs.port;
        }
    };

    struct Packet {
        ENetPeer *sender;
        std::vector<uint8> data;

        Packet(ENetPeer *peer)
            : sender(peer) {
        }

        Packet(ENetPeer *peer, const uint8 *data, const size_t size)
            : sender(peer),
              data(data, data + size) {
        }
    };

    void Initialize();
}
