-- Prepare scene here, it's an execute single time script
l = luna.Log.new()
l:info("lua world scene script is running")
--l:error("lua erro hello")

--  them are obtained through Getxxx / Newxxx which CPP will help in managing lifecycle

-- Camera
camAct = luna.NewCameraActor()
pos = glm.vec3.new(0, 0.5, 3)
camAct:setLocalPosition(pos)


