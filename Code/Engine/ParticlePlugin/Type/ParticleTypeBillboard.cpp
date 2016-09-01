#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/Type/ParticleTypeBillboard.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Textures/TextureResource.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleTypeBillboardFactory, 1, ezRTTIDefaultAllocator<ezParticleTypeBillboardFactory>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Texture", m_sTexture)->AddAttributes(new ezAssetBrowserAttribute("Texture 2D")),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleTypeBillboard, 1, ezRTTIDefaultAllocator<ezParticleTypeBillboard>)
EZ_END_DYNAMIC_REFLECTED_TYPE

const ezRTTI* ezParticleTypeBillboardFactory::GetTypeType() const
{
  return ezGetStaticRTTI<ezParticleTypeBillboard>();
}


void ezParticleTypeBillboardFactory::CopyTypeProperties(ezParticleType* pObject) const
{
  ezParticleTypeBillboard* pType = static_cast<ezParticleTypeBillboard*>(pObject);

  pType->m_hTexture.Invalidate();

  if (!m_sTexture.IsEmpty())
    pType->m_hTexture = ezResourceManager::LoadResource<ezTextureResource>(m_sTexture);
}

enum class TypeBillboardVersion
{
  Version_0 = 0,
  Version_1,
  Version_2, // added texture

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void ezParticleTypeBillboardFactory::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = (int)TypeBillboardVersion::Version_Current;
  stream << uiVersion;

  stream << m_sTexture;
}

void ezParticleTypeBillboardFactory::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)TypeBillboardVersion::Version_Current, "Invalid version %u", uiVersion);

  if (uiVersion >= 2)
  {
    stream >> m_sTexture;
  }
}

void ezParticleTypeBillboard::CreateRequiredStreams()
{
  CreateStream("Position", ezStream::DataType::Float3, &m_pStreamPosition);
  CreateStream("Size", ezStream::DataType::Float, &m_pStreamSize);
  CreateStream("Color", ezStream::DataType::Float4, &m_pStreamColor);
}

struct EZ_ALIGN_16(ParticleData)
{
  ezVec3 m_vPosition;
  float m_fSize;
  ezColorLinearUB m_Color;
  float dummy[3];
};

EZ_CHECK_AT_COMPILETIME(sizeof(ParticleData) == 32);

#define DEBUG_BUFFER_SIZE (1024 * 256)
#define PARTICLES_PER_BATCH (DEBUG_BUFFER_SIZE / sizeof(ParticleData))

void ezParticleTypeBillboard::CreateDataBuffer(ezUInt32 uiStructSize) const
{
  if (m_hDataBuffer.IsInvalidated())
  {
    ezGALBufferCreationDescription desc;
    desc.m_uiStructSize = uiStructSize;
    desc.m_uiTotalSize = DEBUG_BUFFER_SIZE;
    desc.m_BufferType = ezGALBufferType::Generic;
    desc.m_bUseAsStructuredBuffer = true;
    desc.m_bAllowShaderResourceView = true;
    desc.m_ResourceAccess.m_bImmutable = false;

    m_hDataBuffer = ezGALDevice::GetDefaultDevice()->CreateBuffer(desc);
  }
}

void ezParticleTypeBillboard::Render(const ezRenderViewContext& renderViewContext, ezRenderPipelinePass* pPass) const
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezGALContext* pGALContext = renderViewContext.m_pRenderContext->GetGALContext();

  const ezVec3* pPosition = m_pStreamPosition->GetData<ezVec3>();
  const float* pSize = m_pStreamSize->GetData<float>();
  const ezColor* pColor = m_pStreamColor->GetData<ezColor>();

  if (pPosition == nullptr || pSize == nullptr || pColor == nullptr)
    return;

  ezUInt32 uiNumParticles = (ezUInt32)GetOwnerSystem()->GetNumActiveParticles();

  ezShaderResourceHandle hDebugPrimitiveShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Particles/Billboards.ezShader");
  CreateDataBuffer(sizeof(ParticleData));

  renderViewContext.m_pRenderContext->BindShader(hDebugPrimitiveShader);
  renderViewContext.m_pRenderContext->BindBuffer(ezGALShaderStage::VertexShader, "particleData", pDevice->GetDefaultResourceView(m_hDataBuffer));

  if (m_hTexture.IsValid())
  {
    renderViewContext.m_pRenderContext->BindTexture(ezGALShaderStage::PixelShader, "ParticleTexture", m_hTexture);
  }

  static ParticleData particleData[PARTICLES_PER_BATCH];

  while (uiNumParticles > 0)
  {
    const ezUInt32 uiNumParticlesInBatch = ezMath::Min<ezUInt32>(uiNumParticles, PARTICLES_PER_BATCH);

    for (ezUInt32 i = 0; i < uiNumParticlesInBatch; ++i)
    {
      particleData[i].m_vPosition = pPosition[i];
      particleData[i].m_fSize = pSize[i];
      particleData[i].m_Color = pColor[i];
    }

    pGALContext->UpdateBuffer(m_hDataBuffer, 0, ezMakeArrayPtr(particleData, uiNumParticlesInBatch).ToByteArray());

    renderViewContext.m_pRenderContext->BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr, ezGALPrimitiveTopology::Triangles, uiNumParticlesInBatch * 2);
    renderViewContext.m_pRenderContext->DrawMeshBuffer();

    uiNumParticles -= uiNumParticlesInBatch;

    pPosition += uiNumParticlesInBatch;
    pSize += uiNumParticlesInBatch;
    pColor += uiNumParticlesInBatch;
  }
}


