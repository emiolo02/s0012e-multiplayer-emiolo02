#pragma once
#include "network/network.h"
#include "proto.h"

namespace Packet {
    // Server to client.
    flatbuffers::FlatBufferBuilder ClientConnectS2C(uint32 uuid, uint64 timeMs);

    flatbuffers::FlatBufferBuilder GameStateS2C(const std::vector<Protocol::Laser> &lasers,
                                                const std::vector<Protocol::Player> &players);

    flatbuffers::FlatBufferBuilder SpawnPlayerS2C(const Protocol::Player *player);

    flatbuffers::FlatBufferBuilder DespawnPlayerS2C(uint32 uuid);

    flatbuffers::FlatBufferBuilder UpdatePlayerS2C(uint64 timeMs, const Protocol::Player *player);

    flatbuffers::FlatBufferBuilder TeleportPlayerS2C(uint64 timeMs, const Protocol::Player *player);

    flatbuffers::FlatBufferBuilder SpawnLaserS2C(const Protocol::Laser *laser);

    flatbuffers::FlatBufferBuilder DespawnLaserS2C(uint32 uuid);

    flatbuffers::FlatBufferBuilder CollisionS2C(uint32 uuidA, uint32 uuidB);

    flatbuffers::FlatBufferBuilder TextS2C(const std::string &text);

    // Client to server.
    flatbuffers::FlatBufferBuilder InputC2S(uint64 timeMs, uint16 bitmap);

    flatbuffers::FlatBufferBuilder TextC2S(const std::string &text);
}
