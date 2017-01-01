InputAction
{
	string %Set{"Game"}
	string %Action{"MoveForwards"}
	bool %TimeScale{true}
	Slot
	{
		string %Key{"keyboard_w"}
		float %Scale{1}
	}
}
InputAction
{
	string %Set{"Game"}
	string %Action{"MoveBackwards"}
	bool %TimeScale{true}
	Slot
	{
		string %Key{"keyboard_s"}
		float %Scale{1}
	}
}
InputAction
{
	string %Set{"Game"}
	string %Action{"MoveLeft"}
	bool %TimeScale{true}
	Slot
	{
		string %Key{"keyboard_a"}
		float %Scale{1}
	}
}
InputAction
{
	string %Set{"Game"}
	string %Action{"MoveRight"}
	bool %TimeScale{true}
	Slot
	{
		string %Key{"keyboard_d"}
		float %Scale{1}
	}
}
InputAction
{
	string %Set{"Game"}
	string %Action{"MoveUp"}
	bool %TimeScale{true}
	Slot
	{
		string %Key{"keyboard_q"}
		float %Scale{1}
	}
}
InputAction
{
	string %Set{"Game"}
	string %Action{"MoveDown"}
	bool %TimeScale{true}
	Slot
	{
		string %Key{"keyboard_e"}
		float %Scale{1}
	}
}
InputAction
{
	string %Set{"Game"}
	string %Action{"Run"}
	bool %TimeScale{true}
	Slot
	{
		string %Key{"keyboard_left_shift"}
		float %Scale{1}
	}
}
InputAction
{
	string %Set{"Game"}
	string %Action{"TurnLeft"}
	bool %TimeScale{true}
	Slot
	{
		string %Key{"keyboard_left"}
		float %Scale{1}
	}
	Slot
	{
		string %Key{"mouse_move_negx"}
		float %Scale{0.5}
	}
}
InputAction
{
	string %Set{"Game"}
	string %Action{"TurnRight"}
	bool %TimeScale{true}
	Slot
	{
		string %Key{"keyboard_right"}
		float %Scale{1}
	}
	Slot
	{
		string %Key{"mouse_move_posx"}
		float %Scale{0.5}
	}
}
InputAction
{
	string %Set{"Game"}
	string %Action{"TurnUp"}
	bool %TimeScale{true}
	Slot
	{
		string %Key{"keyboard_up"}
		float %Scale{1}
	}
	Slot
	{
		string %Key{"mouse_move_negy"}
		float %Scale{0.5}
	}
}
InputAction
{
	string %Set{"Game"}
	string %Action{"TurnDown"}
	bool %TimeScale{true}
	Slot
	{
		string %Key{"keyboard_down"}
		float %Scale{1}
	}
	Slot
	{
		string %Key{"mouse_move_posy"}
		float %Scale{0.5}
	}
}
