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

  ezExpressionAST::Node* CreateRandom(ezUInt32 uiSeed, ezExpressionAST& out_ast, const ezProcGenNodeBase::GraphContext& context)
  {
    EZ_ASSERT_DEV(context.m_OutputType != ezProcGenNodeBase::GraphContext::Unknown, "Unkown output type");

    auto pointIndexDataType = context.m_OutputType == ezProcGenNodeBase::GraphContext::Placement ? ezProcessingStream::DataType::Short : ezProcessingStream::DataType::Int;
    ezExpressionAST::Node* pPointIndex = out_ast.CreateInput({ezProcGenInternal::ExpressionInputs::s_sPointIndex, pointIndexDataType});

    ezExpressionAST::Node* pSeed = out_ast.CreateFunctionCall(ezProcGenExpressionFunctions::s_GetInstanceSeedFunc.m_Desc, ezArrayPtr<ezExpressionAST::Node*>());
    pSeed = out_ast.CreateBinaryOperator(ezExpressionAST::NodeType::Add, pSeed, out_ast.CreateConstant(uiSeed, ezExpressionAST::DataType::Int));

    ezExpressionAST::Node* arguments[] = {pPointIndex, pSeed};
    return out_ast.CreateFunctionCall(ezDefaultExpressionFunctions::s_RandomFunc.m_Desc, arguments);
  }

  ezExpressionAST::Node* CreateRemapFrom01(ezExpressionAST::Node* pInput, float fMin, float fMax, ezExpressionAST& out_ast)
  {
    auto pOffset = out_ast.CreateConstant(fMin);
    auto pScale = out_ast.CreateConstant(fMax - fMin);

    auto pValue = out_ast.CreateBinaryOperator(ezExpressionAST::NodeType::Multiply, pInput, pScale);
    pValue = out_ast.CreateBinaryOperator(ezExpressionAST::NodeType::Add, pValue, pOffset);

    return pValue;
  }

  ezExpressionAST::Node* CreateRemapTo01WithFadeout(ezExpressionAST::Node* pInput, float fMin, float fMax, float fLowerFade, float fUpperFade, ezExpressionAST& out_ast)
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

    auto pLowerOffset = out_ast.CreateConstant(fMin);
    auto pLowerValue = out_ast.CreateBinaryOperator(ezExpressionAST::NodeType::Subtract, pInput, pLowerOffset);
    auto pLowerScale = out_ast.CreateConstant(ezMath::Max(fLowerScale, eps));
    pLowerValue = out_ast.CreateBinaryOperator(ezExpressionAST::NodeType::Divide, pLowerValue, pLowerScale);

    auto pUpperOffset = out_ast.CreateConstant(fMax);
    auto pUpperValue = out_ast.CreateBinaryOperator(ezExpressionAST::NodeType::Subtract, pUpperOffset, pInput);
    auto pUpperScale = out_ast.CreateConstant(ezMath::Max(fUpperScale, eps));
    pUpperValue = out_ast.CreateBinaryOperator(ezExpressionAST::NodeType::Divide, pUpperValue, pUpperScale);

    auto pValue = out_ast.CreateBinaryOperator(ezExpressionAST::NodeType::Min, pLowerValue, pUpperValue);
    return out_ast.CreateUnaryOperator(ezExpressionAST::NodeType::Saturate, pValue);
  }

  void AddDefaultInputs(ezExpressionAST& out_ast)
  {
    out_ast.m_InputNodes.Clear();

    out_ast.m_InputNodes.PushBack(out_ast.CreateInput({ezProcGenInternal::ExpressionInputs::s_sPositionX, ezProcessingStream::DataType::Float}));
    out_ast.m_InputNodes.PushBack(out_ast.CreateInput({ezProcGenInternal::ExpressionInputs::s_sPositionY, ezProcessingStream::DataType::Float}));
    out_ast.m_InputNodes.PushBack(out_ast.CreateInput({ezProcGenInternal::ExpressionInputs::s_sPositionZ, ezProcessingStream::DataType::Float}));

    out_ast.m_InputNodes.PushBack(out_ast.CreateInput({ezProcGenInternal::ExpressionInputs::s_sNormalX, ezProcessingStream::DataType::Float}));
    out_ast.m_InputNodes.PushBack(out_ast.CreateInput({ezProcGenInternal::ExpressionInputs::s_sNormalY, ezProcessingStream::DataType::Float}));
    out_ast.m_InputNodes.PushBack(out_ast.CreateInput({ezProcGenInternal::ExpressionInputs::s_sNormalZ, ezProcessingStream::DataType::Float}));
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
    EZ_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new ezDynamicStringEnumAttribute("ProcGenOutputNameEnum")),
  }
  EZ_END_PROPERTIES;

  flags.Add(ezTypeFlags::Abstract);
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezProcGenOutput::Save(ezStreamWriter& inout_stream)
{
  inout_stream << m_sName;
  inout_stream.WriteArray(m_VolumeTagSetIndices).IgnoreResult();
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
    EZ_MEMBER_PROPERTY("YawRotationSnap", m_YawRotationSnap)->AddAttributes(new ezClampValueAttribute(ezAngle::MakeFromRadian(0.0f), ezVariant())),
    EZ_MEMBER_PROPERTY("AlignToNormal", m_fAlignToNormal)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_MEMBER_PROPERTY("MinScale", m_vMinScale)->AddAttributes(new ezDefaultValueAttribute(ezVec3(1.0f)), new ezClampValueAttribute(ezVec3(0.0f), ezVariant())),
    EZ_MEMBER_PROPERTY("MaxScale", m_vMaxScale)->AddAttributes(new ezDefaultValueAttribute(ezVec3(1.0f)), new ezClampValueAttribute(ezVec3(0.0f), ezVariant())),
    EZ_MEMBER_PROPERTY("ColorGradient", m_sColorGradient)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Data_Gradient")),
    EZ_MEMBER_PROPERTY("CullDistance", m_fCullDistance)->AddAttributes(new ezDefaultValueAttribute(30.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ENUM_MEMBER_PROPERTY("PlacementMode", ezProcPlacementMode, m_PlacementMode),
    EZ_ENUM_MEMBER_PROPERTY("PlacementPattern", ezProcPlacementPattern, m_PlacementPattern),
    EZ_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
    EZ_MEMBER_PROPERTY("Surface", m_sSurface)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Surface", ezDependencyFlags::Package)),

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

ezExpressionAST::Node* ezProcGen_PlacementOutput::GenerateExpressionASTNode(ezTempHashedString sOutputName, ezArrayPtr<ezExpressionAST::Node*> inputs, ezExpressionAST& out_ast, GraphContext& ref_context)
{
  EZ_ASSERT_DEBUG(sOutputName == "", "Implementation error");

  AddDefaultInputs(out_ast);
  out_ast.m_InputNodes.PushBack(out_ast.CreateInput({ezProcGenInternal::ExpressionInputs::s_sPointIndex, ezProcessingStream::DataType::Short}));

  out_ast.m_OutputNodes.Clear();

  // density
  {
    auto pDensity = inputs[0];
    if (pDensity == nullptr)
    {
      pDensity = out_ast.CreateConstant(1.0f);
    }

    out_ast.m_OutputNodes.PushBack(out_ast.CreateOutput({ezProcGenInternal::ExpressionOutputs::s_sOutDensity, ezProcessingStream::DataType::Float}, pDensity));
  }

  // scale
  {
    auto pScale = inputs[1];
    if (pScale == nullptr)
    {
      pScale = CreateRandom(11.0f, out_ast, ref_context);
    }

    out_ast.m_OutputNodes.PushBack(out_ast.CreateOutput({ezProcGenInternal::ExpressionOutputs::s_sOutScale, ezProcessingStream::DataType::Float}, pScale));
  }

  // color index
  {
    auto pColorIndex = inputs[2];
    if (pColorIndex == nullptr)
    {
      pColorIndex = CreateRandom(13.0f, out_ast, ref_context);
    }

    pColorIndex = out_ast.CreateUnaryOperator(ezExpressionAST::NodeType::Saturate, pColorIndex);
    pColorIndex = out_ast.CreateBinaryOperator(ezExpressionAST::NodeType::Multiply, pColorIndex, out_ast.CreateConstant(255.0f));
    pColorIndex = out_ast.CreateBinaryOperator(ezExpressionAST::NodeType::Add, pColorIndex, out_ast.CreateConstant(0.5f));

    out_ast.m_OutputNodes.PushBack(out_ast.CreateOutput({ezProcGenInternal::ExpressionOutputs::s_sOutColorIndex, ezProcessingStream::DataType::Byte}, pColorIndex));
  }

  // object index
  {
    auto pObjectIndex = inputs[3];
    if (pObjectIndex == nullptr)
    {
      pObjectIndex = CreateRandom(17.0f, out_ast, ref_context);
    }

    pObjectIndex = out_ast.CreateUnaryOperator(ezExpressionAST::NodeType::Saturate, pObjectIndex);
    pObjectIndex = out_ast.CreateBinaryOperator(ezExpressionAST::NodeType::Multiply, pObjectIndex, out_ast.CreateConstant(m_ObjectsToPlace.GetCount() - 1));
    pObjectIndex = out_ast.CreateBinaryOperator(ezExpressionAST::NodeType::Add, pObjectIndex, out_ast.CreateConstant(0.5f));

    out_ast.m_OutputNodes.PushBack(out_ast.CreateOutput({ezProcGenInternal::ExpressionOutputs::s_sOutObjectIndex, ezProcessingStream::DataType::Byte}, pObjectIndex));
  }

  return nullptr;
}

void ezProcGen_PlacementOutput::Save(ezStreamWriter& inout_stream)
{
  SUPER::Save(inout_stream);

  inout_stream.WriteArray(m_ObjectsToPlace).IgnoreResult();

  inout_stream << m_fFootprint;

  inout_stream << m_vMinOffset;
  inout_stream << m_vMaxOffset;

  // chunk version 6
  inout_stream << m_YawRotationSnap;
  inout_stream << m_fAlignToNormal;

  inout_stream << m_vMinScale;
  inout_stream << m_vMaxScale;

  inout_stream << m_fCullDistance;

  inout_stream << m_uiCollisionLayer;

  inout_stream << m_sColorGradient;

  // chunk version 3
  inout_stream << m_sSurface;

  // chunk version 5
  inout_stream << m_PlacementMode;

  // chunk version 7
  inout_stream << m_PlacementPattern;
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

ezExpressionAST::Node* ezProcGen_VertexColorOutput::GenerateExpressionASTNode(ezTempHashedString sOutputName, ezArrayPtr<ezExpressionAST::Node*> inputs, ezExpressionAST& out_ast, GraphContext& ref_context)
{
  EZ_ASSERT_DEBUG(sOutputName == "", "Implementation error");

  AddDefaultInputs(out_ast);

  out_ast.m_InputNodes.PushBack(out_ast.CreateInput({ezProcGenInternal::ExpressionInputs::s_sColorR, ezProcessingStream::DataType::Float}));
  out_ast.m_InputNodes.PushBack(out_ast.CreateInput({ezProcGenInternal::ExpressionInputs::s_sColorG, ezProcessingStream::DataType::Float}));
  out_ast.m_InputNodes.PushBack(out_ast.CreateInput({ezProcGenInternal::ExpressionInputs::s_sColorB, ezProcessingStream::DataType::Float}));
  out_ast.m_InputNodes.PushBack(out_ast.CreateInput({ezProcGenInternal::ExpressionInputs::s_sColorA, ezProcessingStream::DataType::Float}));

  out_ast.m_InputNodes.PushBack(out_ast.CreateInput({ezProcGenInternal::ExpressionInputs::s_sPointIndex, ezProcessingStream::DataType::Int}));

  out_ast.m_OutputNodes.Clear();

  ezHashedString sOutputNames[4] = {
    ezProcGenInternal::ExpressionOutputs::s_sOutColorR,
    ezProcGenInternal::ExpressionOutputs::s_sOutColorG,
    ezProcGenInternal::ExpressionOutputs::s_sOutColorB,
    ezProcGenInternal::ExpressionOutputs::s_sOutColorA,
  };

  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(sOutputNames); ++i)
  {
    auto pInput = inputs[i];
    if (pInput == nullptr)
    {
      pInput = out_ast.CreateConstant(0.0f);
    }

    out_ast.m_OutputNodes.PushBack(out_ast.CreateOutput({sOutputNames[i], ezProcessingStream::DataType::Float}, pInput));
  }

  return nullptr;
}

void ezProcGen_VertexColorOutput::Save(ezStreamWriter& inout_stream)
{
  SUPER::Save(inout_stream);
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

ezExpressionAST::Node* ezProcGen_Random::GenerateExpressionASTNode(ezTempHashedString sOutputName, ezArrayPtr<ezExpressionAST::Node*> inputs, ezExpressionAST& out_ast, GraphContext& ref_context)
{
  EZ_ASSERT_DEBUG(sOutputName == "Value", "Implementation error");

  float fSeed = m_iSeed < 0 ? m_uiAutoSeed : m_iSeed;

  auto pRandom = CreateRandom(fSeed, out_ast, ref_context);
  return CreateRemapFrom01(pRandom, m_fOutputMin, m_fOutputMax, out_ast);
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

ezExpressionAST::Node* ezProcGen_PerlinNoise::GenerateExpressionASTNode(ezTempHashedString sOutputName, ezArrayPtr<ezExpressionAST::Node*> inputs, ezExpressionAST& out_ast, GraphContext& ref_context)
{
  EZ_ASSERT_DEBUG(sOutputName == "Value", "Implementation error");

  ezExpressionAST::Node* pPos = out_ast.CreateInput({ezProcGenInternal::ExpressionInputs::s_sPosition, ezProcessingStream::DataType::Float3});
  pPos = out_ast.CreateBinaryOperator(ezExpressionAST::NodeType::Divide, pPos, out_ast.CreateConstant(m_Scale, ezExpressionAST::DataType::Float3));
  pPos = out_ast.CreateBinaryOperator(ezExpressionAST::NodeType::Add, pPos, out_ast.CreateConstant(m_Offset, ezExpressionAST::DataType::Float3));

  auto pPosX = out_ast.CreateSwizzle(ezExpressionAST::VectorComponent::X, pPos);
  auto pPosY = out_ast.CreateSwizzle(ezExpressionAST::VectorComponent::Y, pPos);
  auto pPosZ = out_ast.CreateSwizzle(ezExpressionAST::VectorComponent::Z, pPos);

  auto pNumOctaves = out_ast.CreateConstant(m_uiNumOctaves, ezExpressionAST::DataType::Int);

  ezExpressionAST::Node* arguments[] = {pPosX, pPosY, pPosZ, pNumOctaves};

  auto pNoiseFunc = out_ast.CreateFunctionCall(ezDefaultExpressionFunctions::s_PerlinNoiseFunc.m_Desc, arguments);

  return CreateRemapFrom01(pNoiseFunc, m_fOutputMin, m_fOutputMax, out_ast);
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

ezExpressionAST::Node* ezProcGen_Blend::GenerateExpressionASTNode(ezTempHashedString sOutputName, ezArrayPtr<ezExpressionAST::Node*> inputs, ezExpressionAST& out_ast, GraphContext& ref_context)
{
  EZ_ASSERT_DEBUG(sOutputName == "Value", "Implementation error");

  auto pInputA = inputs[0];
  if (pInputA == nullptr)
  {
    pInputA = out_ast.CreateConstant(m_fInputValueA);
  }

  auto pInputB = inputs[1];
  if (pInputB == nullptr)
  {
    pInputB = out_ast.CreateConstant(m_fInputValueB);
  }

  ezExpressionAST::Node* pBlend = out_ast.CreateBinaryOperator(GetOperator(m_Operator), pInputA, pInputB);

  if (m_bClampOutput)
  {
    pBlend = out_ast.CreateUnaryOperator(ezExpressionAST::NodeType::Saturate, pBlend);
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

ezExpressionAST::Node* ezProcGen_Height::GenerateExpressionASTNode(ezTempHashedString sOutputName, ezArrayPtr<ezExpressionAST::Node*> inputs, ezExpressionAST& out_ast, GraphContext& ref_context)
{
  EZ_ASSERT_DEBUG(sOutputName == "Value", "Implementation error");

  auto pHeight = out_ast.CreateInput({ezProcGenInternal::ExpressionInputs::s_sPositionZ, ezProcessingStream::DataType::Float});
  return CreateRemapTo01WithFadeout(pHeight, m_fMinHeight, m_fMaxHeight, m_fLowerFade, m_fUpperFade, out_ast);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProcGen_Slope, 1, ezRTTIDefaultAllocator<ezProcGen_Slope>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("MinSlope", m_MinSlope)->AddAttributes(new ezDefaultValueAttribute(ezAngle::MakeFromDegree(0.0f))),
    EZ_MEMBER_PROPERTY("MaxSlope", m_MaxSlope)->AddAttributes(new ezDefaultValueAttribute(ezAngle::MakeFromDegree(60.0f))),
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

ezExpressionAST::Node* ezProcGen_Slope::GenerateExpressionASTNode(ezTempHashedString sOutputName, ezArrayPtr<ezExpressionAST::Node*> inputs, ezExpressionAST& out_ast, GraphContext& ref_context)
{
  EZ_ASSERT_DEBUG(sOutputName == "Value", "Implementation error");

  auto pNormalZ = out_ast.CreateInput({ezProcGenInternal::ExpressionInputs::s_sNormalZ, ezProcessingStream::DataType::Float});
  // acos explodes for values slightly larger than 1 so make sure to clamp before
  auto pClampedNormalZ = out_ast.CreateBinaryOperator(ezExpressionAST::NodeType::Min, out_ast.CreateConstant(1.0f), pNormalZ);
  auto pAngle = out_ast.CreateUnaryOperator(ezExpressionAST::NodeType::ACos, pClampedNormalZ);
  return CreateRemapTo01WithFadeout(pAngle, m_MinSlope.GetRadian(), m_MaxSlope.GetRadian(), m_fLowerFade, m_fUpperFade, out_ast);
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

ezExpressionAST::Node* ezProcGen_MeshVertexColor::GenerateExpressionASTNode(ezTempHashedString sOutputName, ezArrayPtr<ezExpressionAST::Node*> inputs, ezExpressionAST& out_ast, GraphContext& ref_context)
{
  if (sOutputName == "R")
  {
    return out_ast.CreateInput({ezProcGenInternal::ExpressionInputs::s_sColorR, ezProcessingStream::DataType::Float});
  }
  else if (sOutputName == "G")
  {
    return out_ast.CreateInput({ezProcGenInternal::ExpressionInputs::s_sColorG, ezProcessingStream::DataType::Float});
  }
  else if (sOutputName == "B")
  {
    return out_ast.CreateInput({ezProcGenInternal::ExpressionInputs::s_sColorB, ezProcessingStream::DataType::Float});
  }
  else
  {
    EZ_ASSERT_DEBUG(sOutputName == "A", "Implementation error");
    return out_ast.CreateInput({ezProcGenInternal::ExpressionInputs::s_sColorA, ezProcessingStream::DataType::Float});
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

ezExpressionAST::Node* ezProcGen_ApplyVolumes::GenerateExpressionASTNode(ezTempHashedString sOutputName, ezArrayPtr<ezExpressionAST::Node*> inputs, ezExpressionAST& out_ast, GraphContext& ref_context)
{
  EZ_ASSERT_DEBUG(sOutputName == "Value", "Implementation error");

  ezUInt32 tagSetIndex = ref_context.m_SharedData.AddTagSet(m_IncludeTags);
  EZ_ASSERT_DEV(tagSetIndex <= 255, "Too many tag sets");
  if (!ref_context.m_VolumeTagSetIndices.Contains(tagSetIndex))
  {
    ref_context.m_VolumeTagSetIndices.PushBack(tagSetIndex);
  }

  auto pPosX = out_ast.CreateInput({ezProcGenInternal::ExpressionInputs::s_sPositionX, ezProcessingStream::DataType::Float});
  auto pPosY = out_ast.CreateInput({ezProcGenInternal::ExpressionInputs::s_sPositionY, ezProcessingStream::DataType::Float});
  auto pPosZ = out_ast.CreateInput({ezProcGenInternal::ExpressionInputs::s_sPositionZ, ezProcessingStream::DataType::Float});

  auto pInput = inputs[0];
  if (pInput == nullptr)
  {
    pInput = out_ast.CreateConstant(m_fInputValue);
  }

  ezExpressionAST::Node* arguments[] = {
    pPosX,
    pPosY,
    pPosZ,
    pInput,
    out_ast.CreateConstant(tagSetIndex, ezExpressionAST::DataType::Int),
    out_ast.CreateConstant(m_ImageVolumeMode.GetValue(), ezExpressionAST::DataType::Int),
    out_ast.CreateConstant(ezMath::ColorByteToFloat(m_RefColor.r)),
    out_ast.CreateConstant(ezMath::ColorByteToFloat(m_RefColor.g)),
    out_ast.CreateConstant(ezMath::ColorByteToFloat(m_RefColor.b)),
    out_ast.CreateConstant(ezMath::ColorByteToFloat(m_RefColor.a)),
  };

  return out_ast.CreateFunctionCall(ezProcGenExpressionFunctions::s_ApplyVolumesFunc.m_Desc, arguments);
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

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
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
