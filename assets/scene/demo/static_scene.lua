function staticDemoScene()
    local l = luna.Log.new()
    l:info("script static scene is running")

    luna.NewStaticActor("assets/models/cube.obj")
    -- luna.NewStaticActor("assets/models/mic.glb")
    -- luna.NewStaticActor("assets/models/miniature_night_city.glb")
    -- luna.NewStaticActor("assets/models/2CylinderEngine.glb")
    -- local gun = luna.NewStaticActor("assets/models/gun.glb")
    -- gun:setScale(4);
    -- gun:setLocalPosition(glm.vec3.new(0, -4, 1));
    -- gun:setRotation(glm.angleAxis(glm.radians(90), glm.vec3.new(0, 1, 0)))
    -- luna.NewStaticActor("assets/models/red_squid_game.glb")

    -- local ganyu = luna.NewStaticActor("assets/models/ganyu.obj")
    -- ganyu:setScale(0.1);
end
