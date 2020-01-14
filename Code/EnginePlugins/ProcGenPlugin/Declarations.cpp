#include <ProcGenPluginPCH.h>

#include <ProcGenPlugin/Declarations.h>
#include <ProcGenPlugin/VM/ExpressionByteCode.h>

// clang-format off
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
// clang-format on

static ezTypeVersion s_ProcVertexColorMappingVersion = 1;
ezResult ezProcVertexColorMapping::Serialize(ezStreamWriter& stream) const
{
  stream.WriteVersion(s_ProcVertexColorMappingVersion);
  stream << m_R;
  stream << m_G;
  stream << m_B;
  stream << m_A;

  return EZ_SUCCESS;
}

ezResult ezProcVertexColorMapping::Deserialize(ezStreamReader& stream)
{
  /*ezTypeVersion version =*/stream.ReadVersion(s_ProcVertexColorMappingVersion);
  stream >> m_R;
  stream >> m_G;
  stream >> m_B;
  stream >> m_A;

  return EZ_SUCCESS;
}

namespace ezProcGenInternal
{
  Output::~Output() = default;

  ezHashedString ExpressionInputs::s_sPositionX = ezMakeHashedString("PositionX");
  ezHashedString ExpressionInputs::s_sPositionY = ezMakeHashedString("PositionY");
  ezHashedString ExpressionInputs::s_sPositionZ = ezMakeHashedString("PositionZ");
  ezHashedString ExpressionInputs::s_sNormalX = ezMakeHashedString("NormalX");
  ezHashedString ExpressionInputs::s_sNormalY = ezMakeHashedString("NormalY");
  ezHashedString ExpressionInputs::s_sNormalZ = ezMakeHashedString("NormalZ");
  ezHashedString ExpressionInputs::s_sColorR = ezMakeHashedString("ColorR");
  ezHashedString ExpressionInputs::s_sColorG = ezMakeHashedString("ColorG");
  ezHashedString ExpressionInputs::s_sColorB = ezMakeHashedString("ColorB");
  ezHashedString ExpressionInputs::s_sColorA = ezMakeHashedString("ColorA");
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
