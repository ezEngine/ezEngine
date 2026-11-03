#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Curves/Curve1DResource.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_RandomSize.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleInitializer_RandomSize, 3, ezRTTIDefaultAllocator<ezParticleInitializer_RandomSize>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Size", m_Size)->AddAttributes(new ezDefaultValueAttribute(ezVarianceTypeFloat(1.0f)), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_RESOURCE_MEMBER_PROPERTY("SizeCurve", m_hCurve)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Data_Curve")),
    EZ_MEMBER_PROPERTY("SizeScaleParameter", m_sSizeScaleParameter),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezParticleInitializer_RandomSize::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = 3;
  inout_stream << uiVersion;

  inout_stream << m_hCurve;
  inout_stream << m_Size.m_Value;
  inout_stream << m_Size.m_fVariance;
  inout_stream << m_sSizeScaleParameter;
}

void ezParticleInitializer_RandomSize::Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor)
{
  ezUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  inout_stream >> m_hCurve;
  inout_stream >> m_Size.m_Value;
  inout_stream >> m_Size.m_fVariance;

  if (uiVersion >= 3)
  {
    inout_stream >> m_sSizeScaleParameter;
  }
}

bool ezParticleInitializer_RandomSize::GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}

void ezParticleInitializer_RandomSize::CreateRequiredStreams(const ezParticleModuleContext& ctx) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  ctx.CreateStream(ezParticleStreamNames::s_sSize, ezProcessingStream::DataType::Half, &data.m_pStreamSize, true);
}

void ezParticleInitializer_RandomSize::InitializeElements(const ezParticleModuleContext& ctx, ezUInt64 uiStartIndex, ezUInt64 uiNumElements) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  EZ_PROFILE_SCOPE("PFX: Random Size");

  ezFloat16* pSize = data.m_pStreamSize->GetWritableData<ezFloat16>();

  ezRandom& rng = ctx.GetRNG();

  const float fSizeScale = ezMath::Max(ctx.GetEffect()->GetFloatParameter(m_sSizeScaleParameter, 1.0f), 0.0f);

  if (!m_hCurve.IsValid())
  {
    for (ezUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      pSize[i] = rng.FloatVariance(m_Size.m_Value, m_Size.m_fVariance) * fSizeScale;
    }
  }
  else
  {
    ezResourceLock<ezCurve1DResource> pResource(m_hCurve, ezResourceAcquireMode::BlockTillLoaded);

    if (!pResource->GetDescriptor().m_Curves.IsEmpty())
    {
      const ezCurve1D& curve = pResource->GetDescriptor().m_Curves[0];

      double fMinX, fMaxX;
      curve.QueryExtents(fMinX, fMaxX);

      // make sure the curve has a length of at least 1
      fMinX = ezMath::Min(fMinX, 0.0);
      fMaxX = ezMath::Max(fMaxX, 1.0);

      for (ezUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
      {
        const double f = rng.DoubleMinMax(fMinX, fMaxX);

        double val = curve.Evaluate(f);
        val = curve.NormalizeValue(val);

        pSize[i] = (float)(val * rng.DoubleVariance(m_Size.m_Value, m_Size.m_fVariance)) * fSizeScale;
      }
    }
    else
    {
      for (ezUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
      {
        pSize[i] = fSizeScale;
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////////

class ezParticleInitializer_RandomSize_1_2 : public ezGraphPatch
{
public:
  ezParticleInitializer_RandomSize_1_2()
    : ezGraphPatch("ezParticleInitializerFactory_RandomSize", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    pNode->InlineProperty("Size").IgnoreResult();
  }
};

ezParticleInitializer_RandomSize_1_2 g_ezParticleInitializer_RandomSize_1_2;

EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Initializer_ParticleInitializer_RandomSize);
