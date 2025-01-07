#include "network.h"

namespace Net {
    void Initialize() {
        if (int res = enet_initialize()) {
            std::cout << res << " Failed to initialize ENet.\n";
            exit(res);
        }

        atexit(enet_deinitialize);
    }
}
