#include <EditorPluginProcGen/EditorPluginProcGenPCH.h>

#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenNodes.h>
#include <ProcGenPlugin/Tasks/Utils.h>

namespace
{
  ezExpressionAST::NodeType::Enum GetOperator(ezProcGenBinaryOperator::Enum blendMode)
  {
    switch (blendMode)
    {
      case ezProcGenBinaryOperator::Add:
        return ezExpressionAST::NodeType::Add;
      case ezProcGenBinaryOperator::Subtract:
        return ezExpressionAST::NodeType::Subtract;
      case ezProcGenBinaryOperator::Multiply:
        return ezExpressionAST::NodeType::Multiply;
      case ezProcGenBinaryOperator::Divide:
        return ezExpressionAST::NodeType::Divide;
      case ezProcGenBinaryOperator::Max:
        return ezExpressionAST::NodeType::Max;
      case ezProcGenBinaryOperator::Min:
        return ezExpressionAST::NodeType::Min;

        EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
    }

    return ezExpressionAST::NodeType::Invalid;
  }

  ezExpressionAST::Node* CreateRandom(float fSeed, ezExpressionAST& out_Ast)
  {
    auto pPointIndex = out_Ast.CreateInput({ezProcGenInternal::ExpressionInputs::s_sPointIndex, ezProcessingStream::DataType::Float});
    auto pSeedConstant = out_Ast.CreateConstant(fSeed);

    auto pFunctionCall = out_Ast.CreateFunctionCall(ezDefaultExpressionFunctions::s_RandomFunc.m_Desc);
    pFunctionCall->m_Arguments.PushBack(pPointIndex);
    pFunctionCall->m_Arguments.PushBack(pSeedConstant);

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
    // Note that we need to clamp the scale if it is below eps or we would end up with a division by 0.
    // To counter the clamp we move the lower and upper bounds by eps.
    // If no fade out is specified we would get a value of 0 for inputs that are exactly on the bounds otherwise which is not the expected behavior.

    const float eps = ezMath::DefaultEpsilon<float>();
    const float fLowerScale = ezMath::Max((fMax - fMin), 0.0f) * fLowerFade;
    const float fUpperScale = ezMath::Max((fMax - fMin), 0.0f) * fUpperFade;

    if (fLowerScale < eps)
      fMin = fMin - eps;
    if (fUpperScale < eps)
      fMax = fMax + eps;

    auto pLowerOffset = out_Ast.CreateConstant(fMin);
    auto pLowerValue = out_Ast.CreateBinaryOperator(ezExpressionAST::NodeType::Subtract, pInput, pLowerOffset);
    auto pLowerScale = out_Ast.CreateConstant(ezMath::Max(fLowerScale, eps));
    pLowerValue = out_Ast.CreateBinaryOperator(ezExpressionAST::NodeType::Divide, pLowerValue, pLowerScale);

    auto pUpperOffset = out_Ast.CreateConstant(fMax);
    auto pUpperValue = out_Ast.CreateBinaryOperator(ezExpressionAST::NodeType::Subtract, pUpperOffset, pInput);
    auto pUpperScale = out_Ast.CreateConstant(ezMath::Max(fUpperScale, eps));
    pUpperValue = out_Ast.CreateBinaryOperator(ezExpressionAST::NodeType::Divide, pUpperValue, pUpperScale);

    auto pValue = out_Ast.CreateBinaryOperator(ezExpressionAST::NodeType::Min, pLowerValue, pUpperValue);
    return out_Ast.CreateUnaryOperator(ezExpressionAST::NodeType::Saturate, pValue);
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
    EZ_ARRAY_MEMBER_PROPERTY("Objects", m_ObjectsToPlace)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Prefab")),
    EZ_MEMBER_PROPERTY("Footprint", m_fFootprint)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("MinOffset", m_vMinOffset),
    EZ_MEMBER_PROPERTY("MaxOffset", m_vMaxOffset),
    EZ_MEMBER_PROPERTY("YawRotationSnap", m_YawRotationSnap)->AddAttributes(new ezClampValueAttribute(ezAngle::Radian(0.0f), ezVariant())),
    EZ_MEMBER_PROPERTY("AlignToNormal", m_fAlignToNormal)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_MEMBER_PROPERTY("MinScale", m_vMinScale)->AddAttributes(new ezDefaultValueAttribute(ezVec3(1.0f)), new ezClampValueAttribute(ezVec3(0.0f), ezVariant())),
    EZ_MEMBER_PROPERTY("MaxScale", m_vMaxScale)->AddAttributes(new ezDefaultValueAttribute(ezVec3(1.0f)), new ezClampValueAttribute(ezVec3(0.0f), ezVariant())),
    EZ_MEMBER_PROPERTY("ColorGradient", m_sColorGradient)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Data_Gradient")),
    EZ_MEMBER_PROPERTY("CullDistance", m_fCullDistance)->AddAttributes(new ezDefaultValueAttribute(30.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ENUM_MEMBER_PROPERTY("PlacementMode", ezProcPlacementMode, m_PlacementMode),
    EZ_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
    EZ_MEMBER_PROPERTY("Surface", m_sSurface)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Surface")),

    EZ_MEMBER_PROPERTY("Density", m_DensityPin),
    EZ_MEMBER_PROPERTY("Scale", m_ScalePin)->AddAttributes(new ezColorAttribute(ezColorScheme::DarkUI(ezColorScheme::Pink))),
    EZ_MEMBER_PROPERTY("ColorIndex", m_ColorIndexPin)->AddAttributes(new ezColorAttribute(ezColorScheme::DarkUI(ezColorScheme::Violet))),
    EZ_MEMBER_PROPERTY("ObjectIndex", m_ObjectIndexPin)->AddAttributes(new ezColorAttribute(ezColorScheme::DarkUI(ezColorScheme::Cyan)))
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

ezExpressionAST::Node* ezProcGen_PlacementOutput::GenerateExpressionASTNode(ezTempHashedString sOutputName, ezArrayPtr<ezExpressionAST::Node*> inputs, ezExpressionAST& out_Ast, GraphContext& context)
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

    out_Ast.m_OutputNodes.PushBack(out_Ast.CreateOutput({ezProcGenInternal::ExpressionOutputs::s_sDensity, ezProcessingStream::DataType::Float}, pDensity));
  }

  // scale
  {
    auto pScale = inputs[1];
    if (pScale == nullptr)
    {
      pScale = CreateRandom(11.0f, out_Ast);
    }

    out_Ast.m_OutputNodes.PushBack(out_Ast.CreateOutput({ezProcGenInternal::ExpressionOutputs::s_sScale, ezProcessingStream::DataType::Float}, pScale));
  }

  // color index
  {
    auto pColorIndex = inputs[2];
    if (pColorIndex == nullptr)
    {
      pColorIndex = CreateRandom(13.0f, out_Ast);
    }

    out_Ast.m_OutputNodes.PushBack(out_Ast.CreateOutput({ezProcGenInternal::ExpressionOutputs::s_sColorIndex, ezProcessingStream::DataType::Float}, pColorIndex));
  }

  // object index
  {
    auto pObjectIndex = inputs[3];
    if (pObjectIndex == nullptr)
    {
      pObjectIndex = CreateRandom(17.0f, out_Ast);
    }

    out_Ast.m_OutputNodes.PushBack(out_Ast.CreateOutput({ezProcGenInternal::ExpressionOutputs::s_sObjectIndex, ezProcessingStream::DataType::Float}, pObjectIndex));
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

  // chunk version 6
  stream << m_YawRotationSnap;
  stream << m_fAlignToNormal;

  stream << m_vMinScale;
  stream << m_vMaxScale;

  stream << m_fCullDistance;

  stream << m_uiCollisionLayer;

  stream << m_sColorGradient;

  // chunk version 3
  stream << m_sSurface;

  // chunk version 5
  stream << m_PlacementMode;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProcGen_VertexColorOutput, 1, ezRTTIDefaultAllocator<ezProcGen_VertexColorOutput>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("R", m_RPin)->AddAttributes(new ezColorAttribute(ezColorScheme::DarkUI(ezColorScheme::Red))),
    EZ_MEMBER_PROPERTY("G", m_GPin)->AddAttributes(new ezColorAttribute(ezColorScheme::DarkUI(ezColorScheme::Green))),
    EZ_MEMBER_PROPERTY("B", m_BPin)->AddAttributes(new ezColorAttribute(ezColorScheme::DarkUI(ezColorScheme::Blue))),
    EZ_MEMBER_PROPERTY("A", m_APin),
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

ezExpressionAST::Node* ezProcGen_VertexColorOutput::GenerateExpressionASTNode(ezTempHashedString sOutputName, ezArrayPtr<ezExpressionAST::Node*> inputs, ezExpressionAST& out_Ast, GraphContext& context)
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

    out_Ast.m_OutputNodes.PushBack(out_Ast.CreateOutput({sOutputNames[i], ezProcessingStream::DataType::Float}, pInput));
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

ezExpressionAST::Node* ezProcGen_Random::GenerateExpressionASTNode(ezTempHashedString sOutputName, ezArrayPtr<ezExpressionAST::Node*> inputs, ezExpressionAST& out_Ast, GraphContext& context)
{
  EZ_ASSERT_DEBUG(sOutputName == "Value", "Implementation error");

  float fSeed = m_iSeed < 0 ? m_uiAutoSeed : m_iSeed;

  auto pRandom = CreateRandom(fSeed, out_Ast);
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

ezExpressionAST::Node* ezProcGen_PerlinNoise::GenerateExpressionASTNode(ezTempHashedString sOutputName, ezArrayPtr<ezExpressionAST::Node*> inputs, ezExpressionAST& out_Ast, GraphContext& context)
{
  EZ_ASSERT_DEBUG(sOutputName == "Value", "Implementation error");

  ezExpressionAST::Node* pPosX = out_Ast.CreateInput({ezProcGenInternal::ExpressionInputs::s_sPositionX, ezProcessingStream::DataType::Float});
  ezExpressionAST::Node* pPosY = out_Ast.CreateInput({ezProcGenInternal::ExpressionInputs::s_sPositionY, ezProcessingStream::DataType::Float});
  ezExpressionAST::Node* pPosZ = out_Ast.CreateInput({ezProcGenInternal::ExpressionInputs::s_sPositionZ, ezProcessingStream::DataType::Float});

  pPosX = out_Ast.CreateBinaryOperator(ezExpressionAST::NodeType::Divide, pPosX, out_Ast.CreateConstant(m_Scale.x));
  pPosY = out_Ast.CreateBinaryOperator(ezExpressionAST::NodeType::Divide, pPosY, out_Ast.CreateConstant(m_Scale.y));
  pPosZ = out_Ast.CreateBinaryOperator(ezExpressionAST::NodeType::Divide, pPosZ, out_Ast.CreateConstant(m_Scale.z));

  pPosX = out_Ast.CreateBinaryOperator(ezExpressionAST::NodeType::Add, pPosX, out_Ast.CreateConstant(m_Offset.x));
  pPosY = out_Ast.CreateBinaryOperator(ezExpressionAST::NodeType::Add, pPosY, out_Ast.CreateConstant(m_Offset.y));
  pPosZ = out_Ast.CreateBinaryOperator(ezExpressionAST::NodeType::Add, pPosZ, out_Ast.CreateConstant(m_Offset.z));

  auto pNumOctaves = out_Ast.CreateConstant(m_uiNumOctaves, ezExpressionAST::DataType::Int);

  auto pNoiseFunc = out_Ast.CreateFunctionCall(ezDefaultExpressionFunctions::s_PerlinNoiseFunc.m_Desc);
  pNoiseFunc->m_Arguments.PushBack(pPosX);
  pNoiseFunc->m_Arguments.PushBack(pPosY);
  pNoiseFunc->m_Arguments.PushBack(pPosZ);
  pNoiseFunc->m_Arguments.PushBack(pNumOctaves);

  return CreateRemapFrom01(pNoiseFunc, m_fOutputMin, m_fOutputMax, out_Ast);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProcGen_Blend, 2, ezRTTIDefaultAllocator<ezProcGen_Blend>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("Operator", ezProcGenBinaryOperator, m_Operator),
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
    new ezTitleAttribute("{Operator}({A}, {B})"),
    new ezCategoryAttribute("Math"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezExpressionAST::Node* ezProcGen_Blend::GenerateExpressionASTNode(ezTempHashedString sOutputName, ezArrayPtr<ezExpressionAST::Node*> inputs, ezExpressionAST& out_Ast, GraphContext& context)
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

  ezExpressionAST::Node* pBlend = out_Ast.CreateBinaryOperator(GetOperator(m_Operator), pInputA, pInputB);

  if (m_bClampOutput)
  {
    pBlend = out_Ast.CreateUnaryOperator(ezExpressionAST::NodeType::Saturate, pBlend);
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

ezExpressionAST::Node* ezProcGen_Height::GenerateExpressionASTNode(ezTempHashedString sOutputName, ezArrayPtr<ezExpressionAST::Node*> inputs, ezExpressionAST& out_Ast, GraphContext& context)
{
  EZ_ASSERT_DEBUG(sOutputName == "Value", "Implementation error");

  auto pHeight = out_Ast.CreateInput({ezProcGenInternal::ExpressionInputs::s_sPositionZ, ezProcessingStream::DataType::Float});
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

ezExpressionAST::Node* ezProcGen_Slope::GenerateExpressionASTNode(ezTempHashedString sOutputName, ezArrayPtr<ezExpressionAST::Node*> inputs, ezExpressionAST& out_Ast, GraphContext& context)
{
  EZ_ASSERT_DEBUG(sOutputName == "Value", "Implementation error");

  auto pNormalZ = out_Ast.CreateInput({ezProcGenInternal::ExpressionInputs::s_sNormalZ, ezProcessingStream::DataType::Float});
  // acos explodes for values slightly larger than 1 so make sure to clamp before
  auto pClampedNormalZ = out_Ast.CreateBinaryOperator(ezExpressionAST::NodeType::Min, out_Ast.CreateConstant(1.0f), pNormalZ);
  auto pAngle = out_Ast.CreateUnaryOperator(ezExpressionAST::NodeType::ACos, pClampedNormalZ);
  return CreateRemapTo01WithFadeout(pAngle, m_MinSlope.GetRadian(), m_MaxSlope.GetRadian(), m_fLowerFade, m_fUpperFade, out_Ast);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProcGen_MeshVertexColor, 1, ezRTTIDefaultAllocator<ezProcGen_MeshVertexColor>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("R", m_RPin)->AddAttributes(new ezColorAttribute(ezColorScheme::DarkUI(ezColorScheme::Red))),
    EZ_MEMBER_PROPERTY("G", m_GPin)->AddAttributes(new ezColorAttribute(ezColorScheme::DarkUI(ezColorScheme::Green))),
    EZ_MEMBER_PROPERTY("B", m_BPin)->AddAttributes(new ezColorAttribute(ezColorScheme::DarkUI(ezColorScheme::Blue))),
    EZ_MEMBER_PROPERTY("A", m_APin),
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

ezExpressionAST::Node* ezProcGen_MeshVertexColor::GenerateExpressionASTNode(ezTempHashedString sOutputName, ezArrayPtr<ezExpressionAST::Node*> inputs, ezExpressionAST& out_Ast, GraphContext& context)
{
  if (sOutputName == "R")
  {
    return out_Ast.CreateInput({ezProcGenInternal::ExpressionInputs::s_sColorR, ezProcessingStream::DataType::Float});
  }
  else if (sOutputName == "G")
  {
    return out_Ast.CreateInput({ezProcGenInternal::ExpressionInputs::s_sColorG, ezProcessingStream::DataType::Float});
  }
  else if (sOutputName == "B")
  {
    return out_Ast.CreateInput({ezProcGenInternal::ExpressionInputs::s_sColorB, ezProcessingStream::DataType::Float});
  }
  else
  {
    EZ_ASSERT_DEBUG(sOutputName == "A", "Implementation error");
    return out_Ast.CreateInput({ezProcGenInternal::ExpressionInputs::s_sColorA, ezProcessingStream::DataType::Float});
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProcGen_ApplyVolumes, 1, ezRTTIDefaultAllocator<ezProcGen_ApplyVolumes>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_SET_MEMBER_PROPERTY("IncludeTags", m_IncludeTags)->AddAttributes(new ezTagSetWidgetAttribute("Default")),

    EZ_MEMBER_PROPERTY("InputValue", m_fInputValue),

    EZ_ENUM_MEMBER_PROPERTY("ImageVolumeMode", ezProcVolumeImageMode, m_ImageVolumeMode),
    EZ_MEMBER_PROPERTY("RefColor", m_RefColor)->AddAttributes(new ezExposeColorAlphaAttribute()),

    EZ_MEMBER_PROPERTY("In", m_InputValuePin),
    EZ_MEMBER_PROPERTY("Value", m_OutputValuePin)
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezTitleAttribute("Volumes: {IncludeTags}"),
    new ezCategoryAttribute("Modifiers"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezExpressionAST::Node* ezProcGen_ApplyVolumes::GenerateExpressionASTNode(ezTempHashedString sOutputName, ezArrayPtr<ezExpressionAST::Node*> inputs, ezExpressionAST& out_Ast, GraphContext& context)
{
  EZ_ASSERT_DEBUG(sOutputName == "Value", "Implementation error");

  ezUInt32 tagSetIndex = context.m_SharedData.AddTagSet(m_IncludeTags);
  EZ_ASSERT_DEV(tagSetIndex <= 255, "Too many tag sets");
  if (!context.m_VolumeTagSetIndices.Contains(tagSetIndex))
  {
    context.m_VolumeTagSetIndices.PushBack(tagSetIndex);
  }

  ezExpressionAST::Node* pPosX = out_Ast.CreateInput({ezProcGenInternal::ExpressionInputs::s_sPositionX, ezProcessingStream::DataType::Float});
  ezExpressionAST::Node* pPosY = out_Ast.CreateInput({ezProcGenInternal::ExpressionInputs::s_sPositionY, ezProcessingStream::DataType::Float});
  ezExpressionAST::Node* pPosZ = out_Ast.CreateInput({ezProcGenInternal::ExpressionInputs::s_sPositionZ, ezProcessingStream::DataType::Float});

  auto pInput = inputs[0];
  if (pInput == nullptr)
  {
    pInput = out_Ast.CreateConstant(m_fInputValue);
  }

  auto pFunctionCall = out_Ast.CreateFunctionCall(ezProcGenExpressionFunctions::s_ApplyVolumesFunc.m_Desc);
  pFunctionCall->m_Arguments.PushBack(pPosX);
  pFunctionCall->m_Arguments.PushBack(pPosY);
  pFunctionCall->m_Arguments.PushBack(pPosZ);
  pFunctionCall->m_Arguments.PushBack(pInput);
  pFunctionCall->m_Arguments.PushBack(out_Ast.CreateConstant(tagSetIndex, ezExpressionAST::DataType::Int));
  pFunctionCall->m_Arguments.PushBack(out_Ast.CreateConstant(m_ImageVolumeMode.GetValue(), ezExpressionAST::DataType::Int));
  pFunctionCall->m_Arguments.PushBack(out_Ast.CreateConstant(ezMath::ColorByteToFloat(m_RefColor.r)));
  pFunctionCall->m_Arguments.PushBack(out_Ast.CreateConstant(ezMath::ColorByteToFloat(m_RefColor.g)));
  pFunctionCall->m_Arguments.PushBack(out_Ast.CreateConstant(ezMath::ColorByteToFloat(m_RefColor.b)));
  pFunctionCall->m_Arguments.PushBack(out_Ast.CreateConstant(ezMath::ColorByteToFloat(m_RefColor.a)));

  return pFunctionCall;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class ezProcGen_Blend_1_2 : public ezGraphPatch
{
public:
  ezProcGen_Blend_1_2()
    : ezGraphPatch("ezProcGen_Blend", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    auto* pMode = pNode->FindProperty("Mode");
    if (pMode && pMode->m_Value.IsA<ezString>())
    {
      ezStringBuilder val = pMode->m_Value.Get<ezString>();
      val.ReplaceAll("ezProcGenBlendMode", "ezProcGenBinaryOperator");

      pNode->AddProperty("Operator", val.GetData());
    }
  }
};

ezProcGen_Blend_1_2 g_ezProcGen_Blend_1_2;
