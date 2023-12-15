#include <memory>

#include "core/engine.hpp"

int main() {
    auto engine = make_shared<Engine>();
    if (!engine->initialize(engine)) {
        return 1;
    };
    engine->run();
    return 0;
}
