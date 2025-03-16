require('assets.scene.demo')

function init()
    local l = luna.Log.new()
    l:info("lua script is running")

    -- Camera
    local camAct = luna.NewCameraActor()
    local pos = glm.vec3.new(0, -0.5, 3)
    camAct:setLocalPosition(pos)
    camAct:setRotation(glm.angleAxis(glm.radians(0), glm.vec3.new(1, 0, 0)))

    -- Maybe do some additional configuration here like
    -- setting up screen resolution etc.
end

init()

-- Select demo here
staticDemoScene()
--physicDemoScene()

