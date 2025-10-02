#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <ProcGenPlugin/Declarations.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezProcGenBinaryOperator, 1)
  EZ_ENUM_CONSTANTS(ezProcGenBinaryOperator::Add, ezProcGenBinaryOperator::Subtract, ezProcGenBinaryOperator::Multiply, ezProcGenBinaryOperator::Divide)
  EZ_ENUM_CONSTANTS(ezProcGenBinaryOperator::Max, ezProcGenBinaryOperator::Min)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezProcGenBlendMode, 1)
  EZ_ENUM_CONSTANTS(ezProcGenBlendMode::Add, ezProcGenBlendMode::Subtract, ezProcGenBlendMode::Multiply, ezProcGenBlendMode::Divide)
  EZ_ENUM_CONSTANTS(ezProcGenBlendMode::Max, ezProcGenBlendMode::Min)
  EZ_ENUM_CONSTANTS(ezProcGenBlendMode::Set)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezProcVertexColorChannelMapping, 1)
  EZ_ENUM_CONSTANTS(ezProcVertexColorChannelMapping::R, ezProcVertexColorChannelMapping::G, ezProcVertexColorChannelMapping::B, ezProcVertexColorChannelMapping::A)
  EZ_ENUM_CONSTANTS(ezProcVertexColorChannelMapping::Black, ezProcVertexColorChannelMapping::White)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezProcVertexColorMapping, ezNoBase, 1, ezRTTIDefaultAllocator<ezProcVertexColorMapping>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("R", ezProcVertexColorChannelMapping, m_R)->AddAttributes(new ezDefaultValueAttribute(ezProcVertexColorChannelMapping::R)),
    EZ_ENUM_MEMBER_PROPERTY("G", ezProcVertexColorChannelMapping, m_G)->AddAttributes(new ezDefaultValueAttribute(ezProcVertexColorChannelMapping::G)),
    EZ_ENUM_MEMBER_PROPERTY("B", ezProcVertexColorChannelMapping, m_B)->AddAttributes(new ezDefaultValueAttribute(ezProcVertexColorChannelMapping::B)),
    EZ_ENUM_MEMBER_PROPERTY("A", ezProcVertexColorChannelMapping, m_A)->AddAttributes(new ezDefaultValueAttribute(ezProcVertexColorChannelMapping::A)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezProcPlacementMode, 1)
  EZ_ENUM_CONSTANTS(ezProcPlacementMode::Raycast, ezProcPlacementMode::RaycastHighQuality, ezProcPlacementMode::Fixed)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezProcPlacementPattern, 1)
  EZ_ENUM_CONSTANTS(ezProcPlacementPattern::RegularGrid, ezProcPlacementPattern::HexGrid, ezProcPlacementPattern::Natural)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezProcVolumeImageMode, 1)
  EZ_ENUM_CONSTANTS(ezProcVolumeImageMode::ReferenceColor, ezProcVolumeImageMode::ChannelR, ezProcVolumeImageMode::ChannelG, ezProcVolumeImageMode::ChannelB, ezProcVolumeImageMode::ChannelA)
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

static ezTypeVersion s_ProcVertexColorMappingVersion = 1;
ezResult ezProcVertexColorMapping::Serialize(ezStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(s_ProcVertexColorMappingVersion);
  inout_stream << m_R;
  inout_stream << m_G;
  inout_stream << m_B;
  inout_stream << m_A;

  return EZ_SUCCESS;
}

ezResult ezProcVertexColorMapping::Deserialize(ezStreamReader& inout_stream)
{
  /*ezTypeVersion version =*/inout_stream.ReadVersion(s_ProcVertexColorMappingVersion);
  inout_stream >> m_R;
  inout_stream >> m_G;
  inout_stream >> m_B;
  inout_stream >> m_A;

  return EZ_SUCCESS;
}

namespace ezProcGenInternal
{
  GraphSharedDataBase::~GraphSharedDataBase() = default;
  Output::~Output() = default;

  ezHashedString ExpressionInputs::s_sPosition = ezMakeHashedString("position");
  ezHashedString ExpressionInputs::s_sPositionX = ezMakeHashedString("position.x");
  ezHashedString ExpressionInputs::s_sPositionY = ezMakeHashedString("position.y");
  ezHashedString ExpressionInputs::s_sPositionZ = ezMakeHashedString("position.z");
  ezHashedString ExpressionInputs::s_sNormal = ezMakeHashedString("normal");
  ezHashedString ExpressionInputs::s_sNormalX = ezMakeHashedString("normal.x");
  ezHashedString ExpressionInputs::s_sNormalY = ezMakeHashedString("normal.y");
  ezHashedString ExpressionInputs::s_sNormalZ = ezMakeHashedString("normal.z");
  ezHashedString ExpressionInputs::s_sColor = ezMakeHashedString("color");
  ezHashedString ExpressionInputs::s_sColorR = ezMakeHashedString("color.x");
  ezHashedString ExpressionInputs::s_sColorG = ezMakeHashedString("color.y");
  ezHashedString ExpressionInputs::s_sColorB = ezMakeHashedString("color.z");
  ezHashedString ExpressionInputs::s_sColorA = ezMakeHashedString("color.w");
  ezHashedString ExpressionInputs::s_sPointIndex = ezMakeHashedString("pointIndex");

  ezHashedString ExpressionOutputs::s_sOutDensity = ezMakeHashedString("outDensity");
  ezHashedString ExpressionOutputs::s_sOutScale = ezMakeHashedString("outScale");
  ezHashedString ExpressionOutputs::s_sOutColorIndex = ezMakeHashedString("outColorIndex");
  ezHashedString ExpressionOutputs::s_sOutObjectIndex = ezMakeHashedString("outObjectIndex");

  ezHashedString ExpressionOutputs::s_sOutColor = ezMakeHashedString("outColor");
  ezHashedString ExpressionOutputs::s_sOutColorR = ezMakeHashedString("outColor.x");
  ezHashedString ExpressionOutputs::s_sOutColorG = ezMakeHashedString("outColor.y");
  ezHashedString ExpressionOutputs::s_sOutColorB = ezMakeHashedString("outColor.z");
  ezHashedString ExpressionOutputs::s_sOutColorA = ezMakeHashedString("outColor.w");
} // namespace ezProcGenInternal


EZ_STATICLINK_FILE(ProcGenPlugin, ProcGenPlugin_Declarations);
