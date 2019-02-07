#include <PCH.h>

#include <ProceduralPlacementPlugin/ProceduralPlacementPluginDLL.h>

namespace ezPPInternal
{
  ezHashedString ExpressionInputs::s_sPositionX = ezMakeHashedString("PositionX");
  ezHashedString ExpressionInputs::s_sPositionY = ezMakeHashedString("PositionY");
  ezHashedString ExpressionInputs::s_sPositionZ = ezMakeHashedString("PositionZ");
  ezHashedString ExpressionInputs::s_sNormalX = ezMakeHashedString("NormalX");
  ezHashedString ExpressionInputs::s_sNormalY = ezMakeHashedString("NormalY");
  ezHashedString ExpressionInputs::s_sNormalZ = ezMakeHashedString("NormalZ");
  ezHashedString ExpressionInputs::s_sPointIndex = ezMakeHashedString("PointIndex");


  ezHashedString ExpressionOutputs::s_sDensity = ezMakeHashedString("Density");
  ezHashedString ExpressionOutputs::s_sScale = ezMakeHashedString("Scale");
  ezHashedString ExpressionOutputs::s_sColorIndex = ezMakeHashedString("ColorIndex");
  ezHashedString ExpressionOutputs::s_sObjectIndex = ezMakeHashedString("ObjectIndex");
}
