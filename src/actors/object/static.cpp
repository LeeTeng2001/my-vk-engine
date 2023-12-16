#include "static.hpp"
#include "components/graphic/mesh.hpp"

void StaticActor::delayInit() {
    _meshComp = make_shared<MeshComponent>(getSelf());
    _meshComp->loadModal(_modelPath);
    if (!_diffuseTexPath.empty()) {
        _meshComp->loadDiffuseTexture(_diffuseTexPath);
    }
    _meshComp->uploadToGpu();
    addComponent(_meshComp);
}
