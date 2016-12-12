#include <ParticlePlugin/PCH.h>
#include <Foundation/Math/Angle.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Math/Vec3.h>

#include <ParticlePlugin/Type/Sprite/ParticleTypeSprite.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Textures/TextureResource.h>
#include <Core/World/GameObject.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <Core/World/World.h>

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezSpriteAxis, 1)
EZ_ENUM_CONSTANTS(ezSpriteAxis::EmitterDirection, ezSpriteAxis::WorldUp, ezSpriteAxis::Random)
EZ_END_STATIC_REFLECTED_ENUM()

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleTypeSpriteFactory, 1, ezRTTIDefaultAllocator<ezParticleTypeSpriteFactory>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Texture", m_sTexture)->AddAttributes(new ezAssetBrowserAttribute("Texture 2D")),
    EZ_ENUM_MEMBER_PROPERTY("RotationAxis", ezSpriteAxis, m_RotationAxis),
    EZ_MEMBER_PROPERTY("Deviation", m_MaxDeviation)->AddAttributes(new ezClampValueAttribute(ezAngle::Degree(0), ezAngle::Degree(90))),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleTypeSprite, 1, ezRTTIDefaultAllocator<ezParticleTypeSprite>)
EZ_END_DYNAMIC_REFLECTED_TYPE

const ezRTTI* ezParticleTypeSpriteFactory::GetTypeType() const
{
  return ezGetStaticRTTI<ezParticleTypeSprite>();
}


void ezParticleTypeSpriteFactory::CopyTypeProperties(ezParticleType* pObject) const
{
  ezParticleTypeSprite* pType = static_cast<ezParticleTypeSprite*>(pObject);

  pType->m_RotationAxis = m_RotationAxis;
  pType->m_MaxDeviation = m_MaxDeviation;
  pType->m_hTexture.Invalidate();

  if (!m_sTexture.IsEmpty())
    pType->m_hTexture = ezResourceManager::LoadResource<ezTextureResource>(m_sTexture);
}

enum class TypeSpriteVersion
{
  Version_0 = 0,
  Version_1,

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void ezParticleTypeSpriteFactory::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = (int)TypeSpriteVersion::Version_Current;
  stream << uiVersion;

  stream << m_sTexture;
  stream << m_RotationAxis.GetValue();
  stream << m_MaxDeviation;
}

void ezParticleTypeSpriteFactory::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)TypeSpriteVersion::Version_Current, "Invalid version {0}", uiVersion);

  stream >> m_sTexture;

  ezSpriteAxis::StorageType val;
  stream >> val;
  m_RotationAxis.SetValue(val);

  stream >> m_MaxDeviation;
}


void ezParticleTypeSprite::CreateRequiredStreams()
{
  CreateStream("Position", ezProcessingStream::DataType::Float3, &m_pStreamPosition, false);
  CreateStream("Size", ezProcessingStream::DataType::Float, &m_pStreamSize, false);
  CreateStream("Color", ezProcessingStream::DataType::Float4, &m_pStreamColor, false);
  CreateStream("RotationSpeed", ezProcessingStream::DataType::Float, &m_pStreamRotationSpeed, false);
  CreateStream("Axis", ezProcessingStream::DataType::Float3, &m_pStreamAxis, true);
}

void ezParticleTypeSprite::InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
{
  ezVec3* pAxis = m_pStreamAxis->GetWritableData<ezVec3>();
  ezRandom& rng = GetRNG();

  if (m_RotationAxis == ezSpriteAxis::Random)
  {
    for (ezUInt32 i = 0; i < uiNumElements; ++i)
    {
      const ezUInt64 uiElementIdx = uiStartIndex + i;

      pAxis[uiElementIdx] = ezVec3::CreateRandomDirection(rng);
    }
  }
  else
  {
    ezVec3 vNormal;

    if (m_RotationAxis == ezSpriteAxis::EmitterDirection)
    {
      vNormal = GetOwnerSystem()->GetTransform().m_Rotation.GetColumn(2).GetNormalized(); // Z axis
    }
    else if (m_RotationAxis == ezSpriteAxis::WorldUp)
    {
      ezCoordinateSystem coord;
      GetOwnerSystem()->GetWorld()->GetCoordinateSystem(GetOwnerSystem()->GetTransform().m_vPosition, coord);

      vNormal = coord.m_vUpDir;
    }

    if (m_MaxDeviation > ezAngle::Degree(1.0f))
    {
      // how to get from the X axis to the desired normal
      ezQuat qRotToDir;
      qRotToDir.SetShortestRotation(ezVec3(1, 0, 0), vNormal);

      for (ezUInt32 i = 0; i < uiNumElements; ++i)
      {
        const ezUInt64 uiElementIdx = uiStartIndex + i;
        const ezVec3 vRandomX = ezVec3::CreateRandomDeviationX(rng, m_MaxDeviation);

        pAxis[uiElementIdx] = qRotToDir * vRandomX;
      }
    }
    else
    {
      for (ezUInt32 i = 0; i < uiNumElements; ++i)
      {
        const ezUInt64 uiElementIdx = uiStartIndex + i;
        pAxis[uiElementIdx] = vNormal;
      }
    }
  }
}

void ezParticleTypeSprite::ExtractTypeRenderData(const ezView& view, ezExtractedRenderData* pExtractedRenderData, const ezTransform& instanceTransform, ezUInt64 uiExtractedFrame) const
{
  if (!m_hTexture.IsValid())
    return;

  if (GetOwnerSystem()->GetNumActiveParticles() == 0)
    return;

  // don't copy the data multiple times in the same frame, if the effect is instanced
  if (m_uiLastExtractedFrame != uiExtractedFrame)
  {
    m_uiLastExtractedFrame = uiExtractedFrame;

    const ezTime tCur = GetOwnerSystem()->GetWorld()->GetClock().GetAccumulatedTime();

    const ezVec3* pPosition = m_pStreamPosition->GetData<ezVec3>();
    const float* pSize = m_pStreamSize->GetData<float>();
    const ezColor* pColor = m_pStreamColor->GetData<ezColor>();
    const float* pRotationSpeed = m_pStreamRotationSpeed->GetData<float>();
    const ezVec3* pAxis = m_pStreamAxis->GetData<ezVec3>();

    if (m_GpuData == nullptr)
    {
      m_GpuData = EZ_DEFAULT_NEW(ezSpriteParticleDataContainer);
      m_GpuData->m_Content.SetCountUninitialized((ezUInt32)GetOwnerSystem()->GetMaxParticles());
    }

    ezSpriteParticleData* TempData = m_GpuData->m_Content.GetData();

    ezTransform t;

    for (ezUInt32 p = 0; p < (ezUInt32)GetOwnerSystem()->GetNumActiveParticles(); ++p)
    {
      TempData[p].Size = pSize[p];
      TempData[p].Color = pColor[p];
    }

    for (ezUInt32 p = 0; p < (ezUInt32)GetOwnerSystem()->GetNumActiveParticles(); ++p)
    {
      const ezVec3 vNormal = pAxis[p];
      const ezVec3 vTangentStart = vNormal.GetOrthogonalVector().GetNormalized();

      ezMat3 mRotation;
      mRotation.SetRotationMatrix(vNormal, ezAngle::Radian((float)(tCur.GetSeconds() * pRotationSpeed[p])));

      const ezVec3 vTangentX = mRotation * vTangentStart;

      TempData[p].Position = pPosition[p];
      TempData[p].TangentX = vTangentX;
      TempData[p].TangentZ = vTangentX.Cross(vNormal);
    }
  }

  /// \todo Is this batch ID correct?
  const ezUInt32 uiBatchId = m_hTexture.GetResourceIDHash();
  auto pRenderData = ezCreateRenderDataForThisFrame<ezParticleSpriteRenderData>(nullptr, uiBatchId);

  pRenderData->m_GlobalTransform = instanceTransform;
  pRenderData->m_uiNumParticles = (ezUInt32)GetOwnerSystem()->GetNumActiveParticles();
  pRenderData->m_hTexture = m_hTexture;
  pRenderData->m_GpuData = m_GpuData;

  /// \todo Generate a proper sorting key?
  const ezUInt32 uiSortingKey = 0;
  pExtractedRenderData->AddRenderData(pRenderData, ezDefaultRenderDataCategories::SimpleTransparent, uiSortingKey);
}


