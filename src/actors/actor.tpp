
namespace luna {
template<class T>
std::shared_ptr<T> Actor::getComponent() {
    for (const auto &c: _components) {
        std::shared_ptr<T> derivedPtr = std::dynamic_pointer_cast<T>(c);
        if (derivedPtr) {
            return derivedPtr;
        }
    }
    return nullptr;
}
}
