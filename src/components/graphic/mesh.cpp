#include <utility>
#include <tiny_obj_loader.h>
#include <tiny_gltf.h>

#include "core/engine.hpp"
#include "actors/actor.hpp"
#include "core/renderer/renderer.hpp"
#include "mesh.hpp"
#include "utils/algo.hpp"

MeshComponent::MeshComponent(weak_ptr<Actor> owner, int updateOrder) : Component(std::move(owner), updateOrder) {
}

MeshComponent::~MeshComponent() {
    if (_modelState != nullptr) {
        getOwner()->getEngine()->getRenderer()->removeModal(_modelState);
    }
}

void MeshComponent::postUpdate() {
    if (_modelState != nullptr) {
         _modelState->worldTransform = getOwner()->getWorldTransform();
    }
}

void MeshComponent::loadObj(const string &path, const glm::vec3 &upAxis) {
    auto l = SLog::get();
    fs::path modelPath(path);

    l->info(fmt::format("Loading obj model: {:s}", modelPath.generic_string()));
    tinyobj::ObjReader objReader;
    tinyobj::ObjReaderConfig reader_config;
    reader_config.triangulate = true;
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

    array<int, 3> axisIdxOrder = HelperAlgo::getAxisOrder(upAxis);

    // Upload materials info
    vector<int> gpuMatId{};
    for (const auto &mat: materials) {
        MaterialCpu matCpu{};
        matCpu.info.diffuse = glm::vec4{mat.diffuse[0], mat.diffuse[1], mat.diffuse[2], 1};
        matCpu.info.emissive = glm::vec4{mat.emission[0], mat.emission[1], mat.emission[2], 1};

        // Load texture if any
        if (!mat.diffuse_texname.empty()) {
            matCpu.albedoTexture.path = mat.diffuse_texname;
            matCpu.albedoTexture.stbRef = stbi_load(mat.diffuse_texname.c_str(), &matCpu.albedoTexture.texWidth, &matCpu.albedoTexture.texHeight, &matCpu.albedoTexture.texChannels, 4);
            matCpu.albedoTexture.texChannels = 4;
            if (!matCpu.albedoTexture.stbRef) {
                l->error(fmt::format("failed to load diffuse texture at path: {:s}", mat.diffuse_texname));
            } else {
                matCpu.info.setColor();
            }
        }
        if (!mat.normal_texname.empty()) {
            matCpu.albedoTexture.path = mat.normal_texname;
            matCpu.albedoTexture.stbRef = stbi_load(mat.normal_texname.c_str(), &matCpu.normalTexture.texWidth, &matCpu.normalTexture.texHeight, &matCpu.normalTexture.texChannels, 4);
            matCpu.albedoTexture.texChannels = 4;
            if (!matCpu.albedoTexture.stbRef) {
                l->error(fmt::format("failed to load diffuse texture at path: {:s}", mat.normal_texname));
            } else {
                matCpu.info.setNormal();
            }
        }
        // TODO: load ao, height, roughness into single image

        gpuMatId.push_back(getOwner()->getEngine()->getRenderer()->createMaterial(matCpu));
    }

    // Usage Guide: https://github.com/tinyobjloader/tinyobjloader
    // Loop over shapes
    // By default obj is already in counter-clockwise order
    ModelDataPartition curPartition{};
    curPartition.materialId = -1;
    for (auto &shape : shapes) {
        l->info(fmt::format("shape: {:s}", shape.name));
        // Loop over faces (polygon)
        size_t index_offset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            // check for new material group
            if (gpuMatId[shape.mesh.material_ids[f]] != curPartition.materialId) {
                if (curPartition.materialId != -1) {
                    // push new material
                    curPartition.indexCount = _modelData.indices.size() - curPartition.firstIndex;
                    _modelData.modelDataPartition.push_back(curPartition);
                }
                curPartition.materialId = gpuMatId[shape.mesh.material_ids[f]];
                curPartition.firstIndex = _modelData.indices.size();
            }

            bool shouldGenerateNormal = false;
            // Loop over vertices in the face. (rn I hardcode 3 vertices for a face)
            for (size_t v = 0; v < 3; v++) {
                Vertex vertex{};

                // access to vertex
                tinyobj::index_t idx = shape.mesh.indices[index_offset + v];
                tinyobj::real_t vx = attrib.vertices[3*size_t(idx.vertex_index)+axisIdxOrder[0]];
                tinyobj::real_t vy = attrib.vertices[3*size_t(idx.vertex_index)+axisIdxOrder[1]];
                tinyobj::real_t vz = attrib.vertices[3*size_t(idx.vertex_index)+axisIdxOrder[2]];

                vertex.pos.x = vx;
                vertex.pos.y = vy;
                vertex.pos.z = vz;

                // Check if `normal_index` is zero or positive. negative = no normal data
                if (idx.normal_index >= 0) {
                    tinyobj::real_t nx = attrib.normals[3*size_t(idx.normal_index)+axisIdxOrder[0]];
                    tinyobj::real_t ny = attrib.normals[3*size_t(idx.normal_index)+axisIdxOrder[1]];
                    tinyobj::real_t nz = attrib.normals[3*size_t(idx.normal_index)+axisIdxOrder[2]];
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

            // good figures: https://www.opengl-tutorial.org/intermediate-tutorials/tutorial-13-normal-mapping/
            // calculate tangent bitangent for normals
            glm::vec3& v0 = _modelData.vertex[index_offset+0].pos;
            glm::vec3& v1 = _modelData.vertex[index_offset+1].pos;
            glm::vec3& v2 = _modelData.vertex[index_offset+2].pos;
            glm::vec2& uv0 = _modelData.vertex[index_offset+0].texCoord;
            glm::vec2& uv1 = _modelData.vertex[index_offset+1].texCoord;
            glm::vec2& uv2 = _modelData.vertex[index_offset+2].texCoord;

            // Edges of the triangle : position delta, UV delta
            glm::vec3 deltaPos1 = v1-v0;
            glm::vec3 deltaPos2 = v2-v0;
            glm::vec2 deltaUV1 = uv1-uv0;
            glm::vec2 deltaUV2 = uv2-uv0;

            // formula for tangent
            // great formula derivation: https://learnopengl.com/Advanced-Lighting/Normal-Mapping#:~:text=Tangent%20space%20is%20a%20space,of%20the%20final%20transformed%20direction.
            float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
            glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y)*r;
            glm::vec3 bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x)*r;
            for (int i = 0; i < 3; ++i) { // TODO: could probably optimise this
                _modelData.vertex[index_offset+i].tangents = tangent;
                _modelData.vertex[index_offset+i].bitangents = bitangent;
            }

            index_offset += 3;
        }

        // last partition
        curPartition.indexCount = _modelData.indices.size() - curPartition.firstIndex;
        _modelData.modelDataPartition.push_back(curPartition);
        curPartition.materialId = -1;
        l->debug(fmt::format("shape: {:s}, cur total partition: {:d}", shape.name, _modelData.modelDataPartition.size()));
    }

    l->info(fmt::format("model total indices: {:d}, partition: {:d}",
                        _modelData.indices.size(), _modelData.modelDataPartition.size()));
}

void MeshComponent::loadGlb(const string &path, const glm::vec3 &upAxis) {
    auto l = SLog::get();

    l->info(fmt::format("Loading glb model: {:s}", path));
    tinygltf::Model modal;
    tinygltf::TinyGLTF loader;
    string err;
    string warn;

    if (!loader.LoadBinaryFromFile(&modal, &err, &warn, path)) {
        l->error("failed to parse binary glb file");
        return;
    }
    if (!err.empty()) {
        l->error(fmt::format("Load glb file error: {:s}", err));
        return;
    }
    if (!warn.empty()) {
        l->warn(warn);
    }

    // TODO: implement the rest
//    for (const auto &mesh: modal.meshes) {
//    }
}

void MeshComponent::loadModal(const string &path, const glm::vec3 &upAxis) {
    if (path.ends_with(".obj")) {
        loadObj(path, upAxis);
    } else if (path.ends_with(".glb")) {
        loadGlb(path, upAxis);
    } else {
        auto l = SLog::get();
        l->error("unrecognised model format");
    }
}

void MeshComponent::generatedSquarePlane(float sideLength) {
    // generate square plane facing upward
    float hs = sideLength / 2;
    // pos, normal, tex, tangent, bitangent
    // counter-clockwise
    _modelData.vertex = {
            {{-hs, hs, 0},  {0, 0, 1}, {0, 0},  {1,0,0}, {0,1,0}},
            {{hs, hs, 0},   {0, 0, 1}, {1, 0},  {1,0,0}, {0,1,0}},
            {{-hs, -hs, 0}, {0, 0, 1}, {0, 1},  {1,0,0}, {0,1,0}},
            {{hs, -hs, 0},  {0, 0, 1}, {1, 1},  {1,0,0}, {0,1,0}},
    };
    _modelData.indices = {
            0, 2, 1,
            2, 3, 1,
    };
}

void MeshComponent::uploadToGpu() {
    auto l = SLog::get();

    // Upload to GPU
    _modelState = getOwner()->getEngine()->getRenderer()->uploadModel(_modelData);
    if (_modelState == nullptr) {
        l->error("failed to upload modal data to gpu");
    }

//    // TODO: free local resource
//    stbi_image_free(_modelData.albedoTexture.stbRef);
//    stbi_image_free(_modelData.normalTexture.stbRef);
}

