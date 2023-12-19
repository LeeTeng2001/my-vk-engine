#include <utility>
#include <tiny_obj_loader.h>

#include "core/engine.hpp"
#include "actors/actor.hpp"
#include "renderer/renderer.hpp"
#include "mesh.hpp"

MeshComponent::MeshComponent(weak_ptr<Actor> owner, int updateOrder) : Component(std::move(owner), updateOrder) {
}

MeshComponent::~MeshComponent() {
    if (_modelState != nullptr) {
        getOwner()->getEngine()->getRenderer()->removeModal(_modelState);
    }
}

void MeshComponent::onUpdateWorldTransform() {
    if (_modelState != nullptr) {
         _modelState->worldTransform = getOwner()->getWorldTransform();
         _modelState->rotationTransform = glm::mat4_cast(getOwner()->getRotation());
    }
}

void MeshComponent::loadModal(const string &path) {
    auto l = SLog::get();
    fs::path modelPath(path);

    l->info(fmt::format("Loading model: {:s}", modelPath.generic_string()));
    tinyobj::ObjReader objReader;
    tinyobj::ObjReaderConfig reader_config;
    if (!objReader.ParseFromFile(modelPath.generic_string(), reader_config)) {
        l->error("load model return error");
        if (!objReader.Error().empty()) {
            l->error(fmt::format("load model error: {:s}", objReader.Error().c_str()));
        }
        return;
    }
    if (!objReader.Warning().empty()) {
        l->warn(fmt::format("load model warn: {:s}", objReader.Warning().c_str()));
    }
    //attrib will contain the vertex arrays of the file
    tinyobj::attrib_t attrib = objReader.GetAttrib();
    std::vector<tinyobj::shape_t> shapes = objReader.GetShapes();
    std::vector<tinyobj::material_t> materials = objReader.GetMaterials();

    l->info(fmt::format("model shapes: {:d}", shapes.size()));
    l->info(fmt::format("model materials: {:d}", materials.size()));

    // Usage Guide: https://github.com/tinyobjloader/tinyobjloader
    // Loop over shapes
    // By default obj is already in counter-clockwise order
    for (auto &shape : shapes) {
        // Loop over faces (polygon)
        size_t index_offset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            bool shouldGenerateNormal = false;
            // Loop over vertices in the face. (rn I hardcode 3 vertices for a face)
            for (size_t v = 0; v < 3; v++) {
                Vertex vertex{};

                // access to vertex
                tinyobj::index_t idx = shape.mesh.indices[index_offset + v];
                tinyobj::real_t vx = attrib.vertices[3*size_t(idx.vertex_index)+0];
                tinyobj::real_t vy = attrib.vertices[3*size_t(idx.vertex_index)+1];
                tinyobj::real_t vz = attrib.vertices[3*size_t(idx.vertex_index)+2];

                vertex.pos.x = vx;
                vertex.pos.y = vy;
                vertex.pos.z = vz;
                vertex.color.x = vx;
                vertex.color.y = vy;
                vertex.color.z = vz;

                // Check if `normal_index` is zero or positive. negative = no normal data
                if (idx.normal_index >= 0) {
                    tinyobj::real_t nx = attrib.normals[3*size_t(idx.normal_index)+0];
                    tinyobj::real_t ny = attrib.normals[3*size_t(idx.normal_index)+1];
                    tinyobj::real_t nz = attrib.normals[3*size_t(idx.normal_index)+2];
                    vertex.normal.x = nx;
                    vertex.normal.y = ny;
                    vertex.normal.z = nz;
                } else {
                    shouldGenerateNormal = true;
                }
                // TODO: Normal view transformation https://www.google.com/search?q=Normal+Vector+Transformation&rlz=1C1CHBD_enMY1006MY1006&oq=normal+vec&gs_lcrp=EgZjaHJvbWUqBggBEEUYOzIGCAAQRRg5MgYIARBFGDsyBggCEEUYOzIGCAMQRRg70gEIMjA0OGowajeoAgCwAgA&sourceid=chrome&ie=UTF-8
                // Check if `texcoord_index` is zero or positive. negative = no texcoord data
                if (idx.texcoord_index >= 0) {
                    tinyobj::real_t tx = attrib.texcoords[2*size_t(idx.texcoord_index)+0];
                    tinyobj::real_t ty = attrib.texcoords[2*size_t(idx.texcoord_index)+1];
                    // need to flip sign becase vulkan use tex from topy -> btm, inverse to OBJ
                    vertex.texCoord.x = tx;
                    vertex.texCoord.y = 1-ty;
                }
                // Optional: vertex colors
                // tinyobj::real_t red   = attrib.colors[3*size_t(idx.vertex_index)+0];
                // tinyobj::real_t green = attrib.colors[3*size_t(idx.vertex_index)+1];
                // tinyobj::real_t blue  = attrib.colors[3*size_t(idx.vertex_index)+2];

                _modelData.vertex.push_back(vertex);
                _modelData.indices.push_back(_modelData.indices.size());
            }

            if (shouldGenerateNormal) {
                // since our model is being drawn in counter-clockwise
                glm::vec3 v1 = _modelData.vertex[index_offset+2].pos - _modelData.vertex[index_offset+1].pos;
                glm::vec3 v2 = _modelData.vertex[index_offset].pos - _modelData.vertex[index_offset+1].pos;
                glm::vec3 norm = glm::normalize(glm::cross(v1, v2));
                for (int i = 0; i < 3; ++i) {
                    _modelData.vertex[index_offset+i].normal = norm;
                }
            }

            // By default obj uses counter-clockwise face
            // since, our front face is counter clockwise, and the
            // viewport get flipped during projection transformation
            // we need to swap front faces to clockwise in world space
            std::swap(_modelData.vertex[index_offset+1], _modelData.vertex[index_offset+2]);

            index_offset += 3;
        }
    }
}

void MeshComponent::generatedSquarePlane(float sideLength) {
    // generate square plane facing upward
    float hs = sideLength / 2;
    // pos, normal, color, tex
    // clockwise, will be flipped by cam projection
    _modelData.vertex = {
            {{-hs, 0, hs}, {0, 1, 0}, {0, 0, 0}, {0, 0}},
            {{hs, 0, hs}, {0, 1, 0},  {1, 0, 0}, {1, 0}},
            {{-hs, 0, -hs}, {0, 1, 0},{0, 1, 0}, {0, 1}},
            {{hs, 0, -hs}, {0, 1, 0}, {1, 1, 0}, {1, 1}},
    };
    _modelData.indices = {
            0, 2, 1,
            2, 3, 1,
    };
}

void MeshComponent::loadDiffuseTexture(const string &path) {
    auto l = SLog::get();
    l->info(fmt::format("loading texture {:s}", path));

    _modelData.albedoTexture.path = path;
    _modelData.albedoTexture.stbRef = stbi_load(path.c_str(), &_modelData.albedoTexture.texWidth, &_modelData.albedoTexture.texHeight, &_modelData.albedoTexture.texChannels, 4);
    _modelData.albedoTexture.texChannels = 4; // this is 4 because we force it
    if (!_modelData.albedoTexture.stbRef) {
        l->error(fmt::format("failed to load diffuse texture at path: {:s}", path));
    }
}

void MeshComponent::loadNormalTexture(const string &path) {
    auto l = SLog::get();
    l->info(fmt::format("loading texture {:s}", path));

    _modelData.normalTexture.path = path;
    _modelData.normalTexture.stbRef = stbi_load(path.c_str(), &_modelData.normalTexture.texWidth, &_modelData.normalTexture.texHeight, &_modelData.normalTexture.texChannels, 4);
    _modelData.normalTexture.texChannels = 4; // this is 4 because we force it
    if (!_modelData.normalTexture.stbRef) {
        l->error(fmt::format("failed to load diffuse texture at path: {:s}", path));
    }
}

void MeshComponent::uploadToGpu() {
    auto l = SLog::get();

    // Upload to GPU
    _modelState = getOwner()->getEngine()->getRenderer()->uploadAndPopulateModal(_modelData);
    if (_modelState == nullptr) {
        l->error("failed to upload modal data to gpu");
    }

    // free local resource
    stbi_image_free(_modelData.albedoTexture.stbRef);
    stbi_image_free(_modelData.normalTexture.stbRef);
}

