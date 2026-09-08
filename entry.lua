en = create_entity()
pos = {
	x = 100,
	y = 500
}
ren = {
	w = 100,
	h = 100
}
data = {
	hp = 100,
	armor = 50
}
add_newCom(en, pos, "pos")
add_newCom(en, ren, "Render_body")
add_newLuaCom(en, data, "entity_data")

local MoveSystem = function(dt)
	for_each({"pos","entity_data"},function(com_data)
		if keyPressed("forw") then
			com_data.pos.x = com_data.pos.x + dt * 10
		end
		if keyPressed("back") then
			print("down")
		end
	end)	
end
Sys_Reg(MoveSystem)
