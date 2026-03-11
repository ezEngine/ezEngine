InputAction
{
	string %Set{"Default"}
	string %Action{"Up"}
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
	string %Set{"Default"}
	string %Action{"Down"}
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
InputAction
{
	string %Set{"Default"}
	string %Action{"Left"}
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
	string %Set{"Default"}
	string %Action{"Right"}
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
