#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/Math/Color16f.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Type/Point/ParticleTypePoint.h>
#include <RendererCore/Pipeline/RenderDataManager.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleTypePoint, 1, ezRTTIDefaultAllocator<ezParticleTypePoint>)
{
  //EZ_BEGIN_ATTRIBUTES
  //{
  //  new ezHiddenAttribute()
  //}
  //EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

enum class TypePointVersion
{
  Version_0 = 0,


  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void ezParticleTypePoint::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = (int)TypePointVersion::Version_Current;
  inout_stream << uiVersion;
}

void ezParticleTypePoint::Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor)
{
  ezUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)TypePointVersion::Version_Current, "Invalid version {0}", uiVersion);
}

bool ezParticleTypePoint::GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}

void ezParticleTypePoint::CreateRequiredStreams(const ezParticleModuleContext& ctx) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  ctx.CreateStream(ezParticleStreamNames::s_sPosition, ezProcessingStream::DataType::Float4, &data.m_pStreamPosition, false);
  ctx.CreateStream(ezParticleStreamNames::s_sColor, ezProcessingStream::DataType::Half4, &data.m_pStreamColor, false);
}

void ezParticleTypePoint::ExtractTypeRenderData(const ezParticleModuleContext& ctx, ezMsgExtractRenderData& ref_msg, const ezTransform& instanceTransform) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  EZ_PROFILE_SCOPE("PFX: Point");

  const ezUInt32 numParticles = (ezUInt32)ctx.m_pSystem->GetNumActiveParticles();

  if (numParticles == 0)
    return;

  // don't copy the data multiple times in the same frame, if the effect is instanced
  if (data.m_uiLastExtractedFrame != ezRenderWorld::GetFrameCounter())
  {
    data.m_uiLastExtractedFrame = ezRenderWorld::GetFrameCounter();

    const ezVec4* pPosition = data.m_pStreamPosition->GetData<ezVec4>();
    const ezColorLinear16f* pColor = data.m_pStreamColor->GetData<ezColorLinear16f>();

    // this will automatically be deallocated at the end of the frame
    data.m_BaseParticleData = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezBaseParticleShaderData, numParticles);
    data.m_BillboardParticleData = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezBillboardQuadParticleShaderData, numParticles);

    for (ezUInt32 p = 0; p < numParticles; ++p)
    {
      data.m_BaseParticleData[p].Color = pColor[p].ToLinearFloat();
      data.m_BillboardParticleData[p].Position = pPosition[p].GetAsVec3();
    }
  }

  auto pRenderData = ref_msg.m_pRenderDataManager->CreateRenderDataForThisFrame<ezParticlePointRenderData>(nullptr);

  pRenderData->m_vGlobalPosition = instanceTransform.m_vPosition;
  pRenderData->m_GlobalTransform = ctx.GetEffect()->NeedsToApplyTransform() ? instanceTransform : ezTransform::MakeIdentity();
  pRenderData->m_TotalEffectLifeTime = ctx.GetEffect()->GetTotalEffectLifeTime();
  pRenderData->m_BaseParticleData = data.m_BaseParticleData;
  pRenderData->m_BillboardParticleData = data.m_BillboardParticleData;

  ref_msg.AddRenderData(pRenderData, ezDefaultRenderDataCategories::LitTransparent, ezRenderData::Caching::Never);
}



EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_Point_ParticleTypePoint);
