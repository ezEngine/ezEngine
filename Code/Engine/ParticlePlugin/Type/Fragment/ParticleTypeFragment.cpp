#include <PCH.h>
#include <ParticlePlugin/Type/Fragment/ParticleTypeFragment.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <Core/World/GameObject.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <Core/World/World.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezFragmentAxis, 1)
EZ_ENUM_CONSTANTS(ezFragmentAxis::OrthogonalEmitterDirection, ezFragmentAxis::EmitterDirection)
EZ_END_STATIC_REFLECTED_ENUM()

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleTypeFragmentFactory, 1, ezRTTIDefaultAllocator<ezParticleTypeFragmentFactory>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Texture", m_sTexture)->AddAttributes(new ezAssetBrowserAttribute("Texture 2D")),
    EZ_MEMBER_PROPERTY("NumSpritesX", m_uiNumSpritesX)->AddAttributes(new ezDefaultValueAttribute(1), new ezClampValueAttribute(1, 16)),
    EZ_MEMBER_PROPERTY("NumSpritesY", m_uiNumSpritesY)->AddAttributes(new ezDefaultValueAttribute(1), new ezClampValueAttribute(1, 16)),
    EZ_ENUM_MEMBER_PROPERTY("RotationAxis", ezFragmentAxis, m_RotationAxis),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleTypeFragment, 1, ezRTTIDefaultAllocator<ezParticleTypeFragment>)
EZ_END_DYNAMIC_REFLECTED_TYPE

const ezRTTI* ezParticleTypeFragmentFactory::GetTypeType() const
{
  return ezGetStaticRTTI<ezParticleTypeFragment>();
}


void ezParticleTypeFragmentFactory::CopyTypeProperties(ezParticleType* pObject) const
{
  ezParticleTypeFragment* pType = static_cast<ezParticleTypeFragment*>(pObject);

  pType->m_RotationAxis = m_RotationAxis;
  pType->m_hTexture.Invalidate();
  pType->m_uiNumSpritesX = m_uiNumSpritesX;
  pType->m_uiNumSpritesY = m_uiNumSpritesY;

  if (!m_sTexture.IsEmpty())
    pType->m_hTexture = ezResourceManager::LoadResource<ezTexture2DResource>(m_sTexture);
}

enum class TypeFragmentVersion
{
  Version_0 = 0,
  Version_1,
  Version_2, // added texture
  Version_3, // added fragment rotation mode
  Version_4, // added flipbook animation

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void ezParticleTypeFragmentFactory::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = (int)TypeFragmentVersion::Version_Current;
  stream << uiVersion;

  stream << m_sTexture;
  stream << m_RotationAxis.GetValue();
  stream << m_uiNumSpritesX;
  stream << m_uiNumSpritesY;
}

void ezParticleTypeFragmentFactory::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)TypeFragmentVersion::Version_Current, "Invalid version {0}", uiVersion);

  if (uiVersion >= 2)
  {
    stream >> m_sTexture;
  }

  if (uiVersion >= 3)
  {
    ezFragmentAxis::StorageType val;
    stream >> val;
    m_RotationAxis.SetValue(val);
  }

  if (uiVersion >= 4)
  {
    stream >> m_uiNumSpritesX;
    stream >> m_uiNumSpritesY;
  }
}


void ezParticleTypeFragment::CreateRequiredStreams()
{
  CreateStream("LifeTime", ezProcessingStream::DataType::Float2, &m_pStreamLifeTime, false);
  CreateStream("Position", ezProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("Size", ezProcessingStream::DataType::Float, &m_pStreamSize, false);
  CreateStream("Color", ezProcessingStream::DataType::Float4, &m_pStreamColor, false);
  CreateStream("RotationSpeed", ezProcessingStream::DataType::Float, &m_pStreamRotationSpeed, false);
}

void ezParticleTypeFragment::ExtractTypeRenderData(const ezView& view, ezExtractedRenderData& extractedRenderData, const ezTransform& instanceTransform, ezUInt64 uiExtractedFrame) const
{
  EZ_PROFILE("PFX: Fragment");

  if (!m_hTexture.IsValid())
    return;

  if (GetOwnerSystem()->GetNumActiveParticles() == 0)
    return;

  const ezTime tCur = GetOwnerSystem()->GetWorld()->GetClock().GetAccumulatedTime();

  const ezVec3 vEmitterPos = GetOwnerSystem()->GetTransform().m_vPosition;
  const ezVec3 vEmitterDir = GetOwnerSystem()->GetTransform().m_qRotation * ezVec3(0, 0, 1); // Z axis

  // don't copy the data multiple times in the same frame, if the effect is instanced
  if (m_uiLastExtractedFrame != uiExtractedFrame)
  {
    m_uiLastExtractedFrame = uiExtractedFrame;

    const ezVec4* pPosition = m_pStreamPosition->GetData<ezVec4>();
    const float* pSize = m_pStreamSize->GetData<float>();
    const ezColor* pColor = m_pStreamColor->GetData<ezColor>();
    const float* pRotationSpeed = m_pStreamRotationSpeed->GetData<float>();
    const ezVec2* pLifeTime = m_pStreamLifeTime->GetData<ezVec2>();

    // this will automatically be deallocated at the end of the frame
    m_ParticleData = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezFragmentParticleData, (ezUInt32)GetOwnerSystem()->GetNumActiveParticles());

    ezTransform t;

    for (ezUInt32 p = 0; p < (ezUInt32)GetOwnerSystem()->GetNumActiveParticles(); ++p)
    {
      m_ParticleData[p].Size = pSize[p];
      m_ParticleData[p].Color = pColor[p];
      m_ParticleData[p].Life = pLifeTime[p].x * pLifeTime[p].y;
    }

    if (m_RotationAxis == ezFragmentAxis::EmitterDirection)
    {
      const ezVec3 vTangentZ = vEmitterDir;
      const ezVec3 vTangentX = vEmitterDir.GetOrthogonalVector();

      for (ezUInt32 p = 0; p < (ezUInt32)GetOwnerSystem()->GetNumActiveParticles(); ++p)
      {
        ezMat3 mRotation;
        mRotation.SetRotationMatrix(vEmitterDir, ezAngle::Radian((float)(tCur.GetSeconds() * pRotationSpeed[p])));

        m_ParticleData[p].Position = pPosition[p].GetAsVec3();
        m_ParticleData[p].TangentX = mRotation * vTangentX;
        m_ParticleData[p].TangentZ = vTangentZ;
      }
    }
    else if (m_RotationAxis == ezFragmentAxis::OrthogonalEmitterDirection)
    {
      const ezVec3 vTangentZ = vEmitterDir;

      for (ezUInt32 p = 0; p < (ezUInt32)GetOwnerSystem()->GetNumActiveParticles(); ++p)
      {
        const ezVec3 vDirToParticle = (pPosition[p].GetAsVec3() - vEmitterPos);
        ezVec3 vOrthoDir = vEmitterDir.Cross(vDirToParticle);
        vOrthoDir.NormalizeIfNotZero(ezVec3(1, 0, 0));

        ezMat3 mRotation;
        mRotation.SetRotationMatrix(vOrthoDir, ezAngle::Radian((float)(tCur.GetSeconds() * pRotationSpeed[p])));

        m_ParticleData[p].Position = pPosition[p].GetAsVec3();
        m_ParticleData[p].TangentX = vOrthoDir;
        m_ParticleData[p].TangentZ = mRotation * vTangentZ;
      }
    }
  }

  /// \todo Is this batch ID correct?
  const ezUInt32 uiBatchId = m_hTexture.GetResourceIDHash();
  auto pRenderData = ezCreateRenderDataForThisFrame<ezParticleFragmentRenderData>(nullptr, uiBatchId);

  pRenderData->m_bApplyObjectTransform = GetOwnerEffect()->NeedsToApplyTransform();
  pRenderData->m_GlobalTransform = instanceTransform;
  pRenderData->m_hTexture = m_hTexture;
  pRenderData->m_ParticleData = m_ParticleData;
  pRenderData->m_uiNumSpritesX = m_uiNumSpritesX;
  pRenderData->m_uiNumSpritesY = m_uiNumSpritesY;

  const ezUInt32 uiSortingKey = ComputeSortingKey(ezParticleTypeRenderMode::Opaque);
  extractedRenderData.AddRenderData(pRenderData, ezDefaultRenderDataCategories::LitTransparent, uiSortingKey);
}




EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_Fragment_ParticleTypeFragment);

