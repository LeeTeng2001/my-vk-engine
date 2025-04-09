-- New namespace ------------------------------
---@class luna.Log
luna.Log = {}

---@return luna.Log
function luna.Log.new() end

---@param message string
function luna.Log:debug(message) end

---@param message string
function luna.Log:info(message) end

---@param message string
function luna.Log:warn(message) end

---@param message string
function luna.Log:error(message) end

-- New namespace ------------------------------
---@class glm
glm = {}

---@class glm.vec3
---@field x number
---@field y number
---@field z number
---@operator add(glm.vec3): glm.vec3
---@operator sub(glm.vec3): glm.vec3
---@operator mul(number): glm.vec3
glm.vec3 = {}

---@param x number
---@param y number
---@param z number
---@return glm.vec3
function glm.vec3.new(x, y, z) end

---@class glm.quat
---@field x number
---@field y number
---@field z number
---@field w number
glm.quat = {}

---@param angle number
---@param axis glm.vec3
---@return glm.quat
function glm.angleAxis(angle, axis) end

---@param degree number
---@return number
function glm.radians(degree) end

-- New namespace ------------------------------
---@class luna
luna = {}

---@return Engine
function luna.GetEngine() end

---@param color glm.vec3
function luna.SetGraphicBgColor(color) end

---@param dir glm.vec3
---@param color glm.vec3
function luna.SetGraphicSunlight(dir, color) end

---@class luna.Actor
---@field id number
luna.Actor = {}

---@param position glm.vec3
function luna.Actor:setLocalPosition(position) end

---@return glm.vec3
function luna.Actor:getLocalPosition() end

---@param rotation glm.quat
function luna.Actor:setRotation(rotation) end

---@param scale glm.vec3
function luna.Actor:setScale(scale) end

---@return number
function luna.Actor:getId() end

---@class luna.EmptyActor : luna.Actor
luna.EmptyActor = {}

---@return luna.EmptyActor
function luna.NewEmptyActor() end

---@class luna.CameraActor : luna.Actor
luna.CameraActor = {}

---@return luna.CameraActor
function luna.NewCameraActor() end

---@class luna.StaticActor : luna.Actor
luna.StaticActor = {}

---@param modelPath string
---@return luna.StaticActor
function luna.NewStaticActor(modelPath) end

---@class luna.PointLightActor : luna.Actor
luna.PointLightActor = {}

---@param color glm.vec3
---@param radius number
---@return luna.PointLightActor
function luna.NewPointLightActor(color, radius) end

---@class luna.Component
luna.Component = {}

---@class luna.MeshComponent : luna.Component
luna.MeshComponent = {}

---@param actorId number
---@return luna.MeshComponent
function luna.NewMeshComponent(actorId) end

function luna.MeshComponent:generateSquarePlane() end

function luna.MeshComponent:generateSphere() end

---@param modelPath string
function luna.MeshComponent:loadModal(modelPath) end

function luna.MeshComponent:uploadToGpu() end

---@class luna.RigidBodyComponent : luna.Component
luna.RigidBodyComponent = {}

---@param actorId number
---@return luna.RigidBodyComponent
function luna.NewRigidBodyComponent(actorId) end

---@param isStatic boolean
function luna.RigidBodyComponent:setIsStatic(isStatic) end

---@param bounciness number
function luna.RigidBodyComponent:setBounciness(bounciness) end

function luna.RigidBodyComponent:createBox() end

function luna.RigidBodyComponent:createSphere() end

---@param velocity glm.vec3
function luna.RigidBodyComponent:setLinearVelocity(velocity) end

---@class luna.TweenComponent : luna.Component
luna.TweenComponent = {}

---@param actorId number
---@return luna.TweenComponent
function luna.NewTweenComponent(actorId) end

---@param offset glm.vec3
function luna.TweenComponent:addTranslateOffset(offset) end

---@param offset glm.quat
function luna.TweenComponent:addRotationOffset(offset) end

---@param loopType number
function luna.TweenComponent:setLoopType(loopType) end
