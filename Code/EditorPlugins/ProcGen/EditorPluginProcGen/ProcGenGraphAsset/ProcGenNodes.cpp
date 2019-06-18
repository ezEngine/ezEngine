#include <EditorPluginProcGenPCH.h>

#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenNodes.h>
#include <Foundation/Math/Random.h>

namespace
{
  static ezHashedString s_sRandom = ezMakeHashedString("Random");

  ezExpressionAST::Node* CreateRandom(float fSeed, ezExpressionAST& out_Ast)
  {
    auto pPointIndex = out_Ast.CreateInput(ezProcGenInternal::ExpressionInputs::s_sPointIndex);
    auto pSeedConstant = out_Ast.CreateConstant(fSeed);

    auto pSeed = out_Ast.CreateBinaryOperator(ezExpressionAST::NodeType::Add, pPointIndex, pSeedConstant);

    auto pFunctionCall = out_Ast.CreateFunctionCall(s_sRandom);
    pFunctionCall->m_Arguments.PushBack(pSeed);

    return pFunctionCall;
  }

  ezExpressionAST::Node* CreateRemapWithFadeout(
    ezExpressionAST::Node* pInput, float fMin, float fMax, float fFadeFraction, ezExpressionAST& out_Ast)
  {
    auto pOffset = out_Ast.CreateConstant(fMin);
    ezExpressionAST::Node* pValue = out_Ast.CreateBinaryOperator(ezExpressionAST::NodeType::Subtract, pInput, pOffset);

    auto pScale = out_Ast.CreateConstant(ezMath::Max(fMax - fMin, 0.0f) * fFadeFraction * 0.5f);
    pValue = out_Ast.CreateBinaryOperator(ezExpressionAST::NodeType::Divide, pValue, pScale);

    auto pFadeFactor = out_Ast.CreateConstant(1.0f / fFadeFraction);
    pValue = out_Ast.CreateBinaryOperator(ezExpressionAST::NodeType::Subtract, pValue, pFadeFactor);
    pValue = out_Ast.CreateUnaryOperator(ezExpressionAST::NodeType::Absolute, pValue);
    pValue = out_Ast.CreateBinaryOperator(ezExpressionAST::NodeType::Subtract, pFadeFactor, pValue);

    return pValue;
  }
} // namespace

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProcGenNodeBase, 1, ezRTTINoAllocator)
{
  flags.Add(ezTypeFlags::Abstract);
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProcGenOutput, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("Name", m_sName),
  }
  EZ_END_PROPERTIES;

  flags.Add(ezTypeFlags::Abstract);
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProcGenPlacementOutput, 1, ezRTTIDefaultAllocator<ezProcGenPlacementOutput>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ARRAY_MEMBER_PROPERTY("Objects", m_ObjectsToPlace)->AddAttributes(new ezAssetBrowserAttribute("Prefab")),
    EZ_MEMBER_PROPERTY("Footprint", m_fFootprint)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("MinOffset", m_vMinOffset),
    EZ_MEMBER_PROPERTY("MaxOffset", m_vMaxOffset),
    EZ_MEMBER_PROPERTY("AlignToNormal", m_fAlignToNormal)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_MEMBER_PROPERTY("MinScale", m_vMinScale)->AddAttributes(new ezDefaultValueAttribute(ezVec3(1.0f)), new ezClampValueAttribute(ezVec3(0.0f), ezVariant())),
    EZ_MEMBER_PROPERTY("MaxScale", m_vMaxScale)->AddAttributes(new ezDefaultValueAttribute(ezVec3(1.0f)), new ezClampValueAttribute(ezVec3(0.0f), ezVariant())),
    EZ_MEMBER_PROPERTY("ColorGradient", m_sColorGradient)->AddAttributes(new ezAssetBrowserAttribute("ColorGradient")),
    EZ_MEMBER_PROPERTY("CullDistance", m_fCullDistance)->AddAttributes(new ezDefaultValueAttribute(30.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
    EZ_MEMBER_PROPERTY("Surface", m_sSurface)->AddAttributes(new ezAssetBrowserAttribute("Surface")),

    EZ_MEMBER_PROPERTY("Density", m_DensityPin)->AddAttributes(new ezColorAttribute(ezColor::White)),
    EZ_MEMBER_PROPERTY("Scale", m_ScalePin)->AddAttributes(new ezColorAttribute(ezColor::LightCoral)),
    EZ_MEMBER_PROPERTY("ColorIndex", m_ColorIndexPin)->AddAttributes(new ezColorAttribute(ezColor::Orchid)),
    EZ_MEMBER_PROPERTY("ObjectIndex", m_ObjectIndexPin)->AddAttributes(new ezColorAttribute(ezColor::LightSkyBlue))
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezTitleAttribute("{Active} Placement Output: {Name}"),
    new ezCategoryAttribute("Output"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezExpressionAST::Node* ezProcGenPlacementOutput::GenerateExpressionASTNode(
  ezArrayPtr<ezExpressionAST::Node*> inputs, ezExpressionAST& out_Ast)
{
  out_Ast.m_OutputNodes.Clear();

  // density
  {
    auto pDensity = inputs[0];
    if (pDensity == nullptr)
    {
      pDensity = out_Ast.CreateConstant(1.0f);
    }

    out_Ast.m_OutputNodes.PushBack(out_Ast.CreateOutput(ezProcGenInternal::ExpressionOutputs::s_sDensity, pDensity));
  }

  // scale
  {
    auto pScale = inputs[1];
    if (pScale == nullptr)
    {
      pScale = CreateRandom(11.0f, out_Ast);
    }

    out_Ast.m_OutputNodes.PushBack(out_Ast.CreateOutput(ezProcGenInternal::ExpressionOutputs::s_sScale, pScale));
  }

  // color index
  {
    auto pColorIndex = inputs[2];
    if (pColorIndex == nullptr)
    {
      pColorIndex = CreateRandom(13.0f, out_Ast);
    }

    out_Ast.m_OutputNodes.PushBack(out_Ast.CreateOutput(ezProcGenInternal::ExpressionOutputs::s_sColorIndex, pColorIndex));
  }

  // object index
  {
    auto pObjectIndex = inputs[3];
    if (pObjectIndex == nullptr)
    {
      pObjectIndex = CreateRandom(17.0f, out_Ast);
    }

    out_Ast.m_OutputNodes.PushBack(out_Ast.CreateOutput(ezProcGenInternal::ExpressionOutputs::s_sObjectIndex, pObjectIndex));
  }

  return nullptr;
}

void ezProcGenPlacementOutput::Save(ezStreamWriter& stream)
{
  stream << m_sName;

  stream << m_ObjectsToPlace.GetCount();
  for (auto& object : m_ObjectsToPlace)
  {
    stream << object;
  }

  stream << m_fFootprint;

  stream << m_vMinOffset;
  stream << m_vMaxOffset;

  stream << m_fAlignToNormal;

  stream << m_vMinScale;
  stream << m_vMaxScale;

  stream << m_fCullDistance;

  stream << m_uiCollisionLayer;

  stream << m_sColorGradient;

  stream << m_uiByteCodeIndex;

  // chunk version 3
  stream << m_sSurface;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProcGenVertexColorOutput, 1, ezRTTIDefaultAllocator<ezProcGenVertexColorOutput>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("R", m_RPin)->AddAttributes(new ezColorAttribute(ezColor::LightCoral)),
    EZ_MEMBER_PROPERTY("G", m_GPin)->AddAttributes(new ezColorAttribute(ezColor::LightGreen)),
    EZ_MEMBER_PROPERTY("B", m_BPin)->AddAttributes(new ezColorAttribute(ezColor::LightSkyBlue)),
    EZ_MEMBER_PROPERTY("A", m_APin)->AddAttributes(new ezColorAttribute(ezColor::White))
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezTitleAttribute("{Active} Vertex Color Output: {Name}"),
    new ezCategoryAttribute("Output"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezExpressionAST::Node* ezProcGenVertexColorOutput::GenerateExpressionASTNode(
  ezArrayPtr<ezExpressionAST::Node*> inputs, ezExpressionAST& out_Ast)
{
  out_Ast.m_OutputNodes.Clear();

  ezHashedString sOutputNames[4] = {ezProcGenInternal::ExpressionOutputs::s_sR, ezProcGenInternal::ExpressionOutputs::s_sG,
    ezProcGenInternal::ExpressionOutputs::s_sB, ezProcGenInternal::ExpressionOutputs::s_sA};

  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(sOutputNames); ++i)
  {
    auto pInput = inputs[i];
    if (pInput == nullptr)
    {
      pInput = out_Ast.CreateConstant(0.0f);
    }

    out_Ast.m_OutputNodes.PushBack(out_Ast.CreateOutput(sOutputNames[i], pInput));
  }

  return nullptr;
}

void ezProcGenVertexColorOutput::Save(ezStreamWriter& stream)
{
  stream << m_sName;

  stream << m_uiByteCodeIndex;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProcGenRandom, 1, ezRTTIDefaultAllocator<ezProcGenRandom>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Seed", m_iSeed)->AddAttributes(new ezClampValueAttribute(-1, ezVariant()), new ezDefaultValueAttribute(-1), new ezMinValueTextAttribute("Auto")),

    EZ_MEMBER_PROPERTY("Value", m_OutputValuePin)
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_FUNCTION_PROPERTY(OnObjectCreated),
  }
  EZ_END_FUNCTIONS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezTitleAttribute("Random: {Seed}"),
    new ezCategoryAttribute("Input"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezExpressionAST::Node* ezProcGenRandom::GenerateExpressionASTNode(ezArrayPtr<ezExpressionAST::Node*> inputs, ezExpressionAST& out_Ast)
{
  ezRandom rnd;
  rnd.Initialize(m_iSeed < 0 ? m_uiAutoSeed : m_iSeed);

  auto pRandom = CreateRandom(rnd.FloatMinMax(0.0f, 100000.0f), out_Ast);
  return pRandom;
}

void ezProcGenRandom::OnObjectCreated(const ezAbstractObjectNode& node)
{
  m_uiAutoSeed = ezHashHelper<ezUuid>::Hash(node.GetGuid());
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProcGenBlend, 1, ezRTTIDefaultAllocator<ezProcGenBlend>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("InputA", m_fInputValueA)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("InputB", m_fInputValueB)->AddAttributes(new ezDefaultValueAttribute(1.0f)),

    EZ_MEMBER_PROPERTY("A", m_InputValueAPin),
    EZ_MEMBER_PROPERTY("B", m_InputValueBPin),
    EZ_MEMBER_PROPERTY("Value", m_OutputValuePin)
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezTitleAttribute("Blend: {A} * {B}"),
    new ezCategoryAttribute("Math"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezExpressionAST::Node* ezProcGenBlend::GenerateExpressionASTNode(ezArrayPtr<ezExpressionAST::Node*> inputs, ezExpressionAST& out_Ast)
{
  auto pInputA = inputs[0];
  if (pInputA == nullptr)
  {
    pInputA = out_Ast.CreateConstant(m_fInputValueA);
  }

  auto pInputB = inputs[1];
  if (pInputB == nullptr)
  {
    pInputB = out_Ast.CreateConstant(m_fInputValueB);
  }

  auto pBlend = out_Ast.CreateBinaryOperator(ezExpressionAST::NodeType::Multiply, pInputA, pInputB);
  return pBlend;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProcGenHeight, 1, ezRTTIDefaultAllocator<ezProcGenHeight>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("MinHeight", m_fMinHeight)->AddAttributes(new ezDefaultValueAttribute(0.0f)),
    EZ_MEMBER_PROPERTY("MaxHeight", m_fMaxHeight)->AddAttributes(new ezDefaultValueAttribute(1000.0f)),
    EZ_MEMBER_PROPERTY("FadeFraction", m_fFadeFraction)->AddAttributes(new ezDefaultValueAttribute(0.2f), new ezClampValueAttribute(0.01f, 1.0f)),

    EZ_MEMBER_PROPERTY("Value", m_OutputValuePin)
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezTitleAttribute("Height: [{MinHeight}, {MaxHeight}]"),
    new ezCategoryAttribute("Input"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezExpressionAST::Node* ezProcGenHeight::GenerateExpressionASTNode(ezArrayPtr<ezExpressionAST::Node*> inputs, ezExpressionAST& out_Ast)
{
  auto pHeight = out_Ast.CreateInput(ezProcGenInternal::ExpressionInputs::s_sPositionZ);
  return CreateRemapWithFadeout(pHeight, m_fMinHeight, m_fMaxHeight, m_fFadeFraction, out_Ast);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProcGenSlope, 1, ezRTTIDefaultAllocator<ezProcGenSlope>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("MinSlope", m_MinSlope)->AddAttributes(new ezDefaultValueAttribute(ezAngle::Degree(0.0f))),
    EZ_MEMBER_PROPERTY("MaxSlope", m_MaxSlope)->AddAttributes(new ezDefaultValueAttribute(ezAngle::Degree(60.0f))),
    EZ_MEMBER_PROPERTY("FadeFraction", m_fFadeFraction)->AddAttributes(new ezDefaultValueAttribute(0.2f), new ezClampValueAttribute(0.01f, 1.0f)),

    EZ_MEMBER_PROPERTY("Value", m_OutputValuePin)
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezTitleAttribute("Slope: [{MinSlope}, {MaxSlope}]"),
    new ezCategoryAttribute("Input"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezExpressionAST::Node* ezProcGenSlope::GenerateExpressionASTNode(ezArrayPtr<ezExpressionAST::Node*> inputs, ezExpressionAST& out_Ast)
{
  auto pNormalZ = out_Ast.CreateInput(ezProcGenInternal::ExpressionInputs::s_sNormalZ);
  auto pAngle = out_Ast.CreateUnaryOperator(ezExpressionAST::NodeType::ACos, pNormalZ);
  return CreateRemapWithFadeout(pAngle, m_MinSlope.GetRadian(), m_MaxSlope.GetRadian(), m_fFadeFraction, out_Ast);
}
