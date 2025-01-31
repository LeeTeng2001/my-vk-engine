#include <memory>

#include "core/engine.hpp"

int main() {
    auto engine = std::make_shared<luna::Engine>();
    if (!engine->initialize(engine)) {
        return 1;
    };
    engine->run();
    return 0;
}
