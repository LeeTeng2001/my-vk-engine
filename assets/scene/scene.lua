require('assets.scene.demo')

function init()
    -- Camera
    local camAct = luna.NewCameraActor()
    local pos = glm.vec3.new(0, -0.5, 3)
    camAct:setLocalPosition(pos)

    -- Maybe do some additional configuration here like
    -- setting up screen resolution etc.
end

init()

-- Select demo here
staticDemoScene()
--physicDemoScene()

