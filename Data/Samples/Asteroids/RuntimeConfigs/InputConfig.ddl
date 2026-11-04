InputAction
{
	string %Set{"Game"}
	string %Action{"TurnLeft"}
	bool %TimeScale{false}
	Slot
	{
		string %Key{"keyboard_left"}
		float %Scale{1}
	}
	Slot
	{
		string %Key{"keyboard_a"}
		float %Scale{1}
	}
	Slot
	{
		string %Key{"controller0_leftstick_negx"}
		float %Scale{1}
	}
}
InputAction
{
	string %Set{"Game"}
	string %Action{"TurnRight"}
	bool %TimeScale{false}
	Slot
	{
		string %Key{"keyboard_right"}
		float %Scale{1}
	}
	Slot
	{
		string %Key{"keyboard_d"}
		float %Scale{1}
	}
	Slot
	{
		string %Key{"controller0_leftstick_posx"}
		float %Scale{1}
	}
}
InputAction
{
	string %Set{"Game"}
	string %Action{"Thrust"}
	bool %TimeScale{false}
	Slot
	{
		string %Key{"keyboard_up"}
		float %Scale{1}
	}
	Slot
	{
		string %Key{"keyboard_w"}
		float %Scale{1}
	}
	Slot
	{
		string %Key{"controller0_right_trigger"}
		float %Scale{1}
	}
}
InputAction
{
	string %Set{"Game"}
	string %Action{"Fire"}
	bool %TimeScale{false}
	Slot
	{
		string %Key{"keyboard_space"}
		float %Scale{1}
	}
	Slot
	{
		string %Key{"controller0_button_a"}
		float %Scale{1}
	}
}
