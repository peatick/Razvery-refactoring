en = create_entity()
pos = {
	x = 100,
	y = 500
}
ren = {
	w = 100,
	h = 100
}
add_newCom(en, pos, "pos")
add_newCom(en, ren, "Render_body")
data = {
	hp = 100,
	armor = 50
}
add_newLuaCom(en, data, "entity_data")



local MoveSystem = function(dt)
	for ent_id, com_data in pairs(get_entity_with("pos")) do
		if keyPressed("move_up") then
			com_data.pos.x = com_data.pos.x + dt * 100
		end
	end
end
Sys_Reg(MoveSystem)