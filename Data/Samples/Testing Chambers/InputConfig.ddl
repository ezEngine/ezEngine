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
	Slot
	{
		string %Key{"controller0_leftstick_posy"}
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
	Slot
	{
		string %Key{"controller0_leftstick_negy"}
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
	Slot
	{
		string %Key{"controller0_leftstick_negx"}
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
	Slot
	{
		string %Key{"controller0_leftstick_posx"}
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
	Slot
	{
		string %Key{"controller0_button_a"}
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
	Slot
	{
		string %Key{"controller0_button_x"}
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
		string %Key{"controller0_rightstick_negx"}
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
		string %Key{"controller0_rightstick_posx"}
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
	Slot
	{
		string %Key{"controller0_button_b"}
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
	Slot
	{
		string %Key{"controller0_right_trigger"}
		float %Scale{1}
	}
}
InputAction
{
	string %Set{"Player"}
	string %Action{"Crouch"}
	bool %TimeScale{false}
	Slot
	{
		string %Key{"keyboard_left_ctrl"}
		float %Scale{1}
	}
	Slot
	{
		string %Key{"controller0_left_shoulder"}
		float %Scale{1}
	}
}
InputAction
{
	string %Set{"Player"}
	string %Action{"LookUp"}
	bool %TimeScale{true}
	Slot
	{
		string %Key{"keyboard_up"}
		float %Scale{1}
	}
	Slot
	{
		string %Key{"controller0_rightstick_posy"}
		float %Scale{1}
	}
	Slot
	{
		string %Key{"mouse_move_negy"}
		float %Scale{1}
	}
}
InputAction
{
	string %Set{"Player"}
	string %Action{"LookDown"}
	bool %TimeScale{true}
	Slot
	{
		string %Key{"keyboard_down"}
		float %Scale{1}
	}
	Slot
	{
		string %Key{"controller0_rightstick_negy"}
		float %Scale{1}
	}
	Slot
	{
		string %Key{"mouse_move_posy"}
		float %Scale{1}
	}
}
InputAction
{
	string %Set{"Player"}
	string %Action{"Flashlight"}
	bool %TimeScale{false}
	Slot
	{
		string %Key{"keyboard_f"}
		float %Scale{1}
	}
	Slot
	{
		string %Key{"controller0_button_y"}
		float %Scale{1}
	}
}
InputAction
{
	string %Set{"Player"}
	string %Action{"SwitchWeapon1"}
	bool %TimeScale{true}
	Slot
	{
		string %Key{"keyboard_1"}
		float %Scale{1}
	}
}
InputAction
{
	string %Set{"Player"}
	string %Action{"SwitchWeapon2"}
	bool %TimeScale{true}
	Slot
	{
		string %Key{"keyboard_2"}
		float %Scale{1}
	}
}
InputAction
{
	string %Set{"Player"}
	string %Action{"SwitchWeapon3"}
	bool %TimeScale{true}
	Slot
	{
		string %Key{"keyboard_3"}
		float %Scale{1}
	}
}
