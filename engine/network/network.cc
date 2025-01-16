#include "network.h"

namespace Net {
    void Initialize() {
        if (int res = enet_initialize()) {
            LOG(res << " Failed to initialize ENet.\n");
            exit(EXIT_FAILURE);
        }

        LOG("Successfully initialized ENet.\n");

        atexit(enet_deinitialize);
    }
}
