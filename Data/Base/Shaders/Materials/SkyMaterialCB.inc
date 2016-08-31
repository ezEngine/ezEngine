#include <Shaders/Common/GlobalConstants.h>

CONSTANT_BUFFER(MaterialConstants, 1)
{
	COLOR(BaseColor);
	FLOAT1(MaskThreshold);
	FLOAT1(ExposureBias);
	BOOL(InverseTonemap);
};

