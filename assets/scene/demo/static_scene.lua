function staticDemoScene()
    local l = luna.Log.new()
    l:info("static scene is running")

    --luna.NewStaticActor("assets/models/cube.obj")
    --luna.NewStaticActor("assets/models/mic.glb")
    luna.NewStaticActor("assets/models/box.glb")
end
