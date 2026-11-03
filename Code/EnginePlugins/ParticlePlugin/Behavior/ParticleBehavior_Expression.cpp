#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Curves/Curve1DResource.h>
#include <Foundation/CodeUtils/Expression/ExpressionCompiler.h>
#include <Foundation/CodeUtils/Expression/ExpressionParser.h>
#include <Foundation/Math/ColorScheme.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/SimdMath/SimdVec4f.h>
#include <Foundation/SimdMath/SimdVec4i.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Expression.h>
#include <ParticlePlugin/Effect/ParticleEffectDescriptor.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_ApplyVelocity.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezParticleExpressionInput, ezNoBase, 1, ezRTTIDefaultAllocator<ezParticleExpressionInput>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("CurveSource", ezCurveSource, m_CurveSource),
    EZ_MEMBER_PROPERTY("Curve", m_Curve),
    EZ_RESOURCE_MEMBER_PROPERTY("SharedCurve", m_hSharedCurve)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Data_Curve")),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehavior_Expression, 1, ezRTTIDefaultAllocator<ezParticleBehavior_Expression>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Expression", m_sExpression)->AddAttributes(new ezExpressionWidgetAttribute(
      "inPos;inVel;inColor;inSize;inRotSpeed;inLifeTime;"
      "outPos;outVel;outColor;outSize;outRotSpeed;outLifeTime;outDiscard;"
      "timeDiff;sampleCurve",
      ezColorGammaUB(ezColorScheme::DarkUI(ezColorScheme::Teal)))),
    EZ_ARRAY_MEMBER_PROPERTY("Inputs", m_Inputs),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

static ezHashedString s_sTimeDiff = ezMakeHashedString("timeDiff");

static ezHashedString s_sInPos = ezMakeHashedString("inPos");
static ezHashedString s_sInVel = ezMakeHashedString("inVel");
static ezHashedString s_sInColor = ezMakeHashedString("inColor");
static ezHashedString s_sInSize = ezMakeHashedString("inSize");
static ezHashedString s_sInRotSpeed = ezMakeHashedString("inRotSpeed");
static ezHashedString s_sInLifeTime = ezMakeHashedString("inLifeTime");
static ezHashedString s_sOutPos = ezMakeHashedString("outPos");
static ezHashedString s_sOutVel = ezMakeHashedString("outVel");
static ezHashedString s_sOutColor = ezMakeHashedString("outColor");
static ezHashedString s_sOutSize = ezMakeHashedString("outSize");
static ezHashedString s_sOutRotSpeed = ezMakeHashedString("outRotSpeed");
static ezHashedString s_sOutLifeTime = ezMakeHashedString("outLifeTime");
static ezHashedString s_sOutDiscard = ezMakeHashedString("outDiscard");

//////////////////////////////////////////////////////////////////////////

ezParticleBehavior_Expression::ezParticleBehavior_Expression()
{
  // execute last
  m_fPriority = 1000.0f;
}

ezParticleBehavior_Expression::InstanceData::InstanceData()
{
  m_VM.RegisterFunction(ezExtendedExpressionFunctions::s_SampleCurveFunc);
}

void ezParticleBehavior_Expression::QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_finalizerDeps) const
{
  if (m_OutputStreams.IsSet(ezParticleStreamMask::Velocity))
  {
    inout_finalizerDeps.Insert(ezGetStaticRTTI<ezParticleFinalizer_ApplyVelocity>());
  }
}

void ezParticleBehavior_Expression::Save(ezStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(1);

  inout_stream << m_sExpression;

  const ezUInt32 uiCount = m_Inputs.GetCount();
  inout_stream << uiCount;
  for (const auto& input : m_Inputs)
  {
    inout_stream << input.m_CurveSource;
    inout_stream << input.m_hSharedCurve;
    input.m_Curve.ConvertToRuntimeData(input.m_RuntimeCurve);
    input.m_RuntimeCurve.SortControlPoints();
    input.m_RuntimeCurve.Save(inout_stream);
  }
}

void ezParticleBehavior_Expression::Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor)
{
  const auto version = inout_stream.ReadVersion(1);

  inout_stream >> m_sExpression;

  ezUInt32 uiCount = 0;
  inout_stream >> uiCount;
  m_Inputs.SetCount(uiCount);
  for (auto& input : m_Inputs)
  {
    inout_stream >> input.m_CurveSource;
    inout_stream >> input.m_hSharedCurve;
    input.m_RuntimeCurve.Load(inout_stream);
    input.m_RuntimeCurve.SortControlPoints();
    input.m_RuntimeCurve.CreateLinearApproximation();
  }

  CompileExpression(ownerEffectDescriptor);
  BuildCurveSamples();
}

void ezParticleBehavior_Expression::CompileExpression(const ezParticleEffectDescriptor& ownerEffectDescriptor)
{
  if (m_sCompiledExpression == m_sExpression)
    return;

  m_bBytecodeValid = false;
  m_sCompiledExpression = m_sExpression;
  m_FloatParamNames.Clear();
  m_ColorParamNames.Clear();

  if (m_sExpression.IsEmpty())
    return;

  ezTempHybridArray<ezExpression::StreamDesc, 12> inputs;
  ezTempHybridArray<ezExpression::StreamDesc, 8> outputs;

  m_InputStreams.Clear();
  m_OutputStreams.Clear();
  m_bUsesDiscard = false;

  ezStringBuilder sExpressionNoComments = m_sExpression;
  sExpressionNoComments.RemoveCStyleComments();

  {
    if (sExpressionNoComments.FindSubString("inPos"))
    {
      m_InputStreams.Add(ezParticleStreamMask::Position);
      inputs.PushBack(ezExpression::StreamDesc(s_sInPos, ezProcessingStream::DataType::Float3));
    }
    if (sExpressionNoComments.FindSubString("inVel"))
    {
      m_InputStreams.Add(ezParticleStreamMask::Velocity);
      inputs.PushBack(ezExpression::StreamDesc(s_sInVel, ezProcessingStream::DataType::Half4));
    }
    if (sExpressionNoComments.FindSubString("inColor"))
    {
      m_InputStreams.Add(ezParticleStreamMask::Color);
      inputs.PushBack(ezExpression::StreamDesc(s_sInColor, ezProcessingStream::DataType::Half4));
    }
    if (sExpressionNoComments.FindSubString("inSize"))
    {
      m_InputStreams.Add(ezParticleStreamMask::Size);
      inputs.PushBack(ezExpression::StreamDesc(s_sInSize, ezProcessingStream::DataType::Half));
    }
    if (sExpressionNoComments.FindSubString("inRotSpeed"))
    {
      m_InputStreams.Add(ezParticleStreamMask::Rotation);
      inputs.PushBack(ezExpression::StreamDesc(s_sInRotSpeed, ezProcessingStream::DataType::Half));
    }
    if (sExpressionNoComments.FindSubString("inLifeTime"))
    {
      m_InputStreams.Add(ezParticleStreamMask::LifeTime);
      inputs.PushBack(ezExpression::StreamDesc(s_sInLifeTime, ezProcessingStream::DataType::Half2));
    }

    for (auto it = ownerEffectDescriptor.m_FloatParameters.GetIterator(); it.IsValid(); ++it)
    {
      if (sExpressionNoComments.FindSubString(it.Key()))
      {
        ezHashedString sName;
        sName.Assign(it.Key());
        m_FloatParamNames.PushBack(sName);
        inputs.PushBack(ezExpression::StreamDesc(sName, ezProcessingStream::DataType::Float));
      }
    }

    for (auto it = ownerEffectDescriptor.m_ColorParameters.GetIterator(); it.IsValid(); ++it)
    {
      if (sExpressionNoComments.FindSubString(it.Key()))
      {
        ezHashedString sName;
        sName.Assign(it.Key());
        m_ColorParamNames.PushBack(sName);
        inputs.PushBack(ezExpression::StreamDesc(sName, ezProcessingStream::DataType::Float4));
      }
    }

    // Global variables are always registered so the parser accepts them unconditionally.
    // The VM only maps what the bytecode actually references, so providing unused globals has no runtime cost.
    inputs.PushBack(ezExpression::StreamDesc(s_sTimeDiff, ezProcessingStream::DataType::Float));
  }

  {
    if (sExpressionNoComments.FindSubString("outPos"))
    {
      m_OutputStreams.Add(ezParticleStreamMask::Position);
      outputs.PushBack(ezExpression::StreamDesc(s_sOutPos, ezProcessingStream::DataType::Float3));
    }
    if (sExpressionNoComments.FindSubString("outVel"))
    {
      m_OutputStreams.Add(ezParticleStreamMask::Velocity);
      outputs.PushBack(ezExpression::StreamDesc(s_sOutVel, ezProcessingStream::DataType::Half4));
    }
    if (sExpressionNoComments.FindSubString("outColor"))
    {
      m_OutputStreams.Add(ezParticleStreamMask::Color);
      outputs.PushBack(ezExpression::StreamDesc(s_sOutColor, ezProcessingStream::DataType::Half4));
    }
    if (sExpressionNoComments.FindSubString("outSize"))
    {
      m_OutputStreams.Add(ezParticleStreamMask::Size);
      outputs.PushBack(ezExpression::StreamDesc(s_sOutSize, ezProcessingStream::DataType::Half));
    }
    if (sExpressionNoComments.FindSubString("outRotSpeed"))
    {
      m_OutputStreams.Add(ezParticleStreamMask::Rotation);
      outputs.PushBack(ezExpression::StreamDesc(s_sOutRotSpeed, ezProcessingStream::DataType::Half));
    }
    if (sExpressionNoComments.FindSubString("outDiscard"))
    {
      m_bUsesDiscard = true;
      outputs.PushBack(ezExpression::StreamDesc(s_sOutDiscard, ezProcessingStream::DataType::Byte));
    }
  }

  ezExpressionParser parser;
  parser.RegisterFunction(ezExtendedExpressionFunctions::s_SampleCurveFunc.m_Desc);

  ezExpressionParser::Options options;
  ezExpressionAST ast;
  if (parser.Parse(m_sExpression, inputs, outputs, options, ast).Failed())
  {
    ezLog::Warning("Particle Expression: Failed to parse expression '{0}'", m_sExpression);
    return;
  }

  ezExpressionCompiler compiler;
  if (compiler.Compile(ast, m_ByteCode).Failed())
  {
    ezLog::Warning("Particle Expression: Failed to compile expression '{0}'", m_sExpression);
    return;
  }

  m_bBytecodeValid = true;
}

void ezParticleBehavior_Expression::BuildCurveSamples()
{
  m_CurveSamples.SetCount(m_Inputs.GetCount());

  for (ezUInt32 i = 0; i < m_Inputs.GetCount(); ++i)
  {
    const auto& input = m_Inputs[i];
    auto& samples = m_CurveSamples[i];

    ezCurve1D runtimeCurve;

    if (input.m_CurveSource == ezCurveSource::SharedCurve)
    {
      if (input.m_hSharedCurve.IsValid())
      {
        ezResourceLock<ezCurve1DResource> pResource(input.m_hSharedCurve, ezResourceAcquireMode::BlockTillLoaded);
        if (pResource.GetAcquireResult() == ezResourceAcquireResult::Final && !pResource->GetDescriptor().m_Curves.IsEmpty())
        {
          runtimeCurve = pResource->GetDescriptor().m_Curves[0];
        }
      }
    }
    else
    {
      // Prefer converting from edit-time data (editor path).
      // Fall back to the loaded runtime curve when edit data is unavailable (game runtime path).
      input.m_Curve.ConvertToRuntimeData(runtimeCurve);
      if (runtimeCurve.GetNumControlPoints() == 0)
      {
        runtimeCurve = input.m_RuntimeCurve;
      }
      else
      {
        runtimeCurve.SortControlPoints();
      }
    }

    if (runtimeCurve.GetNumControlPoints() == 0)
    {
      samples.m_Samples.SetCount(1);
      samples.m_Samples[0] = 0.0f;
      samples.m_fMinX = 0.0f;
      samples.m_fMaxX = 1.0f;
      continue;
    }

    constexpr ezUInt32 k_uiSampleCount = 256;
    runtimeCurve.GenerateSampledCurve(k_uiSampleCount, samples).AssertSuccess();
  }
}

//////////////////////////////////////////////////////////////////////////

bool ezParticleBehavior_Expression::GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}

void ezParticleBehavior_Expression::CreateRequiredStreams(const ezParticleModuleContext& ctx) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  data.m_pStreamPosition = nullptr;
  data.m_pStreamVelocity = nullptr;
  data.m_pStreamSize = nullptr;
  data.m_pStreamColor = nullptr;
  data.m_pStreamLifeTime = nullptr;
  data.m_pStreamRotationSpeed = nullptr;

  if (m_InputStreams.IsSet(ezParticleStreamMask::Position) || m_OutputStreams.IsSet(ezParticleStreamMask::Position))
  {
    ctx.CreateStream(ezParticleStreamNames::s_sPosition, ezProcessingStream::DataType::Float4, &data.m_pStreamPosition, false);
  }

  if (m_InputStreams.IsSet(ezParticleStreamMask::Velocity) || m_OutputStreams.IsSet(ezParticleStreamMask::Velocity))
  {
    ctx.CreateStream(ezParticleStreamNames::s_sVelocity, ezProcessingStream::DataType::Half4, &data.m_pStreamVelocity, false);
  }

  if (m_InputStreams.IsSet(ezParticleStreamMask::Size) || m_OutputStreams.IsSet(ezParticleStreamMask::Size))
  {
    ctx.CreateStream(ezParticleStreamNames::s_sSize, ezProcessingStream::DataType::Half, &data.m_pStreamSize, false);
  }

  if (m_InputStreams.IsSet(ezParticleStreamMask::Color) || m_OutputStreams.IsSet(ezParticleStreamMask::Color))
  {
    ctx.CreateStream(ezParticleStreamNames::s_sColor, ezProcessingStream::DataType::Half4, &data.m_pStreamColor, false);
  }

  if (m_InputStreams.IsSet(ezParticleStreamMask::Rotation) || m_OutputStreams.IsSet(ezParticleStreamMask::Rotation))
  {
    ctx.CreateStream(ezParticleStreamNames::s_sRotationSpeed, ezProcessingStream::DataType::Half, &data.m_pStreamRotationSpeed, false);
  }

  if (m_InputStreams.IsSet(ezParticleStreamMask::LifeTime) || m_OutputStreams.IsSet(ezParticleStreamMask::LifeTime) || m_bUsesDiscard)
  {
    ctx.CreateStream(ezParticleStreamNames::s_sLifeTime, ezProcessingStream::DataType::Half2, &data.m_pStreamLifeTime, false);
  }
}

void ezParticleBehavior_Expression::OnFinalize(const ezParticleModuleContext& ctx) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  data.m_bExecutionFailed = false;
}

void ezParticleBehavior_Expression::InitializeElements(const ezParticleModuleContext& ctx, ezUInt64 uiStartIndex, ezUInt64 uiNumElements) const
{
  UpdateElements(ctx, uiStartIndex, uiNumElements);
}

void ezParticleBehavior_Expression::Process(const ezParticleModuleContext& ctx, ezUInt64 uiNumElements) const
{
  UpdateElements(ctx, 0, uiNumElements);
}

void ezParticleBehavior_Expression::UpdateElements(const ezParticleModuleContext& ctx, ezUInt64 uiStartIndex, ezUInt64 uiNumElements) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  if (GetByteCode() == nullptr || data.m_bExecutionFailed || uiNumElements == 0)
    return;

  EZ_PROFILE_SCOPE("PFX: Expression");

  static thread_local ezDynamicArray<ezUInt8> s_DiscardBuffer;

  // When called from InitializeElements, uiStartIndex > 0. The VM always processes from element 0,
  // so we must advance all data pointers by uiStartIndex elements to address the correct particle slots.
  auto OffsetStream = [uiStartIndex](const ezHashedString& sName, const ezProcessingStream& src)
  {
    const ezUInt64 uiByteOffset = uiStartIndex * src.GetElementStride();
    ezUInt8* pData = static_cast<ezUInt8*>(src.GetWritableData()) + uiByteOffset;
    const ezUInt32 uiRemainingBytes = static_cast<ezUInt32>(src.GetDataSize() - uiByteOffset);
    return ezProcessingStream(sName, ezArrayPtr<ezUInt8>(pData, uiRemainingBytes), src.GetDataType(), src.GetElementStride());
  };

  // Fetch current effect parameter values. These buffers must outlive the VM execute call.
  ezHybridArray<float, 4> floatParamValues;
  {
    floatParamValues.SetCount(m_FloatParamNames.GetCount());
    for (ezUInt32 i = 0; i < m_FloatParamNames.GetCount(); ++i)
    {
      floatParamValues[i] = ctx.GetEffect()->GetFloatParameter(m_FloatParamNames[i], 0.0f);
    }
  }

  ezHybridArray<ezColor, 2> colorParamValues;
  {
    colorParamValues.SetCount(m_ColorParamNames.GetCount());
    for (ezUInt32 i = 0; i < m_ColorParamNames.GetCount(); ++i)
    {
      colorParamValues[i] = ctx.GetEffect()->GetColorParameter(m_ColorParamNames[i], ezColor::White);
    }
  }

  ezTempHybridArray<ezProcessingStream, 8> vmInputs;
  const float fTimeDiff = static_cast<float>(ctx.GetTimeDiff().GetSeconds());

  {
    if (m_InputStreams.IsSet(ezParticleStreamMask::Position))
    {
      vmInputs.PushBack(OffsetStream(s_sInPos, *data.m_pStreamPosition));
    }

    if (m_InputStreams.IsSet(ezParticleStreamMask::Velocity))
    {
      vmInputs.PushBack(OffsetStream(s_sInVel, *data.m_pStreamVelocity));
    }
    if (m_InputStreams.IsSet(ezParticleStreamMask::Size))
    {
      vmInputs.PushBack(OffsetStream(s_sInSize, *data.m_pStreamSize));
    }
    if (m_InputStreams.IsSet(ezParticleStreamMask::Color))
    {
      vmInputs.PushBack(OffsetStream(s_sInColor, *data.m_pStreamColor));
    }
    if (m_InputStreams.IsSet(ezParticleStreamMask::LifeTime))
    {
      vmInputs.PushBack(OffsetStream(s_sInLifeTime, *data.m_pStreamLifeTime));
    }
    if (m_InputStreams.IsSet(ezParticleStreamMask::Rotation))
    {
      vmInputs.PushBack(OffsetStream(s_sInRotSpeed, *data.m_pStreamRotationSpeed));
    }

    vmInputs.PushBack(ezProcessingStream(s_sTimeDiff,
      ezArrayPtr<ezUInt8>(const_cast<ezUInt8*>(reinterpret_cast<const ezUInt8*>(&fTimeDiff)), sizeof(float)),
      ezProcessingStream::DataType::Float, /*uiStride*/ 0));

    for (ezUInt32 i = 0; i < floatParamValues.GetCount(); ++i)
    {
      vmInputs.PushBack(ezProcessingStream(m_FloatParamNames[i],
        ezArrayPtr<ezUInt8>(reinterpret_cast<ezUInt8*>(&floatParamValues[i]), sizeof(float)),
        ezProcessingStream::DataType::Float, /*uiStride*/ 0));
    }

    for (ezUInt32 i = 0; i < colorParamValues.GetCount(); ++i)
    {
      vmInputs.PushBack(ezProcessingStream(m_ColorParamNames[i],
        ezArrayPtr<ezUInt8>(reinterpret_cast<ezUInt8*>(&colorParamValues[i]), sizeof(ezColor)),
        ezProcessingStream::DataType::Float4, /*uiStride*/ 0));
    }
  }

  ezTempHybridArray<ezProcessingStream, 8> vmOutputs;

  {
    if (m_OutputStreams.IsSet(ezParticleStreamMask::Position))
    {
      vmOutputs.PushBack(OffsetStream(s_sOutPos, *data.m_pStreamPosition));
    }

    if (m_OutputStreams.IsSet(ezParticleStreamMask::Velocity))
    {
      vmOutputs.PushBack(OffsetStream(s_sOutVel, *data.m_pStreamVelocity));
    }
    if (m_OutputStreams.IsSet(ezParticleStreamMask::Size))
    {
      vmOutputs.PushBack(OffsetStream(s_sOutSize, *data.m_pStreamSize));
    }
    if (m_OutputStreams.IsSet(ezParticleStreamMask::Color))
    {
      vmOutputs.PushBack(OffsetStream(s_sOutColor, *data.m_pStreamColor));
    }
    if (m_OutputStreams.IsSet(ezParticleStreamMask::LifeTime))
    {
      vmOutputs.PushBack(OffsetStream(s_sOutLifeTime, *data.m_pStreamLifeTime));
    }
    if (m_OutputStreams.IsSet(ezParticleStreamMask::Rotation))
    {
      vmOutputs.PushBack(OffsetStream(s_sOutRotSpeed, *data.m_pStreamRotationSpeed));
    }
    if (m_bUsesDiscard)
    {
      s_DiscardBuffer.SetCountUninitialized(static_cast<ezUInt32>(uiNumElements));
      vmOutputs.PushBack(ezProcessingStream(s_sOutDiscard, s_DiscardBuffer.GetArrayPtr(), ezProcessingStream::DataType::Byte));
    }
  }

  const ezUInt32 uiCount = static_cast<ezUInt32>(uiNumElements);

  // Build global data for curve lookups (pointers into the sample arrays of the module).
  ezExpression::GlobalData globalData;
  if (!m_CurveSamples.IsEmpty())
  {
    ezVariantArray curves;
    curves.SetCount(m_CurveSamples.GetCount());
    for (ezUInt32 i = 0; i < m_CurveSamples.GetCount(); ++i)
    {
      curves[i] = ezVariant(const_cast<ezSampledCurve1D*>(&m_CurveSamples[i]));
    }
    globalData.Insert(ezMakeHashedString("Curves"), curves);
  }

  if (data.m_VM.Execute(*GetByteCode(), vmInputs, vmOutputs, uiCount, globalData).Failed())
  {
    ezLog::Warning("Particle Expression: Failed to execute expression");
    data.m_bExecutionFailed = true;
    return;
  }

  if (m_bUsesDiscard && data.m_pStreamLifeTime != nullptr)
  {
    const ezUInt8* pDiscardData = s_DiscardBuffer.GetData();
    ezFloat16Vec2* pLifeTime = data.m_pStreamLifeTime->GetWritableData<ezFloat16Vec2>() + uiStartIndex;

    for (ezUInt32 i = 0; i < uiCount; ++i)
    {
      if (pDiscardData[i] > 0)
      {
        pLifeTime[i].x = 0.0f;
      }
    }
  }
}

EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_Expression);
