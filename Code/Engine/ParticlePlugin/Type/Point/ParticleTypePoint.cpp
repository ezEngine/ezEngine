#include <PCH.h>

#include <ParticlePlugin/Type/Point/ParticleTypePoint.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <Core/World/GameObject.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <Core/World/World.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <Foundation/Math/Color16f.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleTypePointFactory, 1, ezRTTIDefaultAllocator<ezParticleTypePointFactory>)
{
  //EZ_BEGIN_ATTRIBUTES
  //{
  //  new ezHiddenAttribute()
  //}
  //EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleTypePoint, 1, ezRTTIDefaultAllocator<ezParticleTypePoint>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const ezRTTI* ezParticleTypePointFactory::GetTypeType() const
{
  return ezGetStaticRTTI<ezParticleTypePoint>();
}

void ezParticleTypePointFactory::CopyTypeProperties(ezParticleType* pObject, bool bFirstTime) const
{
  //ezParticleTypePoint* pType = static_cast<ezParticleTypePoint*>(pObject);

}

enum class TypePointVersion
{
  Version_0 = 0,


  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void ezParticleTypePointFactory::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = (int)TypePointVersion::Version_Current;
  stream << uiVersion;

}

void ezParticleTypePointFactory::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)TypePointVersion::Version_Current, "Invalid version {0}", uiVersion);

}

void ezParticleTypePoint::CreateRequiredStreams()
{
  CreateStream("Position", ezProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("Color", ezProcessingStream::DataType::Half4, &m_pStreamColor, false);
}

void ezParticleTypePoint::ExtractTypeRenderData(const ezView& view, ezExtractedRenderData& extractedRenderData, const ezTransform& instanceTransform, ezUInt64 uiExtractedFrame) const
{
  EZ_PROFILE_SCOPE("PFX: Point");

  const ezUInt32 numParticles = (ezUInt32)GetOwnerSystem()->GetNumActiveParticles();

  if (numParticles == 0)
    return;

  // don't copy the data multiple times in the same frame, if the effect is instanced
  if (m_uiLastExtractedFrame != uiExtractedFrame)
  {
    m_uiLastExtractedFrame = uiExtractedFrame;

    const ezVec4* pPosition = m_pStreamPosition->GetData<ezVec4>();
    const ezColorLinear16f* pColor = m_pStreamColor->GetData<ezColorLinear16f>();

    // this will automatically be deallocated at the end of the frame
    m_BaseParticleData = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezBaseParticleShaderData, numParticles);
    m_BillboardParticleData = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezBillboardQuadParticleShaderData, numParticles);

    for (ezUInt32 p = 0; p < numParticles; ++p)
    {
      m_BaseParticleData[p].Color = pColor[p].ToLinearFloat();
      m_BillboardParticleData[p].Position = pPosition[p].GetAsVec3();
    }
  }

  /// \todo Is this batch ID correct?
  const ezUInt32 uiBatchId = 0;
  auto pRenderData = ezCreateRenderDataForThisFrame<ezParticlePointRenderData>(nullptr, uiBatchId);

  pRenderData->m_bApplyObjectTransform = GetOwnerEffect()->NeedsToApplyTransform();
  pRenderData->m_GlobalTransform = instanceTransform;
  pRenderData->m_BaseParticleData = m_BaseParticleData;
  pRenderData->m_BillboardParticleData = m_BillboardParticleData;

  const ezUInt32 uiSortingKey = 0;
  extractedRenderData.AddRenderData(pRenderData, ezDefaultRenderDataCategories::LitTransparent, uiSortingKey);
}




EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_Point_ParticleTypePoint);

