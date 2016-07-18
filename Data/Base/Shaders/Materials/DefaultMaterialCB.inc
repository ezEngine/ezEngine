#include <Shaders/Common/GlobalConstants.h>

CONSTANT_BUFFER(MaterialConstants, 1)
{
	COLOR(BaseColor);
	FLOAT1(MetallicValue);
	FLOAT1(ReflectanceValue);
	FLOAT1(RoughnessValue);
	FLOAT1(MaskThreshold);
};

