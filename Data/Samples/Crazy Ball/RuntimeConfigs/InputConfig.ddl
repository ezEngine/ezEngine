InputAction
{
	string %Set{"BallGame"}
	string %Action{"Start"}
	bool %TimeScale{false}
	Slot
	{
		string %Key{"keyboard_space"}
		float %Scale{1}
	}
	Slot
	{
		string %Key{"keyboard_return"}
		float %Scale{1}
	}
	Slot
	{
		string %Key{"controller0_button_start"}
		float %Scale{1}
	}
}
InputAction
{
	string %Set{"BallGame"}
	string %Action{"MoveLeft"}
	bool %TimeScale{false}
	Slot
	{
		string %Key{"keyboard_left"}
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
	string %Set{"BallGame"}
	string %Action{"MoveRight"}
	bool %TimeScale{false}
	Slot
	{
		string %Key{"keyboard_right"}
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
	string %Set{"BallGame"}
	string %Action{"MoveUp"}
	bool %TimeScale{false}
	Slot
	{
		string %Key{"keyboard_up"}
		float %Scale{1}
	}
	Slot
	{
		string %Key{"controller0_leftstick_posy"}
		float %Scale{1}
	}
}
InputAction
{
	string %Set{"BallGame"}
	string %Action{"MoveDown"}
	bool %TimeScale{false}
	Slot
	{
		string %Key{"keyboard_down"}
		float %Scale{1}
	}
	Slot
	{
		string %Key{"controller0_leftstick_negy"}
		float %Scale{1}
	}
}
