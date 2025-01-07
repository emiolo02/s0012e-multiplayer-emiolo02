#include "packets.h"

using namespace flatbuffers;
using namespace Protocol;

namespace Packet {
    FlatBufferBuilder
    ClientConnectS2C(const uint32 uuid, const uint64 timeMs) {
        FlatBufferBuilder fbb;
        const auto clientConnect = CreateClientConnectS2C(fbb, uuid, timeMs);
        const auto wrapper = CreatePacketWrapper(fbb, PacketType_ClientConnectS2C, clientConnect.Union());
        fbb.Finish(wrapper);
        return fbb;
    }

    FlatBufferBuilder
    GameStateS2C(const std::vector<Laser> &lasers, const std::vector<Player> &players) {
        FlatBufferBuilder fbb;
        const auto gameState = CreateGameStateS2CDirect(fbb, &players, &lasers);
        const auto wrapper = CreatePacketWrapper(fbb, PacketType_GameStateS2C, gameState.Union());
        fbb.Finish(wrapper);
        return fbb;
    }

    FlatBufferBuilder
    SpawnPlayerS2C(const Player *player) {
        FlatBufferBuilder fbb;
        const auto spawnPlayer = CreateSpawnPlayerS2C(fbb, player);
        const auto wrapper = CreatePacketWrapper(fbb, PacketType_SpawnPlayerS2C, spawnPlayer.Union());
        fbb.Finish(wrapper);
        return fbb;
    }

    FlatBufferBuilder DespawnPlayerS2C(const uint32 uuid) {
        FlatBufferBuilder fbb;
        const auto despawnPlayer = CreateDespawnPlayerS2C(fbb, uuid);
        const auto wrapper = CreatePacketWrapper(fbb, PacketType_DespawnPlayerS2C, despawnPlayer.Union());
        fbb.Finish(wrapper);
        return fbb;
    }

    FlatBufferBuilder UpdatePlayerS2C(const uint64 timeMs, const Player *player) {
        FlatBufferBuilder fbb;
        const auto updatePlayer = CreateUpdatePlayerS2C(fbb, timeMs, player);
        const auto wrapper = CreatePacketWrapper(fbb, PacketType_UpdatePlayerS2C, updatePlayer.Union());
        fbb.Finish(wrapper);
        return fbb;
    }

    FlatBufferBuilder TeleportPlayerS2C(const uint64 timeMs, const Player *player) {
        FlatBufferBuilder fbb;
        const auto teleportPlayer = CreateTeleportPlayerS2C(fbb, timeMs, player);
        const auto wrapper = CreatePacketWrapper(fbb, PacketType_TeleportPlayerS2C, teleportPlayer.Union());
        fbb.Finish(wrapper);
        return fbb;
    }

    FlatBufferBuilder SpawnLaserS2C(const Laser *laser) {
        FlatBufferBuilder fbb;
        const auto spawnLaser = CreateSpawnLaserS2C(fbb, laser);
        const auto wrapper = CreatePacketWrapper(fbb, PacketType_SpawnLaserS2C, spawnLaser.Union());
        fbb.Finish(wrapper);
        return fbb;
    }

    FlatBufferBuilder DespawnLaserS2C(const uint32 uuid) {
        FlatBufferBuilder fbb;
        const auto despawnLaser = CreateDespawnLaserS2C(fbb, uuid);
        const auto wrapper = CreatePacketWrapper(fbb, PacketType_DespawnLaserS2C, despawnLaser.Union());
        fbb.Finish(wrapper);
        return fbb;
    }

    FlatBufferBuilder CollisionS2C(const uint32 uuidA, const uint32 uuidB) {
        FlatBufferBuilder fbb;
        const auto collision = CreateCollisionS2C(fbb, uuidA, uuidB);
        const auto wrapper = CreatePacketWrapper(fbb, PacketType_CollisionS2C, collision.Union());
        fbb.Finish(wrapper);
        return fbb;
    }

    FlatBufferBuilder TextS2C(const std::string &text) {
        FlatBufferBuilder fbb;
        const auto textS2C = CreateTextS2CDirect(fbb, text.c_str());
        const auto wrapper = CreatePacketWrapper(fbb, PacketType_TextS2C, textS2C.Union());
        fbb.Finish(wrapper);
        return fbb;
    }

    FlatBufferBuilder InputC2S(const uint64 timeMs, const uint16 bitmap) {
        FlatBufferBuilder fbb;
        const auto input = CreateInputC2S(fbb, timeMs, bitmap);
        const auto wrapper = CreatePacketWrapper(fbb, PacketType_InputC2S, input.Union());
        fbb.Finish(wrapper);
        return fbb;
    }

    FlatBufferBuilder TextC2S(const std::string &text) {
        FlatBufferBuilder fbb;
        const auto textC2S = CreateTextC2SDirect(fbb, text.c_str());
        const auto wrapper = CreatePacketWrapper(fbb, PacketType_TextC2S, textC2S.Union());
        fbb.Finish(wrapper);
        return fbb;
    }
}
