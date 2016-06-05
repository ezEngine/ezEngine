#include <Shaders/Common/GlobalConstants.h>

CONSTANT_BUFFER(MaterialCB, 1)
{
	COLOR(DiffuseColor);
	FLOAT1(Metallic);
	FLOAT1(Reflectance);
	FLOAT1(Roughness);
	FLOAT1(MaskThreshold);
};

