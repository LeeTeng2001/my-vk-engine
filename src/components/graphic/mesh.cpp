#include <tiny_obj_loader.h>
#include <tiny_gltf.h>

#include "core/engine.hpp"
#include "actors/actor.hpp"
#include "core/renderer/renderer.hpp"
#include "mesh.hpp"
#include "utils/algo.hpp"

namespace luna {

MeshComponent::MeshComponent(const std::shared_ptr<Engine> &engine, int ownerId)
    : Component(engine, ownerId) {}

MeshComponent::~MeshComponent() {
    if (_modelState != nullptr) {
        getEngine()->getRenderer()->removeModal(_modelState);
    }
}

void MeshComponent::postUpdate() {
    if (_modelState != nullptr) {
        _modelState->worldTransform = getOwner()->getWorldTransform();
    }
}

void MeshComponent::loadObj(const std::string &path, const glm::vec3 &upAxis) {
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
    // attrib will contain the vertex arrays of the file
    tinyobj::attrib_t attrib = objReader.GetAttrib();
    std::vector<tinyobj::shape_t> shapes = objReader.GetShapes();
    std::vector<tinyobj::material_t> materials = objReader.GetMaterials();

    l->info(fmt::format("model shapes: {:d}", shapes.size()));
    l->info(fmt::format("model materials: {:d}", materials.size()));

    std::array<int, 3> axisIdxOrder = HelperAlgo::getAxisOrder(upAxis);

    // Upload materials info
    std::stack<std::function<void()>> cpuMatCleanup;
    std::vector<int> gpuMatId{};
    for (const auto &mat : materials) {
        MaterialCpu matCpu{};
        matCpu.info.diffuse = glm::vec4{mat.diffuse[0], mat.diffuse[1], mat.diffuse[2], 1};
        matCpu.info.emissive = glm::vec4{mat.emission[0], mat.emission[1], mat.emission[2], 1};

        // Load texture if any
        if (!mat.diffuse_texname.empty()) {
            stbi_uc *data =
                stbi_load(mat.diffuse_texname.c_str(), &matCpu.albedoTexture.texWidth,
                          &matCpu.albedoTexture.texHeight, &matCpu.albedoTexture.texChannels, 4);
            matCpu.albedoTexture.data = data;
            matCpu.albedoTexture.texChannels = 4;
            if (!matCpu.albedoTexture.data) {
                l->error(fmt::format("failed to load diffuse texture at path: {:s}",
                                     mat.diffuse_texname));
            } else {
                matCpu.info.setColor();
                cpuMatCleanup.emplace([this, data]() { stbi_image_free(data); });
            }
        }
        if (!mat.normal_texname.empty()) {
            stbi_uc *data =
                stbi_load(mat.normal_texname.c_str(), &matCpu.normalTexture.texWidth,
                          &matCpu.normalTexture.texHeight, &matCpu.normalTexture.texChannels, 4);
            matCpu.albedoTexture.data = data;
            matCpu.normalTexture.texChannels = 4;
            if (!matCpu.normalTexture.data) {
                l->error(
                    fmt::format("failed to load normal texture at path: {:s}", mat.normal_texname));
            } else {
                matCpu.info.setNormal();
                cpuMatCleanup.emplace([this, data]() { stbi_image_free(data); });
            }
        }
        // TODO: load ao, height, roughness into single image

        gpuMatId.push_back(getEngine()->getRenderer()->createMaterial(matCpu));
    }
    while (!cpuMatCleanup.empty()) {
        auto nextCleanup = cpuMatCleanup.top();
        cpuMatCleanup.pop();
        nextCleanup();
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
            int primitiveGpuMatId = 0;  // 0 is always default mat
            if (shape.mesh.material_ids[f] != -1) {
                primitiveGpuMatId = gpuMatId[shape.mesh.material_ids[f]];
            }

            if (primitiveGpuMatId != curPartition.materialId) {
                if (curPartition.materialId != -1) {
                    // push new material
                    curPartition.indexCount = _modelData.indices.size() - curPartition.firstIndex;
                    _modelData.modelDataPartition.push_back(curPartition);
                }
                curPartition.materialId = primitiveGpuMatId;
                curPartition.firstIndex = _modelData.indices.size();
            }

            bool shouldGenerateNormal = false;
            // Loop over vertices in the face. (rn I hardcode 3 vertices for a face)
            for (size_t v = 0; v < 3; v++) {
                Vertex vertex{};

                // access to vertex
                tinyobj::index_t idx = shape.mesh.indices[index_offset + v];
                tinyobj::real_t vx =
                    attrib.vertices[3 * size_t(idx.vertex_index) + axisIdxOrder[0]];
                tinyobj::real_t vy =
                    attrib.vertices[3 * size_t(idx.vertex_index) + axisIdxOrder[1]];
                tinyobj::real_t vz =
                    attrib.vertices[3 * size_t(idx.vertex_index) + axisIdxOrder[2]];

                vertex.pos.x = vx;
                vertex.pos.y = vy;
                vertex.pos.z = vz;

                // Check if `normal_index` is zero or positive. negative = no normal data
                if (idx.normal_index >= 0) {
                    tinyobj::real_t nx =
                        attrib.normals[3 * size_t(idx.normal_index) + axisIdxOrder[0]];
                    tinyobj::real_t ny =
                        attrib.normals[3 * size_t(idx.normal_index) + axisIdxOrder[1]];
                    tinyobj::real_t nz =
                        attrib.normals[3 * size_t(idx.normal_index) + axisIdxOrder[2]];
                    vertex.normal.x = nx;
                    vertex.normal.y = ny;
                    vertex.normal.z = nz;
                } else {
                    shouldGenerateNormal = true;
                }
                // TODO: Normal view transformation
                // https://www.google.com/search?q=Normal+Vector+Transformation&rlz=1C1CHBD_enMY1006MY1006&oq=normal+vec&gs_lcrp=EgZjaHJvbWUqBggBEEUYOzIGCAAQRRg5MgYIARBFGDsyBggCEEUYOzIGCAMQRRg70gEIMjA0OGowajeoAgCwAgA&sourceid=chrome&ie=UTF-8
                // Check if `texcoord_index` is zero or positive. negative = no texcoord data
                if (idx.texcoord_index >= 0) {
                    tinyobj::real_t tx = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
                    tinyobj::real_t ty = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];
                    // need to flip sign becase vulkan use tex from topy -> btm, inverse to OBJ
                    vertex.texCoord.x = tx;
                    vertex.texCoord.y = 1 - ty;
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
                glm::vec3 v1 = _modelData.vertex[index_offset + 2].pos -
                               _modelData.vertex[index_offset + 1].pos;
                glm::vec3 v2 =
                    _modelData.vertex[index_offset].pos - _modelData.vertex[index_offset + 1].pos;
                glm::vec3 norm = glm::normalize(glm::cross(v1, v2));
                for (int i = 0; i < 3; ++i) {
                    _modelData.vertex[index_offset + i].normal = norm;
                }
            }

            generateTangentBitangent(index_offset + 0, index_offset + 1, index_offset + 2);

            index_offset += 3;
        }

        // last partition
        curPartition.indexCount = _modelData.indices.size() - curPartition.firstIndex;
        _modelData.modelDataPartition.push_back(curPartition);
        curPartition.materialId = -1;
        l->debug(fmt::format("shape: {:s}, cur total partition: {:d}", shape.name,
                             _modelData.modelDataPartition.size()));
    }

    l->info(fmt::format("model total indices: {:d}, partition: {:d}", _modelData.indices.size(),
                        _modelData.modelDataPartition.size()));
}

void MeshComponent::loadGlb(const std::string &path, const glm::vec3 &upAxis) {
    auto l = SLog::get();

    // https://github.com/KhronosGroup/glTF-Tutorials/blob/master/gltfTutorial/gltfTutorial_002_BasicGltfStructure.md
    // Load GLB will expand all scene
    l->info(fmt::format("Loading glb model: {:s}", path));
    tinygltf::Model modal;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

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

    l->debug(fmt::format("model shapes: {:d}", modal.meshes.size()));
    l->debug(fmt::format("model materials: {:d}", modal.materials.size()));
    l->debug(fmt::format("model textures: {:d}", modal.textures.size()));

    std::array<int, 3> axisIdxOrder = HelperAlgo::getAxisOrder(upAxis);

    // Upload materials info
    std::vector<int> gpuMatId{};
    for (const auto &gltfMat : modal.materials) {
        MaterialCpu matCpu{};
        matCpu.info.diffuse = glm::vec4{gltfMat.pbrMetallicRoughness.baseColorFactor[0],
                                        gltfMat.pbrMetallicRoughness.baseColorFactor[1],
                                        gltfMat.pbrMetallicRoughness.baseColorFactor[2], 1};
        matCpu.info.emissive = glm::vec4{gltfMat.emissiveFactor[0], gltfMat.emissiveFactor[1],
                                         gltfMat.emissiveFactor[2], 1};

        // Load texture if any
        if (gltfMat.pbrMetallicRoughness.baseColorTexture.index != -1) {
            const tinygltf::Texture &gltfTex =
                modal.textures[gltfMat.pbrMetallicRoughness.baseColorTexture.index];
            const tinygltf::Image &image = modal.images[gltfTex.source];
            if (image.component != 4) {
                l->warn(fmt::format("texture {:s} is not RGBA", image.name));
            }
            matCpu.albedoTexture.data = image.image.data();
            matCpu.albedoTexture.texWidth = image.width;
            matCpu.albedoTexture.texHeight = image.height;
            matCpu.albedoTexture.texChannels = image.component;
            matCpu.info.setColor();
        }
        if (gltfMat.normalTexture.index != -1) {
            const tinygltf::Texture &gltfTex = modal.textures[gltfMat.normalTexture.index];
            const tinygltf::Image &image = modal.images[gltfTex.source];
            if (image.component != 4) {
                l->warn(fmt::format("texture {:s} is not RGBA", image.name));
            }
            matCpu.normalTexture.data = image.image.data();
            matCpu.normalTexture.texWidth = image.width;
            matCpu.normalTexture.texHeight = image.height;
            matCpu.normalTexture.texChannels = image.component;
            matCpu.info.setNormal();
        }
        // TODO: load ao, height, roughness into single image

        gpuMatId.push_back(getEngine()->getRenderer()->createMaterial(matCpu));
    }

    ModelDataPartition curPartition{};
    curPartition.materialId = -1;

    // GLTF two reference should be enough, one is the visual guide, one is the official tutorial
    // Visual guide, handybook: https://www.khronos.org/files/gltf20-reference-guide.pdf
    // Official tutorial: https://github.com/KhronosGroup/glTF-Tutorials/blob/master/gltfTutorial

    // A node can compose of mesh, transformation, right now we skip children
    auto identity = glm::identity<glm::mat4>();
    for (const auto &nodeIdx : modal.scenes[modal.defaultScene].nodes) {
        recurParseGlb(nodeIdx, modal, gpuMatId, curPartition, identity);
    }
    // Last partition
    if (curPartition.materialId != -1) {  // push new material
        curPartition.indexCount = _modelData.indices.size() - curPartition.firstIndex;
        _modelData.modelDataPartition.push_back(curPartition);
    }
}

void MeshComponent::recurParseGlb(int nodeIdx, const tinygltf::Model &modal,
                                  const std::vector<int> &gpuMatId, ModelDataPartition &partition,
                                  const glm::mat4 &parentTransform) {
    auto l = SLog::get();

    const tinygltf::Node &node = modal.nodes[nodeIdx];

    // sanity check for non-duplicate transformation, matrix or each individual can only exist one
    if (!node.matrix.empty() &&
        (!node.translation.empty() || !node.scale.empty() || !node.rotation.empty())) {
        l->error("node has non-zero transform matrix and non-zero translation/scale/rotation");
        return;
    }

    // compute current transformation matrix
    glm::mat4 currentTransform = glm::identity<glm::mat4>();
    // either use matrix directly or construct from
    if (!node.matrix.empty()) {
        for (int i = 0; i < 16; ++i) {
            currentTransform[i / 4][i % 4] = static_cast<float>(node.matrix[i]);
        }
    } else {
        if (node.translation.size() == 3) {
            currentTransform = glm::translate(
                currentTransform,
                glm::vec3(node.translation[0], node.translation[1], node.translation[2]));
        }
        if (node.rotation.size() == 4) {
            currentTransform *= glm::mat4_cast(
                glm::quat(node.rotation[0], node.rotation[1], node.rotation[2], node.rotation[3]));
        }
        if (node.scale.size() == 3) {
            currentTransform = glm::scale(currentTransform,
                                          glm::vec3(node.scale[0], node.scale[1], node.scale[2]));
        }
    }

    currentTransform = parentTransform * currentTransform;

    // compute children
    if (!node.children.empty()) {
        // l->debug(fmt::format("parsing node {:s} children {:d}", node.name,
        // node.children.size()));
        for (const auto &childNodeIdx : node.children) {
            recurParseGlb(childNodeIdx, modal, gpuMatId, partition, currentTransform);
        }
        return;
    }

    // parse node info
    // https://github.com/KhronosGroup/glTF-Tutorials/blob/master/gltfTutorial/gltfTutorial_003_MinimalGltfFile.md
    if (node.mesh == -1) {  // parent node
        // l->warn(fmt::format("node {:s} has no children and empty mesh", node.name));
        return;
    }

    for (const auto &primitive : modal.meshes[node.mesh].primitives) {
        if (primitive.mode != TINYGLTF_MODE_TRIANGLES) {
            l->warn(fmt::format("mesh {:s} primitive has non-triangle mode {:d}, skipping",
                                modal.meshes[node.mesh].name, primitive.mode));
            continue;
        }

        // Fallback default material
        int primitiveGpuMatId = 0;  // 0 is always default mat
        if (primitive.material != -1) {
            primitiveGpuMatId = gpuMatId[primitive.material];
        }

        // Parse material, check for new material group
        if (primitiveGpuMatId != partition.materialId) {
            if (partition.materialId != -1) {  // push new material
                partition.indexCount = _modelData.indices.size() - partition.firstIndex;
                _modelData.modelDataPartition.push_back(partition);
            }
            partition.materialId = primitiveGpuMatId;
            partition.firstIndex = _modelData.indices.size();
        }

        // TODO: optimsie by using the buffer directly instead of passing to
        // index buffer for current primitive, right now we force scalar + 16bytes
        const tinygltf::Accessor &indexAccesor = modal.accessors[primitive.indices];
        if (indexAccesor.type != TINYGLTF_TYPE_SCALAR) {
            l->error(fmt::format("index accessor type is not scalar {:d}", indexAccesor.type));
        }
        const tinygltf::BufferView &idxBufView = modal.bufferViews[indexAccesor.bufferView];
        const tinygltf::Buffer &idxBuf = modal.buffers[idxBufView.buffer];
        int baseIndicies = _modelData.vertex.size();
        _modelData.indices.resize(_modelData.indices.size() + indexAccesor.count);
        // We force copy uint16, little endian
        for (int i = 0; i < indexAccesor.count; ++i) {
            memcpy(&_modelData.indices[baseIndicies + i],
                   &idxBuf.data[indexAccesor.byteOffset + idxBufView.byteOffset + (i * 2)], 2);
            _modelData.indices[baseIndicies + i] += baseIndicies;
        }

        // get vertex count
        size_t vertexCount = modal.accessors[primitive.attributes.begin()->second].count;

        glm::mat3 normMatrix = glm::transpose(glm::inverse(glm::mat3(currentTransform)));

        int baseVertIdx = _modelData.vertex.size();
        for (int i = 0; i < vertexCount; ++i) {
            Vertex vertex{};

            // parse each node attribute like position, normal
            for (const auto &pAttr : primitive.attributes) {
                const tinygltf::Accessor &accessor = modal.accessors[pAttr.second];
                const tinygltf::BufferView &bufView = modal.bufferViews[accessor.bufferView];
                const tinygltf::Buffer &buf = modal.buffers[bufView.buffer];

                // size of float + stride
                if (pAttr.first == "POSITION") {
                    if (accessor.type != TINYGLTF_TYPE_VEC3) {
                        l->error(fmt::format("position attribute is not vec3 {:d}", accessor.type));
                    }
                    int stride = glm::max(static_cast<int>(bufView.byteStride),
                                          12);  // might have / not have stride
                    memcpy(&vertex.pos.x,
                           &buf.data[accessor.byteOffset + bufView.byteOffset + (i * stride + 0)],
                           4);
                    memcpy(&vertex.pos.y,
                           &buf.data[accessor.byteOffset + bufView.byteOffset + (i * stride + 4)],
                           4);
                    memcpy(&vertex.pos.z,
                           &buf.data[accessor.byteOffset + bufView.byteOffset + (i * stride + 8)],
                           4);

                    // transform
                    glm::vec4 finalPos = currentTransform * glm::vec4{vertex.pos, 1};
                    vertex.pos = finalPos;
                } else if (pAttr.first == "NORMAL") {
                    if (accessor.type != TINYGLTF_TYPE_VEC3) {
                        l->error(fmt::format("normal attribute is not vec3 {:d}", accessor.type));
                    }
                    int stride = glm::max(static_cast<int>(bufView.byteStride), 12);
                    memcpy(&vertex.normal.x,
                           &buf.data[accessor.byteOffset + bufView.byteOffset + (i * stride + 0)],
                           4);
                    memcpy(&vertex.normal.y,
                           &buf.data[accessor.byteOffset + bufView.byteOffset + (i * stride + 4)],
                           4);
                    memcpy(&vertex.normal.z,
                           &buf.data[accessor.byteOffset + bufView.byteOffset + (i * stride + 8)],
                           4);

                    vertex.normal = glm::normalize(normMatrix * vertex.normal);  // normal correct ?
                } else if (pAttr.first == "TEXCOORD_0") {
                    if (accessor.type != TINYGLTF_TYPE_VEC2) {
                        l->error(fmt::format("normal attribute is not vec3 {:d}", accessor.type));
                    }
                    int stride = glm::max(static_cast<int>(bufView.byteStride), 8);
                    memcpy(&vertex.texCoord.x,
                           &buf.data[accessor.byteOffset + bufView.byteOffset + (i * stride + 0)],
                           4);
                    memcpy(&vertex.texCoord.y,
                           &buf.data[accessor.byteOffset + bufView.byteOffset + (i * stride + 4)],
                           4);
                    // TODO: might need to flip sign
                } else {
                    // l->error(fmt::format("unrecognised attribute name {:s}", pAttr.first));
                    //                            return false;
                }
            }

            _modelData.vertex.push_back(vertex);
        }

        // calculate bitangent
        //
        for (int i = 0; i < indexAccesor.count; i += 3) {
            if (glm::length2(_modelData.vertex[_modelData.indices[baseIndicies + i + 0]].normal) !=
                1) {
                // since our model is being drawn in counter-clockwise
                glm::vec3 v1 = _modelData.vertex[_modelData.indices[baseIndicies + i + 2]].pos -
                               _modelData.vertex[_modelData.indices[baseIndicies + i + 1]].pos;
                glm::vec3 v2 = _modelData.vertex[_modelData.indices[baseIndicies + i + 0]].pos -
                               _modelData.vertex[_modelData.indices[baseIndicies + i + 1]].pos;
                glm::vec3 norm = glm::normalize(glm::cross(v1, v2));
                for (int j = 0; j < 3; ++j) {
                    _modelData.vertex[_modelData.indices[baseIndicies + i + j]].normal = norm;
                }
            }

            generateTangentBitangent(_modelData.indices[baseIndicies + i + 0],
                                     _modelData.indices[baseIndicies + i + 1],
                                     _modelData.indices[baseIndicies + i + 2]);
        }
    }
}

void MeshComponent::loadModal(const std::string &path, const glm::vec3 &upAxis) {
    // Something to optimise
    // 1. remove redundant code into function, material etc
    // 2.
    if (path.ends_with(".obj")) {
        loadObj(path, upAxis);
    } else if (path.ends_with(".glb")) {
        loadGlb(path, upAxis);
    } else {
        auto l = SLog::get();
        l->error("unrecognised model format");
    }
}

void MeshComponent::generateTangentBitangent(int v0Idx, int v1Idx, int v2Idx) {
    // good figures:
    // https://www.opengl-tutorial.org/intermediate-tutorials/tutorial-13-normal-mapping/ calculate
    // tangent bitangent for normals
    glm::vec3 &v0 = _modelData.vertex[v0Idx].pos;
    glm::vec3 &v1 = _modelData.vertex[v1Idx].pos;
    glm::vec3 &v2 = _modelData.vertex[v2Idx].pos;
    glm::vec2 &uv0 = _modelData.vertex[v0Idx].texCoord;
    glm::vec2 &uv1 = _modelData.vertex[v1Idx].texCoord;
    glm::vec2 &uv2 = _modelData.vertex[v2Idx].texCoord;

    // Edges of the triangle : position delta, UV delta
    glm::vec3 deltaPos1 = v1 - v0;
    glm::vec3 deltaPos2 = v2 - v0;
    glm::vec2 deltaUV1 = uv1 - uv0;
    glm::vec2 deltaUV2 = uv2 - uv0;

    // formula for tangent
    // great formula derivation:
    // https://learnopengl.com/Advanced-Lighting/Normal-Mapping#:~:text=Tangent%20space%20is%20a%20space,of%20the%20final%20transformed%20direction.
    float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
    glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
    glm::vec3 bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;

    // could optimise it
    _modelData.vertex[v0Idx].tangents = tangent;
    _modelData.vertex[v0Idx].bitangents = bitangent;
    _modelData.vertex[v1Idx].tangents = tangent;
    _modelData.vertex[v1Idx].bitangents = bitangent;
    _modelData.vertex[v2Idx].tangents = tangent;
    _modelData.vertex[v2Idx].bitangents = bitangent;
}

void MeshComponent::generateSquarePlane(float sideLength, const glm::vec3 &color) {
    // generate square plane facing upward
    float hs = sideLength / 2;
    // pos, normal, tex, tangent, bitangent
    // counter-clockwise
    _modelData.vertex = {
        {{-hs, hs, 0}, {0, 0, 1}, {0, 0}, {1, 0, 0}, {0, 1, 0}},
        {{hs, hs, 0}, {0, 0, 1}, {1, 0}, {1, 0, 0}, {0, 1, 0}},
        {{-hs, -hs, 0}, {0, 0, 1}, {0, 1}, {1, 0, 0}, {0, 1, 0}},
        {{hs, -hs, 0}, {0, 0, 1}, {1, 1}, {1, 0, 0}, {0, 1, 0}},
    };
    _modelData.indices = {
        0, 2, 1, 2, 3, 1,
    };
    int matId = createDefaultMat(color);
    _modelData.modelDataPartition.push_back(
        {0, static_cast<int>(_modelData.indices.size()), matId});
}

void MeshComponent::generateSphere(float radius, int horizontalLine, int verticalLine,
                                   const glm::vec3 &color) {
    // https://stackoverflow.com/questions/4081898/procedurally-generate-a-sphere-mesh
    // generate all vertices
    for (int vi = 0; vi < verticalLine; ++vi) {
        for (int hi = 0; hi <= horizontalLine; ++hi) {
            glm::vec3 pos{glm::sin(glm::pi<float>() * float(hi) / float(horizontalLine)) *
                              glm::cos(2 * glm::pi<float>() * float(vi) / float(verticalLine)),
                          glm::sin(glm::pi<float>() * float(hi) / float(horizontalLine)) *
                              glm::sin(2 * glm::pi<float>() * float(vi) / float(verticalLine)),
                          glm::cos(glm::pi<float>() * float(hi) / float(horizontalLine))};
            _modelData.vertex.push_back(
                {pos * radius,
                 pos,
                 {float(vi) / float(verticalLine), float(hi) / float(horizontalLine)}});
        }
    }
    // indices and tangent bitangent
    for (int vi = 0; vi < verticalLine; ++vi) {
        for (int hi = 0; hi < horizontalLine; ++hi) {  // notice here is < horizontalLine
            int curBase = vi * (horizontalLine + 1) + hi;
            int nextBase = vi == verticalLine - 1 ? hi : (vi + 1) * (horizontalLine + 1) + hi;
            _modelData.indices.push_back(curBase);
            _modelData.indices.push_back(curBase + 1);
            _modelData.indices.push_back(nextBase);
            generateTangentBitangent(curBase, curBase + 1, nextBase);
            _modelData.indices.push_back(curBase + 1);
            _modelData.indices.push_back(nextBase + 1);
            _modelData.indices.push_back(nextBase);
            generateTangentBitangent(curBase + 1, nextBase + 1, nextBase);
        }
    }

    int matId = createDefaultMat(color);
    _modelData.modelDataPartition.push_back(
        {0, static_cast<int>(_modelData.indices.size()), matId});
}

int MeshComponent::createDefaultMat(const glm::vec3 &color) {
    MaterialCpu matCpu{};
    matCpu.info.diffuse = glm::vec4{color, 1};
    return getEngine()->getRenderer()->createMaterial(matCpu);
}

void MeshComponent::uploadToGpu() {
    auto l = SLog::get();

    // Upload to GPU
    _modelState = getEngine()->getRenderer()->uploadModel(_modelData);
    if (_modelState == nullptr) {
        l->error("failed to upload modal data to gpu");
    }

    //    // TODO: free local resource
    //    stbi_image_free(_modelData.albedoTexture.stbRef);
    //    stbi_image_free(_modelData.normalTexture.stbRef);
}

}  // namespace luna