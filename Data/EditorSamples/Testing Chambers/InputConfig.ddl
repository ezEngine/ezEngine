InputAction
{
	string %Set{"Player"}
	string %Action{"MoveForwards"}
	bool %TimeScale{false}
	Slot
	{
		string %Key{"keyboard_w"}
		float %Scale{1}
	}
}
InputAction
{
	string %Set{"Player"}
	string %Action{"MoveBackwards"}
	bool %TimeScale{false}
	Slot
	{
		string %Key{"keyboard_s"}
		float %Scale{1}
	}
}
InputAction
{
	string %Set{"Player"}
	string %Action{"StrafeLeft"}
	bool %TimeScale{false}
	Slot
	{
		string %Key{"keyboard_a"}
		float %Scale{1}
	}
}
InputAction
{
	string %Set{"Player"}
	string %Action{"StrafeRight"}
	bool %TimeScale{false}
	Slot
	{
		string %Key{"keyboard_d"}
		float %Scale{1}
	}
}
InputAction
{
	string %Set{"Player"}
	string %Action{"Jump"}
	bool %TimeScale{false}
	Slot
	{
		string %Key{"keyboard_space"}
		float %Scale{1}
	}
}
InputAction
{
	string %Set{"Player"}
	string %Action{"Run"}
	bool %TimeScale{false}
	Slot
	{
		string %Key{"keyboard_left_shift"}
		float %Scale{1}
	}
}
InputAction
{
	string %Set{"Player"}
	string %Action{"RotateLeft"}
	bool %TimeScale{true}
	Slot
	{
		string %Key{"keyboard_left"}
		float %Scale{1}
	}
	Slot
	{
		string %Key{"mouse_move_negx"}
		float %Scale{1}
	}
}
InputAction
{
	string %Set{"Player"}
	string %Action{"RotateRight"}
	bool %TimeScale{true}
	Slot
	{
		string %Key{"keyboard_right"}
		float %Scale{1}
	}
	Slot
	{
		string %Key{"mouse_move_posx"}
		float %Scale{1}
	}
}
InputAction
{
	string %Set{"Player"}
	string %Action{"Use"}
	bool %TimeScale{false}
	Slot
	{
		string %Key{"keyboard_e"}
		float %Scale{1}
	}
}
InputAction
{
	string %Set{"Player"}
	string %Action{"Shoot"}
	bool %TimeScale{false}
	Slot
	{
		string %Key{"mouse_button_0"}
		float %Scale{1}
	}
}
