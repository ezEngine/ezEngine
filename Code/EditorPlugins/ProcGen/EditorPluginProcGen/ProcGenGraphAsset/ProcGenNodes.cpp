#include <EditorPluginProcGenPCH.h>

#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenNodes.h>
#include <Foundation/Math/Random.h>

namespace
{
  static ezHashedString s_sRandom = ezMakeHashedString("Random");
  static ezHashedString s_sPerlinNoise = ezMakeHashedString("PerlinNoise");
  static ezHashedString s_sApplyVolumes = ezMakeHashedString("ApplyVolumes");

  ezExpressionAST::NodeType::Enum GetBlendOperator(ezProcGenBlendMode::Enum blendMode)
  {
    switch (blendMode)
    {
      case ezProcGenBlendMode::Add:
        return ezExpressionAST::NodeType::Add;
      case ezProcGenBlendMode::Subtract:
        return ezExpressionAST::NodeType::Subtract;
      case ezProcGenBlendMode::Multiply:
        return ezExpressionAST::NodeType::Multiply;
      case ezProcGenBlendMode::Divide:
        return ezExpressionAST::NodeType::Divide;
      case ezProcGenBlendMode::Max:
        return ezExpressionAST::NodeType::Max;
      case ezProcGenBlendMode::Min:
        return ezExpressionAST::NodeType::Min;
      default:
        EZ_ASSERT_NOT_IMPLEMENTED;
        return ezExpressionAST::NodeType::Invalid;
    }
  }

  ezExpressionAST::Node* CreateRandom(float fSeed, ezExpressionAST& out_Ast)
  {
    auto pPointIndex = out_Ast.CreateInput(ezProcGenInternal::ExpressionInputs::s_sPointIndex);
    auto pSeedConstant = out_Ast.CreateConstant(fSeed);

    auto pSeed = out_Ast.CreateBinaryOperator(ezExpressionAST::NodeType::Add, pPointIndex, pSeedConstant);

    auto pFunctionCall = out_Ast.CreateFunctionCall(s_sRandom);
    pFunctionCall->m_Arguments.PushBack(pSeed);

    return pFunctionCall;
  }

  ezExpressionAST::Node* CreateRemapFrom01(ezExpressionAST::Node* pInput, float fMin, float fMax, ezExpressionAST& out_Ast)
  {
    auto pOffset = out_Ast.CreateConstant(fMin);
    auto pScale = out_Ast.CreateConstant(fMax - fMin);

    auto pValue = out_Ast.CreateBinaryOperator(ezExpressionAST::NodeType::Multiply, pInput, pScale);
    pValue = out_Ast.CreateBinaryOperator(ezExpressionAST::NodeType::Add, pValue, pOffset);

    return pValue;
  }

  ezExpressionAST::Node* CreateRemapTo01WithFadeout(ezExpressionAST::Node* pInput, float fMin, float fMax, float fLowerFade, float fUpperFade, ezExpressionAST& out_Ast)
  {
    auto pLowerOffset = out_Ast.CreateConstant(fMin);
    auto pLowerValue = out_Ast.CreateBinaryOperator(ezExpressionAST::NodeType::Subtract, pInput, pLowerOffset);
    auto pLowerScale = out_Ast.CreateConstant(ezMath::Max(fMax - fMin, 0.0f) * fLowerFade);
    pLowerValue = out_Ast.CreateBinaryOperator(ezExpressionAST::NodeType::Divide, pLowerValue, pLowerScale);

    auto pUpperOffset = out_Ast.CreateConstant(fMax);
    auto pUpperValue = out_Ast.CreateBinaryOperator(ezExpressionAST::NodeType::Subtract, pUpperOffset, pInput);
    auto pUpperScale = out_Ast.CreateConstant(ezMath::Max(fMax - fMin, 0.0f) * fUpperFade);
    pUpperValue = out_Ast.CreateBinaryOperator(ezExpressionAST::NodeType::Divide, pUpperValue, pUpperScale);

    auto pValue = out_Ast.CreateBinaryOperator(ezExpressionAST::NodeType::Min, pLowerValue, pUpperValue);
    pValue = out_Ast.CreateBinaryOperator(ezExpressionAST::NodeType::Max, out_Ast.CreateConstant(0.0f), pValue);
    pValue = out_Ast.CreateBinaryOperator(ezExpressionAST::NodeType::Min, out_Ast.CreateConstant(1.0f), pValue);

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

void ezProcGenOutput::Save(ezStreamWriter& stream)
{
  stream << m_sName;
  stream.WriteArray(m_VolumeTagSetIndices).IgnoreResult();
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProcGen_PlacementOutput, 1, ezRTTIDefaultAllocator<ezProcGen_PlacementOutput>)
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

ezExpressionAST::Node* ezProcGen_PlacementOutput::GenerateExpressionASTNode(ezTempHashedString sOutputName, ezArrayPtr<ezExpressionAST::Node*> inputs, ezExpressionAST& out_Ast, GenerateASTContext& context)
{
  EZ_ASSERT_DEBUG(sOutputName == "", "Implementation error");

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

void ezProcGen_PlacementOutput::Save(ezStreamWriter& stream)
{
  SUPER::Save(stream);

  stream.WriteArray(m_ObjectsToPlace).IgnoreResult();

  stream << m_fFootprint;

  stream << m_vMinOffset;
  stream << m_vMaxOffset;

  stream << m_fAlignToNormal;

  stream << m_vMinScale;
  stream << m_vMaxScale;

  stream << m_fCullDistance;

  stream << m_uiCollisionLayer;

  stream << m_sColorGradient;

  // chunk version 3
  stream << m_sSurface;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProcGen_VertexColorOutput, 1, ezRTTIDefaultAllocator<ezProcGen_VertexColorOutput>)
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

ezExpressionAST::Node* ezProcGen_VertexColorOutput::GenerateExpressionASTNode(ezTempHashedString sOutputName, ezArrayPtr<ezExpressionAST::Node*> inputs, ezExpressionAST& out_Ast, GenerateASTContext& context)
{
  EZ_ASSERT_DEBUG(sOutputName == "", "Implementation error");

  out_Ast.m_OutputNodes.Clear();

  ezHashedString sOutputNames[4] = {ezProcGenInternal::ExpressionOutputs::s_sR, ezProcGenInternal::ExpressionOutputs::s_sG, ezProcGenInternal::ExpressionOutputs::s_sB, ezProcGenInternal::ExpressionOutputs::s_sA};

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

void ezProcGen_VertexColorOutput::Save(ezStreamWriter& stream)
{
  SUPER::Save(stream);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProcGen_Random, 1, ezRTTIDefaultAllocator<ezProcGen_Random>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Seed", m_iSeed)->AddAttributes(new ezClampValueAttribute(-1, ezVariant()), new ezDefaultValueAttribute(-1), new ezMinValueTextAttribute("Auto")),
    EZ_MEMBER_PROPERTY("OutputMin", m_fOutputMin),
    EZ_MEMBER_PROPERTY("OutputMax", m_fOutputMax)->AddAttributes(new ezDefaultValueAttribute(1.0f)),

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
    new ezCategoryAttribute("Math"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezExpressionAST::Node* ezProcGen_Random::GenerateExpressionASTNode(ezTempHashedString sOutputName, ezArrayPtr<ezExpressionAST::Node*> inputs, ezExpressionAST& out_Ast, GenerateASTContext& context)
{
  EZ_ASSERT_DEBUG(sOutputName == "Value", "Implementation error");

  ezRandom rnd;
  rnd.Initialize(m_iSeed < 0 ? m_uiAutoSeed : m_iSeed);

  auto pRandom = CreateRandom(rnd.FloatMinMax(0.0f, 100000.0f), out_Ast);
  return CreateRemapFrom01(pRandom, m_fOutputMin, m_fOutputMax, out_Ast);
}

void ezProcGen_Random::OnObjectCreated(const ezAbstractObjectNode& node)
{
  m_uiAutoSeed = ezHashHelper<ezUuid>::Hash(node.GetGuid());
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProcGen_PerlinNoise, 1, ezRTTIDefaultAllocator<ezProcGen_PerlinNoise>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Scale", m_Scale)->AddAttributes(new ezDefaultValueAttribute(ezVec3(10))),
    EZ_MEMBER_PROPERTY("Offset", m_Offset),
    EZ_MEMBER_PROPERTY("NumOctaves", m_uiNumOctaves)->AddAttributes(new ezClampValueAttribute(1, 6), new ezDefaultValueAttribute(3)),
    EZ_MEMBER_PROPERTY("OutputMin", m_fOutputMin),
    EZ_MEMBER_PROPERTY("OutputMax", m_fOutputMax)->AddAttributes(new ezDefaultValueAttribute(1.0f)),

    EZ_MEMBER_PROPERTY("Value", m_OutputValuePin)
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezTitleAttribute("Perlin Noise"),
    new ezCategoryAttribute("Math"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezExpressionAST::Node* ezProcGen_PerlinNoise::GenerateExpressionASTNode(ezTempHashedString sOutputName, ezArrayPtr<ezExpressionAST::Node*> inputs, ezExpressionAST& out_Ast, GenerateASTContext& context)
{
  EZ_ASSERT_DEBUG(sOutputName == "Value", "Implementation error");

  ezExpressionAST::Node* pPosX = out_Ast.CreateInput(ezProcGenInternal::ExpressionInputs::s_sPositionX);
  ezExpressionAST::Node* pPosY = out_Ast.CreateInput(ezProcGenInternal::ExpressionInputs::s_sPositionY);
  ezExpressionAST::Node* pPosZ = out_Ast.CreateInput(ezProcGenInternal::ExpressionInputs::s_sPositionZ);

  pPosX = out_Ast.CreateBinaryOperator(ezExpressionAST::NodeType::Divide, pPosX, out_Ast.CreateConstant(m_Scale.x));
  pPosY = out_Ast.CreateBinaryOperator(ezExpressionAST::NodeType::Divide, pPosY, out_Ast.CreateConstant(m_Scale.y));
  pPosZ = out_Ast.CreateBinaryOperator(ezExpressionAST::NodeType::Divide, pPosZ, out_Ast.CreateConstant(m_Scale.z));

  pPosX = out_Ast.CreateBinaryOperator(ezExpressionAST::NodeType::Add, pPosX, out_Ast.CreateConstant(m_Offset.x));
  pPosY = out_Ast.CreateBinaryOperator(ezExpressionAST::NodeType::Add, pPosY, out_Ast.CreateConstant(m_Offset.y));
  pPosZ = out_Ast.CreateBinaryOperator(ezExpressionAST::NodeType::Add, pPosZ, out_Ast.CreateConstant(m_Offset.z));

  auto pNumOctaves = out_Ast.CreateConstant(static_cast<float>(m_uiNumOctaves));

  auto pNoiseFunc = out_Ast.CreateFunctionCall(s_sPerlinNoise);
  pNoiseFunc->m_Arguments.PushBack(pPosX);
  pNoiseFunc->m_Arguments.PushBack(pPosY);
  pNoiseFunc->m_Arguments.PushBack(pPosZ);
  pNoiseFunc->m_Arguments.PushBack(pNumOctaves);

  return CreateRemapFrom01(pNoiseFunc, m_fOutputMin, m_fOutputMax, out_Ast);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProcGen_Blend, 1, ezRTTIDefaultAllocator<ezProcGen_Blend>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("Mode", ezProcGenBlendMode, m_BlendMode),
    EZ_MEMBER_PROPERTY("InputA", m_fInputValueA)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("InputB", m_fInputValueB)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("ClampOutput", m_bClampOutput),

    EZ_MEMBER_PROPERTY("A", m_InputValueAPin),
    EZ_MEMBER_PROPERTY("B", m_InputValueBPin),
    EZ_MEMBER_PROPERTY("Value", m_OutputValuePin)
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezTitleAttribute("{Mode}({A}, {B})"),
    new ezCategoryAttribute("Math"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezExpressionAST::Node* ezProcGen_Blend::GenerateExpressionASTNode(ezTempHashedString sOutputName, ezArrayPtr<ezExpressionAST::Node*> inputs, ezExpressionAST& out_Ast, GenerateASTContext& context)
{
  EZ_ASSERT_DEBUG(sOutputName == "Value", "Implementation error");

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

  auto pBlend = out_Ast.CreateBinaryOperator(GetBlendOperator(m_BlendMode), pInputA, pInputB);

  if (m_bClampOutput)
  {
    pBlend = out_Ast.CreateBinaryOperator(ezExpressionAST::NodeType::Max, out_Ast.CreateConstant(0.0f), pBlend);
    pBlend = out_Ast.CreateBinaryOperator(ezExpressionAST::NodeType::Min, out_Ast.CreateConstant(1.0f), pBlend);
  }

  return pBlend;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProcGen_Height, 1, ezRTTIDefaultAllocator<ezProcGen_Height>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("MinHeight", m_fMinHeight)->AddAttributes(new ezDefaultValueAttribute(0.0f)),
    EZ_MEMBER_PROPERTY("MaxHeight", m_fMaxHeight)->AddAttributes(new ezDefaultValueAttribute(1000.0f)),
    EZ_MEMBER_PROPERTY("LowerFade", m_fLowerFade)->AddAttributes(new ezDefaultValueAttribute(0.2f), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_MEMBER_PROPERTY("UpperFade", m_fUpperFade)->AddAttributes(new ezDefaultValueAttribute(0.2f), new ezClampValueAttribute(0.0f, 1.0f)),

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

ezExpressionAST::Node* ezProcGen_Height::GenerateExpressionASTNode(ezTempHashedString sOutputName, ezArrayPtr<ezExpressionAST::Node*> inputs, ezExpressionAST& out_Ast, GenerateASTContext& context)
{
  EZ_ASSERT_DEBUG(sOutputName == "Value", "Implementation error");

  auto pHeight = out_Ast.CreateInput(ezProcGenInternal::ExpressionInputs::s_sPositionZ);
  return CreateRemapTo01WithFadeout(pHeight, m_fMinHeight, m_fMaxHeight, m_fLowerFade, m_fUpperFade, out_Ast);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProcGen_Slope, 1, ezRTTIDefaultAllocator<ezProcGen_Slope>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("MinSlope", m_MinSlope)->AddAttributes(new ezDefaultValueAttribute(ezAngle::Degree(0.0f))),
    EZ_MEMBER_PROPERTY("MaxSlope", m_MaxSlope)->AddAttributes(new ezDefaultValueAttribute(ezAngle::Degree(60.0f))),
    EZ_MEMBER_PROPERTY("LowerFade", m_fLowerFade)->AddAttributes(new ezDefaultValueAttribute(0.0f), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_MEMBER_PROPERTY("UpperFade", m_fUpperFade)->AddAttributes(new ezDefaultValueAttribute(0.2f), new ezClampValueAttribute(0.0f, 1.0f)),


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

ezExpressionAST::Node* ezProcGen_Slope::GenerateExpressionASTNode(ezTempHashedString sOutputName, ezArrayPtr<ezExpressionAST::Node*> inputs, ezExpressionAST& out_Ast, GenerateASTContext& context)
{
  EZ_ASSERT_DEBUG(sOutputName == "Value", "Implementation error");

  auto pNormalZ = out_Ast.CreateInput(ezProcGenInternal::ExpressionInputs::s_sNormalZ);
  auto pAngle = out_Ast.CreateUnaryOperator(ezExpressionAST::NodeType::ACos, pNormalZ);
  return CreateRemapTo01WithFadeout(pAngle, m_MinSlope.GetRadian(), m_MaxSlope.GetRadian(), m_fLowerFade, m_fUpperFade, out_Ast);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProcGen_MeshVertexColor, 1, ezRTTIDefaultAllocator<ezProcGen_MeshVertexColor>)
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
    new ezTitleAttribute("Mesh Vertex Color"),
    new ezCategoryAttribute("Input"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezExpressionAST::Node* ezProcGen_MeshVertexColor::GenerateExpressionASTNode(ezTempHashedString sOutputName, ezArrayPtr<ezExpressionAST::Node*> inputs, ezExpressionAST& out_Ast, GenerateASTContext& context)
{
  if (sOutputName == "R")
  {
    return out_Ast.CreateInput(ezProcGenInternal::ExpressionInputs::s_sColorR);
  }
  else if (sOutputName == "G")
  {
    return out_Ast.CreateInput(ezProcGenInternal::ExpressionInputs::s_sColorG);
  }
  else if (sOutputName == "B")
  {
    return out_Ast.CreateInput(ezProcGenInternal::ExpressionInputs::s_sColorB);
  }
  else
  {
    EZ_ASSERT_DEBUG(sOutputName == "A", "Implementation error");
    return out_Ast.CreateInput(ezProcGenInternal::ExpressionInputs::s_sColorA);
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProcGen_ApplyVolumes, 1, ezRTTIDefaultAllocator<ezProcGen_ApplyVolumes>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("InputValue", m_fInputValue),
    EZ_SET_MEMBER_PROPERTY("IncludeTags", m_IncludeTags)->AddAttributes(new ezTagSetWidgetAttribute("Default")),

    EZ_MEMBER_PROPERTY("In", m_InputValuePin),
    EZ_MEMBER_PROPERTY("Value", m_OutputValuePin)
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezTitleAttribute("Volumes: {IncludeTags}"),
    new ezCategoryAttribute("Math"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezExpressionAST::Node* ezProcGen_ApplyVolumes::GenerateExpressionASTNode(ezTempHashedString sOutputName, ezArrayPtr<ezExpressionAST::Node*> inputs, ezExpressionAST& out_Ast, GenerateASTContext& context)
{
  EZ_ASSERT_DEBUG(sOutputName == "Value", "Implementation error");

  ezUInt32 tagSetIndex = context.m_SharedData.AddTagSet(m_IncludeTags);
  EZ_ASSERT_DEV(tagSetIndex <= 255, "Too many tag sets");
  if (!context.m_VolumeTagSetIndices.Contains(tagSetIndex))
  {
    context.m_VolumeTagSetIndices.PushBack(tagSetIndex);
  }

  ezExpressionAST::Node* pPosX = out_Ast.CreateInput(ezProcGenInternal::ExpressionInputs::s_sPositionX);
  ezExpressionAST::Node* pPosY = out_Ast.CreateInput(ezProcGenInternal::ExpressionInputs::s_sPositionY);
  ezExpressionAST::Node* pPosZ = out_Ast.CreateInput(ezProcGenInternal::ExpressionInputs::s_sPositionZ);

  auto pInput = inputs[0];
  if (pInput == nullptr)
  {
    pInput = out_Ast.CreateConstant(m_fInputValue);
  }

  auto pTagSetInex = out_Ast.CreateConstant(static_cast<float>(tagSetIndex));

  auto pFunctionCall = out_Ast.CreateFunctionCall(s_sApplyVolumes);
  pFunctionCall->m_Arguments.PushBack(pPosX);
  pFunctionCall->m_Arguments.PushBack(pPosY);
  pFunctionCall->m_Arguments.PushBack(pPosZ);
  pFunctionCall->m_Arguments.PushBack(pInput);
  pFunctionCall->m_Arguments.PushBack(pTagSetInex);

  return pFunctionCall;
}
