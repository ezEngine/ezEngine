module ez.Script.Uda;

template hasAttribute(alias sym, T)
{
	static bool helper()
	{
		foreach(a; __traits(getAttributes, sym))
		{
			if(is(a == T) || __traits(compiles, typeof(a)) && is(typeof(a) == T))
				return true;
		}
		return false;
	}
	enum bool hasAttribute = helper();
}

template getAttribute(alias sym, T)
{
	static T helper()
	{
		foreach(a; __traits(getAttributes, sym))
		{
			static if(is(a == T) || __traits(compiles, typeof(a)) && is(typeof(a) == T))
				return a;
		}
		assert(0, "attribute " ~ T.stringof ~ " not found");
	}
	enum T getAttribute = helper();
}