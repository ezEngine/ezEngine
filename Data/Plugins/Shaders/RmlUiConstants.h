#include <Shaders/Common/GlobalConstants.h>

CONSTANT_BUFFER(ezRmlUiConstants, 4)
{
  MAT4(UiTransform);
  FLOAT2(UiTranslation);
  BOOL1(TextureNeedsAlphaMultiplication);
};

#define GRADIENT_MAX_NUM_STOPS 16u
#define GRADIENT_MAX_NUM_STOPS_PACKED 4u

#define GRADIENT_LINEAR 0
#define GRADIENT_RADIAL 1
#define GRADIENT_CONIC 2
#define GRADIENT_REPEATING_LINEAR 3
#define GRADIENT_REPEATING_RADIAL 4
#define GRADIENT_REPEATING_CONIC 5

CONSTANT_BUFFER(ezRmlUiAdditionalConstants, 5)
{
  UINT1(GradientFunc);     // one of the above definitions
  UINT1(GradientNumStops);
  UINT2(Padding);

  FLOAT2(GradientParams0); // linear: starting point,         radial: center,                        conic: center
  FLOAT2(GradientParams1); // linear: vector to ending point, radial: 2d curvature (inverse radius), conic: angled unit vector

  COLOR4F(GradientStopColors)[GRADIENT_MAX_NUM_STOPS];
  FLOAT4(GradientStopPositions)[GRADIENT_MAX_NUM_STOPS_PACKED]; // normalized, 0 -> starting point, 1 -> ending point
};

#define GRADIENT_GET_STOP_POS(i) (GradientStopPositions[i >> 2][i & 3])
