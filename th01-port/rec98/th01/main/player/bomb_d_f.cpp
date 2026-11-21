bool16 bomb_deals_damage(uint32_t frame)
{
	if((rem_lives == 0) && (bomb_damaging == true) && ((frame % 50) == 0)) {
		return true;
	}
	return false;
}
