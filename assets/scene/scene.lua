require('assets.scene.demo')

function init()
    local l = luna.Log.new()
    l:info("lua script is running")

    -- Select Shader
    luna.SetGraphicBgColor(glm.vec3.new(1, 0, 0))

    -- Camera
    local camAct = luna.NewCameraActor()
    local pos = glm.vec3.new(0, -0.5, 3)
    camAct:setLocalPosition(pos)
    camAct:setRotation(glm.angleAxis(glm.radians(0), glm.vec3.new(1, 0, 0)))

    -- Default point light
    luna.SetGraphicSunlight(glm.vec3.new(1, -1, 0), glm.vec3.new(1, 1, 1))
    -- local pointLight = luna.NewPointLightActor(glm.vec3.new(1, 1, 1), 20)
    -- pointLight:setLocalPosition(glm.vec3.new(0, 5, 0))

    -- Maybe do some additional configuration here like
    -- setting up screen resolution etc.
end

init()

-- Select demo here
staticDemoScene()
--physicDemoScene()

