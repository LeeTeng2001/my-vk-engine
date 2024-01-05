#include "static.hpp"
#include "components/graphic/mesh.hpp"

namespace luna {

void StaticActor::delayInit() {
    _meshComp = make_shared<MeshComponent>(getEngine(), getId());
    if (!_modelPath.empty()) {
        _meshComp->loadModal(_modelPath);
        _meshComp->uploadToGpu();
    }
    addComponent(_meshComp);
}

}