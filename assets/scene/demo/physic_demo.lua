function physicDemoScene()
    local l = luna.Log.new()
    l:info("physic scene is running")

    -- Procedural floor with bouciness physic
    local floorAct = luna.NewEmptyActor()
    local pos = glm.vec3.new(0, -4, 0)
    floorAct:setLocalPosition(pos)
    floorAct:setRotation(glm.angleAxis(glm.radians(-90), glm.vec3.new(1, 0, 0)))

    local meshComp = luna.NewMeshComponent(floorAct:getId());
    meshComp:generateSquarePlane(5, glm.vec3.new(0, 0.1, 0.9));
    meshComp:uploadToGpu();
    local rigidComp = luna.NewRigidBodyComponent(floorAct:getId());
    rigidComp:setIsStatic(true);
    rigidComp:setBounciness(0.8);
    rigidComp:createBox(glm.vec3.new(2.5, 0.1, 2.5));

    -- Procedural ball
    local ballAct = luna.NewEmptyActor()

    meshComp = luna.NewMeshComponent(ballAct:getId());
    meshComp:generateSphere(1, 30, 30, glm.vec3.new(0.3, 0.8, 0.1));
    meshComp:uploadToGpu();
    rigidComp = luna.NewRigidBodyComponent(ballAct:getId());
    rigidComp:setIsStatic(false);
    rigidComp:createSphere(1);
    rigidComp:setLinearVelocity(glm.vec3.new(0, -1, 0));
end





