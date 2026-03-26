#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Curves/Curve1DResource.h>
#include <Foundation/CodeUtils/Expression/ExpressionCompiler.h>
#include <Foundation/CodeUtils/Expression/ExpressionParser.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Expression.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_ApplyVelocity.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehaviorFactory_Expression, 1, ezRTTIDefaultAllocator<ezParticleBehaviorFactory_Expression>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("Mode", ezParticleExpressionMode, m_Mode),
    EZ_MEMBER_PROPERTY("Expression", m_sExpression),
    EZ_ENUM_MEMBER_PROPERTY("InputA", ezParticleAttributeRead, m_InputA),
    EZ_ENUM_MEMBER_PROPERTY("InputB", ezParticleAttributeRead, m_InputB),
    EZ_ENUM_MEMBER_PROPERTY("InputC", ezParticleAttributeRead, m_InputC),
    EZ_ENUM_MEMBER_PROPERTY("InputD", ezParticleAttributeRead, m_InputD),
    EZ_ENUM_MEMBER_PROPERTY("Modify", ezParticleAttributeWrite, m_Output),
    EZ_ENUM_MEMBER_PROPERTY("Comparison", ezComparisonOperator, m_Comparison)->AddAttributes(new ezDefaultValueAttribute((ezInt32)ezComparisonOperator::Less)),
    EZ_MEMBER_PROPERTY("DiscardThreshold", m_fThreshold),
    EZ_MEMBER_PROPERTY("Curve", m_Curve),
    EZ_RESOURCE_MEMBER_PROPERTY("SharedCurve", m_hSharedCurve)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Data_Curve")),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehavior_Expression, 1, ezRTTIDefaultAllocator<ezParticleBehavior_Expression>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

static const ezHashedString s_InputA = ezMakeHashedString("a");
static const ezHashedString s_InputB = ezMakeHashedString("b");
static const ezHashedString s_InputC = ezMakeHashedString("c");
static const ezHashedString s_InputD = ezMakeHashedString("d");
static const ezHashedString s_Output = ezMakeHashedString("out");

ezParticleBehaviorFactory_Expression::ezParticleBehaviorFactory_Expression() = default;

const ezRTTI* ezParticleBehaviorFactory_Expression::GetBehaviorType() const
{
  return ezGetStaticRTTI<ezParticleBehavior_Expression>();
}

void ezParticleBehaviorFactory_Expression::CopyBehaviorProperties(ezParticleBehavior* pObject, bool bFirstTime) const
{
  ezParticleBehavior_Expression* pBehavior = static_cast<ezParticleBehavior_Expression*>(pObject);

  pBehavior->m_Mode = m_Mode;
  pBehavior->m_InputA = m_InputA;
  pBehavior->m_InputB = m_InputB;
  pBehavior->m_InputC = m_InputC;
  pBehavior->m_InputD = m_InputD;
  pBehavior->m_Output = m_Output;
  pBehavior->m_Comparison = m_Comparison;
  pBehavior->m_fThreshold = m_fThreshold;

  const bool bCurveMode = (m_Mode == ezParticleExpressionMode::LookupCurve || m_Mode == ezParticleExpressionMode::LookupSharedCurve);
  pBehavior->m_pCurve = bCurveMode ? &m_RuntimeCurve : nullptr;

  EnsureExpressionCompiled();
  pBehavior->m_pByteCode = GetByteCode();
}

void ezParticleBehaviorFactory_Expression::QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_finalizerDeps) const
{
  const bool bWritesAttribute = (m_Mode == ezParticleExpressionMode::Set ||
                                 m_Mode == ezParticleExpressionMode::LookupCurve ||
                                 m_Mode == ezParticleExpressionMode::LookupSharedCurve);
  if (bWritesAttribute)
  {
    // If the output modifies velocity, we need the ApplyVelocity finalizer
    if (m_Output == ezParticleAttributeWrite::VelocityX ||
        m_Output == ezParticleAttributeWrite::VelocityY ||
        m_Output == ezParticleAttributeWrite::VelocityZ ||
        m_Output == ezParticleAttributeWrite::Speed)
    {
      inout_finalizerDeps.Insert(ezGetStaticRTTI<ezParticleFinalizerFactory_ApplyVelocity>());
    }
  }
}

void ezParticleBehaviorFactory_Expression::Save(ezStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(1);

  inout_stream << m_sExpression;
  inout_stream << m_InputA;
  inout_stream << m_InputB;
  inout_stream << m_InputC;
  inout_stream << m_InputD;
  inout_stream << m_Output;
  inout_stream << m_Mode;
  inout_stream << m_Comparison;
  inout_stream << m_fThreshold;

  inout_stream << m_hSharedCurve;
  m_Curve.ConvertToRuntimeData(m_RuntimeCurve);
  m_RuntimeCurve.SortControlPoints();
  m_RuntimeCurve.Save(inout_stream);
}

void ezParticleBehaviorFactory_Expression::Load(ezStreamReader& inout_stream)
{
  /*const auto version =*/inout_stream.ReadVersion(1);

  inout_stream >> m_sExpression;
  inout_stream >> m_InputA;
  inout_stream >> m_InputB;
  inout_stream >> m_InputC;
  inout_stream >> m_InputD;
  inout_stream >> m_Output;
  inout_stream >> m_Mode;
  inout_stream >> m_Comparison;
  inout_stream >> m_fThreshold;
  inout_stream >> m_hSharedCurve;
  m_RuntimeCurve.Load(inout_stream);
  m_RuntimeCurve.SortControlPoints();
  m_RuntimeCurve.CreateLinearApproximation();

  if (m_Mode == ezParticleExpressionMode::LookupSharedCurve && m_hSharedCurve.IsValid())
  {
    ezResourceLock<ezCurve1DResource> pCurveResource(m_hSharedCurve, ezResourceAcquireMode::BlockTillLoaded);
    if (pCurveResource.GetAcquireResult() == ezResourceAcquireResult::Final && !pCurveResource->GetDescriptor().m_Curves.IsEmpty())
    {
      m_RuntimeCurve = pCurveResource->GetDescriptor().m_Curves[0];
    }
  }

  EnsureExpressionCompiled();
}

void ezParticleBehaviorFactory_Expression::EnsureExpressionCompiled() const
{
  if (m_sCompiledExpression == m_sExpression)
    return;

  m_bBytecodeValid = false;
  m_sCompiledExpression = m_sExpression;

  if (m_sExpression.IsEmpty())
    return;

  ezExpressionParser parser;

  ezExpression::StreamDesc inputs[4];
  inputs[0] = ezExpression::StreamDesc({s_InputA, ezProcessingStream::DataType::Float});
  inputs[1] = ezExpression::StreamDesc({s_InputB, ezProcessingStream::DataType::Float});
  inputs[2] = ezExpression::StreamDesc({s_InputC, ezProcessingStream::DataType::Float});
  inputs[3] = ezExpression::StreamDesc({s_InputD, ezProcessingStream::DataType::Float});

  ezExpression::StreamDesc outputs[1];
  outputs[0] = ezExpression::StreamDesc({s_Output, ezProcessingStream::DataType::Float});

  const ezStringBuilder sFullExpression("out = ", m_sExpression);

  ezExpressionParser::Options options;
  ezExpressionAST ast;
  if (parser.Parse(sFullExpression, inputs, outputs, options, ast).Failed())
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

//////////////////////////////////////////////////////////////////////////

ezParticleBehavior_Expression::ezParticleBehavior_Expression()
{
  // execute last
  m_fPriority = 1000.0f;
}

void ezParticleBehavior_Expression::CreateRequiredStreams()
{
  CreateStream("Position", ezProcessingStream::DataType::Float4, &m_pStreamPosition, false);
}

void ezParticleBehavior_Expression::QueryOptionalStreams()
{
  m_pStreamSize = GetOwnerSystem()->QueryStream("Size", ezProcessingStream::DataType::Half);
  m_pStreamColor = GetOwnerSystem()->QueryStream("Color", ezProcessingStream::DataType::Half4);
  m_pStreamLifeTime = GetOwnerSystem()->QueryStream("LifeTime", ezProcessingStream::DataType::Half2);
  m_pStreamVelocity = GetOwnerSystem()->QueryStream("Velocity", ezProcessingStream::DataType::Half4);
  m_pStreamRotationSpeed = GetOwnerSystem()->QueryStream("RotationSpeed", ezProcessingStream::DataType::Half);
}

void ezParticleBehavior_Expression::InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
{
  if (m_Mode != ezParticleExpressionMode::Discard)
  {
    UpdateElements(uiStartIndex, uiNumElements);
  }
}

void ezParticleBehavior_Expression::Process(ezUInt64 uiNumElements)
{
  UpdateElements(0, uiNumElements);
}

static constexpr ezUInt32 s_uiBatchSize = 128;

void ezParticleBehavior_Expression::UpdateElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
{
  if (m_pByteCode == nullptr || uiNumElements == 0)
    return;

  EZ_PROFILE_SCOPE("PFX: Expression");

  float inputA[s_uiBatchSize], inputB[s_uiBatchSize], inputC[s_uiBatchSize], inputD[s_uiBatchSize];
  float result[s_uiBatchSize];

  ezProcessingStream vmInputs[4];
  vmInputs[0] = ezProcessingStream(s_InputA, ezMakeArrayPtr(inputA).ToByteArray(), ezProcessingStream::DataType::Float);
  vmInputs[1] = ezProcessingStream(s_InputB, ezMakeArrayPtr(inputB).ToByteArray(), ezProcessingStream::DataType::Float);
  vmInputs[2] = ezProcessingStream(s_InputC, ezMakeArrayPtr(inputC).ToByteArray(), ezProcessingStream::DataType::Float);
  vmInputs[3] = ezProcessingStream(s_InputD, ezMakeArrayPtr(inputD).ToByteArray(), ezProcessingStream::DataType::Float);

  ezProcessingStream vmOutputs[1];
  vmOutputs[0] = ezProcessingStream(s_Output, ezMakeArrayPtr(result).ToByteArray(), ezProcessingStream::DataType::Float);

  const ezUInt32 uiCount = static_cast<ezUInt32>(uiNumElements);
  ezUInt32 uiProcessed = 0;

  while (uiProcessed < uiCount)
  {
    const ezUInt32 uiBatchCount = ezMath::Min(uiCount - uiProcessed, s_uiBatchSize);

    for (ezUInt32 i = 0; i < uiBatchCount; ++i)
    {
      const ezUInt32 idx = static_cast<ezUInt32>(uiStartIndex) + uiProcessed + i;
      inputA[i] = ezReadParticleAttribute(m_InputA, idx, m_pStreamPosition, m_pStreamVelocity, m_pStreamSize, m_pStreamColor, m_pStreamLifeTime, m_pStreamRotationSpeed);
      inputB[i] = ezReadParticleAttribute(m_InputB, idx, m_pStreamPosition, m_pStreamVelocity, m_pStreamSize, m_pStreamColor, m_pStreamLifeTime, m_pStreamRotationSpeed);
      inputC[i] = ezReadParticleAttribute(m_InputC, idx, m_pStreamPosition, m_pStreamVelocity, m_pStreamSize, m_pStreamColor, m_pStreamLifeTime, m_pStreamRotationSpeed);
      inputD[i] = ezReadParticleAttribute(m_InputD, idx, m_pStreamPosition, m_pStreamVelocity, m_pStreamSize, m_pStreamColor, m_pStreamLifeTime, m_pStreamRotationSpeed);
    }

    if (m_VM.Execute(*m_pByteCode, vmInputs, vmOutputs, uiBatchCount).Failed())
    {
      ezLog::Warning("Particle Expression: Failed to execute expression");
      m_pByteCode = nullptr;
      return;
    }

    if (m_Mode == ezParticleExpressionMode::Discard)
    {
      for (ezUInt32 i = 0; i < uiBatchCount; ++i)
      {
        const ezUInt32 idx = static_cast<ezUInt32>(uiStartIndex) + uiProcessed + i;
        if (ezComparisonOperator::Compare(m_Comparison, result[i], m_fThreshold))
        {
          m_pStreamGroup->RemoveElement(idx);
        }
      }
    }
    else
    {
      for (ezUInt32 i = 0; i < uiBatchCount; ++i)
      {
        const ezUInt32 idx = static_cast<ezUInt32>(uiStartIndex) + uiProcessed + i;

        float fOutputValue = result[i];
        if (m_pCurve != nullptr && !m_pCurve->IsEmpty())
        {
          fOutputValue = static_cast<float>(m_pCurve->Evaluate(result[i]));
        }

        ezWriteParticleAttribute(m_Output, idx, fOutputValue, m_pStreamPosition, m_pStreamVelocity, m_pStreamSize, m_pStreamColor, m_pStreamRotationSpeed);
      }
    }

    uiProcessed += uiBatchCount;
  }
}

EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_Expression);
