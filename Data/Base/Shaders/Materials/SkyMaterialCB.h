#include <Shaders/Common/GlobalConstants.h>

CONSTANT_BUFFER(ezMaterialConstants, 1)
{
	COLOR4F(BaseColor);
	FLOAT1(MaskThreshold);
	FLOAT1(ExposureBias);
	BOOL(InverseTonemap);
};

