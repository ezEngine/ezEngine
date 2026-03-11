WindowDesc
{
	string %Title{"RTS Sample"}
	string %Mode{"ResizableWindow"}
	int8 %Monitor{1}
	Vec2u %Resolution{uint32{1920,1080}}
	bool %ClipMouseCursor{false}
	bool %ShowMouseCursor{false}
	bool %SetForegroundOnInit{true}
	bool %CenterWindowOnDisplay{true}
}
