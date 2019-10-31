#include <ProcGenPluginPCH.h>

#include <ProcGenPlugin/Declarations.h>
#include <ProcGenPlugin/VM/ExpressionByteCode.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezProcGenBlendMode, 1)
EZ_ENUM_CONSTANTS(ezProcGenBlendMode::Add, ezProcGenBlendMode::Subtract, ezProcGenBlendMode::Multiply, ezProcGenBlendMode::Divide)
EZ_ENUM_CONSTANTS(ezProcGenBlendMode::Max, ezProcGenBlendMode::Min)
EZ_ENUM_CONSTANTS(ezProcGenBlendMode::Set)
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

namespace ezProcGenInternal
{
  Output::~Output() = default;

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

  ezHashedString ExpressionOutputs::s_sR = ezMakeHashedString("R");
  ezHashedString ExpressionOutputs::s_sG = ezMakeHashedString("G");
  ezHashedString ExpressionOutputs::s_sB = ezMakeHashedString("B");
  ezHashedString ExpressionOutputs::s_sA = ezMakeHashedString("A");
}
