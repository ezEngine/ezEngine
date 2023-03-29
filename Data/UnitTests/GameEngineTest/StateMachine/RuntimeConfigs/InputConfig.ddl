InputAction
{
	string %Set{"Default"}
	string %Action{"Interact"}
	bool %TimeScale{false}
	Slot
	{
		string %Key{"keyboard_space"}
		float %Scale{1}
	}
	Slot
	{
		string %Key{"mouse_button_0"}
		float %Scale{1}
	}
	Slot
	{
		string %Key{"controller0_button_a"}
		float %Scale{1}
	}
}
