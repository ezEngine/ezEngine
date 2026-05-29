#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Math/Intersection.h>
#include <GameEngine/Gameplay/BlackboardComponent.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>
#include <RendererFoundation/Device/Device.h>
#include <RmlUiPlugin/Components/RmlUiCanvas3DComponent.h>

#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RmlUiPlugin/RmlUiContext.h>
#include <RmlUiPlugin/RmlUiSingleton.h>

static ezAtomicInteger32 s_RmlResourceCounter;

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezRmlUiCanvas3DComponent, 2, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_RESOURCE_ACCESSOR_PROPERTY("ProxyMesh", GetProxyMesh, SetProxyMesh)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Mesh_Static")),
    EZ_RESOURCE_ACCESSOR_PROPERTY("BaseMaterial", GetBaseMaterial, SetBaseMaterial)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Material"), new ezDefaultValueAttribute("{ 05af8d07-0b38-44a6-8d50-49731ae2625d }")),
    EZ_ACCESSOR_PROPERTY("MaterialIndex", GetMaterialIndex, SetMaterialIndex)->AddAttributes(new ezDefaultValueAttribute(0)),
    EZ_ACCESSOR_PROPERTY("TextureSlotName", GetTextureSlotName, SetTextureSlotName)->AddAttributes(new ezDefaultValueAttribute("BaseTexture")),
    EZ_ACCESSOR_PROPERTY("TextureSize", GetTextureSize, SetTextureSize)->AddAttributes(new ezSuffixAttribute("px"), new ezDefaultValueAttribute(ezVec2U32(512, 512)), new ezClampValueAttribute(ezVec2U32(0), ezVec2U32(4096))),
    EZ_ACCESSOR_PROPERTY("DpiScale", GetDpiScale, SetDpiScale)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_ACCESSOR_PROPERTY("ClearStaleInput", GetClearStaleInput, SetClearStaleInput)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_ACCESSOR_PROPERTY("IsInteractive", IsInteractive, SetInteractive)->AddAttributes(new ezDefaultValueAttribute(true)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Input/RmlUi"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezRmlUiCanvas3DComponent::ezRmlUiCanvas3DComponent()
{
  m_vSize = ezVec2U32(512, 512);
}

ezRmlUiCanvas3DComponent::~ezRmlUiCanvas3DComponent() = default;
ezRmlUiCanvas3DComponent& ezRmlUiCanvas3DComponent::operator=(ezRmlUiCanvas3DComponent&& rhs) = default;

void ezRmlUiCanvas3DComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  ezStreamWriter& s = inout_stream.GetStream();

  s << m_hBaseMaterial;
  s << m_uiMaterialIndex;
  s << m_sTextureSlotName;
  s << m_vSize;
  s << m_bClearStaleInput;
  s << m_bIsInteractive;
  s << m_fDpiScale;
  s << m_hProxyMesh;
}

void ezRmlUiCanvas3DComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = inout_stream.GetStream();

  s >> m_hBaseMaterial;
  s >> m_uiMaterialIndex;
  s >> m_sTextureSlotName;
  s >> m_vSize;
  s >> m_bClearStaleInput;
  s >> m_bIsInteractive;
  s >> m_fDpiScale;

  if (uiVersion >= 2)
  {
    s >> m_hProxyMesh;
  }
}

void ezRmlUiCanvas3DComponent::OnActivated()
{
  SUPER::OnActivated();

  GetOrCreateRmlContext()->ShowDocument();

  if (m_sTextureSlotName.IsEmpty())
  {
    m_sTextureSlotName.Assign("BaseTexture");
  }

  if (!m_hBaseMaterial.IsValid())
  {
    // use the "Fullbright" material as a fallback
    m_hBaseMaterial = ezResourceManager::LoadResource<ezMaterialResource>("{ 05af8d07-0b38-44a6-8d50-49731ae2625d }");
  }

  Update();

  if (m_hMaterial.IsValid())
  {
    ezMsgSetMeshMaterial msg;
    msg.m_hMaterial = m_hMaterial;
    msg.m_uiMaterialSlot = m_uiMaterialIndex;

    GetOwner()->PostMessageRecursive(msg, ezTime::MakeZero());
  }
}

void ezRmlUiCanvas3DComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  if (m_hMaterial.IsValid())
  {
    ezMsgSetMeshMaterial msg;
    msg.m_uiMaterialSlot = m_uiMaterialIndex;

    GetOwner()->PostMessageRecursive(msg, ezTime::MakeZero());
  }

  m_iInputAge = -1;
  m_hTexture.Invalidate();
  m_hMaterial.Invalidate();
  m_hCachedCpuMesh.Invalidate();
}

void ezRmlUiCanvas3DComponent::Update()
{
  if (m_pContext == nullptr)
    return;

  if (m_bClearStaleInput && m_iInputAge >= 0)
  {
    m_iInputAge += 1;
    if (m_iInputAge > 3)
    {
      m_InputProvider.Update(ezRmlUiInputSnapshot::MakeEmpty());
      m_bNeedsUpdate |= m_pContext->UpdateInput(ezVec2::MakeZero(), m_InputProvider);
      m_iInputAge = -1;
    }
  }

  m_bNeedsUpdate |= UpdateTextureAndMaterial();

  SUPER::Update();
}

bool ezRmlUiCanvas3DComponent::ReceiveInput(const ezVec2& vMousePosInsideCanvas, ezRmlUiInputSnapshot input)
{
  if (IsInteractive() && SUPER::ReceiveInput(vMousePosInsideCanvas, input))
  {
    m_iInputAge = 0;
    return true;
  }
  return false;
}

bool ezRmlUiCanvas3DComponent::RaycastInput(const ezVec3& vRayOrigin, const ezVec3& vRayDir, ezRmlUiInputSnapshot input)
{
  if (m_pContext == nullptr || !IsInteractive())
    return false;

  if (!m_hCachedCpuMesh.IsValid())
  {
    ezMeshResourceHandle hMesh = m_hProxyMesh;

    if (m_hProxyMesh.IsValid())
    {
      hMesh = m_hProxyMesh;
    }
    else
    {
      ezMeshComponent* pMeshComponent = nullptr;
      if (GetOwner()->TryGetComponentOfBaseType(pMeshComponent))
      {
        hMesh = pMeshComponent->GetMesh();
      }
    }

    if (!hMesh.IsValid())
    {
      ezLog::Error("ezRmlUiCanvas3DComponent '{0}' has no mesh to raycast against.", GetOwner()->GetName());
      SetInteractive(false); // deactivate, to prevent repeated errors
      return false;
    }

    m_hCachedCpuMesh = ezResourceManager::LoadResource<ezCpuMeshResource>(hMesh.GetResourceID());
  }

  ezResourceLock<ezCpuMeshResource> pMesh(m_hCachedCpuMesh, ezResourceAcquireMode::AllowLoadingFallback);
  if (pMesh.GetAcquireResult() == ezResourceAcquireResult::LoadingFallback)
  {
    // not yet loaded, skip a frame
    return false;
  }

  if (pMesh.GetAcquireResult() != ezResourceAcquireResult::Final)
  {
    ezLog::Error("ezRmlUiCanvas3DComponent '{0}' failed to get CPU mesh for raycast.", GetOwner()->GetName());
    SetInteractive(false); // deactivate, to prevent repeated errors
    return false;
  }

  const ezTransform tOwner = GetOwner()->GetGlobalTransform();

  if (ezMath::IsZero(tOwner.GetMaxScale(), 0.001f))
  {
    // object was scaled to zero
    return false;
  }

  const ezTransform worldToLocal = tOwner.GetInverse();
  const ezVec3 vRayOriginMeshSpace = worldToLocal.TransformPosition(vRayOrigin);
  const ezVec3 vRayDirMeshSpace = worldToLocal.TransformDirection(vRayDir).GetNormalized();

  const ezMeshResourceDescriptor& desc = pMesh->GetDescriptor();
  for (ezUInt32 uiSubMeshIndex = 0; uiSubMeshIndex < desc.GetSubMeshes().GetCount(); ++uiSubMeshIndex)
  {
    const ezMeshResourceDescriptor::SubMesh& submesh = desc.GetSubMeshes()[uiSubMeshIndex];
    if (submesh.m_uiMaterialIndex != m_uiMaterialIndex)
      continue;

    ezVec2 vTexCoords;
    if (!RaycastMeshTexCoords(pMesh.GetPointer(), uiSubMeshIndex, vRayOriginMeshSpace, vRayDirMeshSpace, vTexCoords))
      continue;

    ezVec2 vCursorPos;
    vCursorPos.x = static_cast<float>(m_vSize.x) * vTexCoords.x;
    vCursorPos.y = static_cast<float>(m_vSize.y) * vTexCoords.y;

    ReceiveInput(vCursorPos, input);

    return true;
  }

  return false;
}

bool ezRmlUiCanvas3DComponent::RaycastMeshTexCoords(const ezCpuMeshResource* pMesh, ezUInt32 uiSubMeshIndex, const ezVec3& vRayOrigin, const ezVec3& vRayDir, ezVec2& out_vTexCoords, float FEpsilon)
{
  const ezMeshBufferResourceDescriptor& mesh = pMesh->GetDescriptor().MeshBufferDesc();

  if (mesh.GetTopology() != ezGALPrimitiveTopology::Triangles)
  {
    ezLog::Warning("Topology '{}' is not supported for raycasting.", mesh.GetTopology());
    return false;
  }

  const ezUInt16* pIndexBuffer = reinterpret_cast<const ezUInt16*>(mesh.GetIndexBufferData().GetPtr());
  ezUInt32 uiNumIndices = mesh.GetIndexBufferData().GetCount() / 2;
  if (mesh.Uses32BitIndices())
  {
    ezLog::Warning("Meshes with 32 bit indices are not supported for raycasting.");
    return false;
  }

  ezMeshResourceDescriptor::SubMesh submesh = pMesh->GetDescriptor().GetSubMeshes()[uiSubMeshIndex];
  ezUInt32 uiFirstIndex = submesh.m_uiFirstPrimitive * 3;
  ezUInt32 uiLastIndex = uiFirstIndex + submesh.m_uiPrimitiveCount * 3;
  EZ_ASSERT_DEV(uiLastIndex <= uiNumIndices, "something is wrong");

  float fClosestDist = 1e20f;
  ezUInt16 uiClosestIndex0 = 0, uiClosestIndex1 = 0, uiClosestIndex2 = 0;
  ezVec3 vClosestPos;

  for (ezUInt32 i = uiFirstIndex; i + 2 < uiLastIndex; i += 3)
  {
    ezUInt16 uiIndex0 = pIndexBuffer[i];
    ezUInt16 uiIndex1 = pIndexBuffer[i + 1];
    ezUInt16 uiIndex2 = pIndexBuffer[i + 2];

    ezVec3 vVertex0 = mesh.GetPosition(uiIndex0);
    ezVec3 vVertex1 = mesh.GetPosition(uiIndex1);
    ezVec3 vVertex2 = mesh.GetPosition(uiIndex2);

    float fDist;
    ezVec3 vPos;

    bool bHit = ezIntersectionUtils::RayTriangleIntersectionCullBackface(vRayOrigin, vRayDir, vVertex0, vVertex1, vVertex2, vPos, &fDist, nullptr);
    if (!bHit || fDist > fClosestDist)
      continue;

    fClosestDist = fDist;
    uiClosestIndex0 = uiIndex0;
    uiClosestIndex1 = uiIndex1;
    uiClosestIndex2 = uiIndex2;
    vClosestPos = vPos;
  }

  if (fClosestDist < 1e20f)
  {
    out_vTexCoords = ezVec2::MakeZero();
    out_vTexCoords += mesh.GetTexCoord0(uiClosestIndex0) * vClosestPos.x;
    out_vTexCoords += mesh.GetTexCoord0(uiClosestIndex1) * vClosestPos.y;
    out_vTexCoords += mesh.GetTexCoord0(uiClosestIndex2) * vClosestPos.z;
    out_vTexCoords.x = ezMath::Fraction(ezMath::Abs(out_vTexCoords.x));
    out_vTexCoords.y = ezMath::Fraction(ezMath::Abs(out_vTexCoords.y));

    return true;
  }

  return false;
}

void ezRmlUiCanvas3DComponent::SetBaseMaterial(const ezMaterialResourceHandle& hMaterial)
{
  if (hMaterial == m_hBaseMaterial)
    return;

  m_hBaseMaterial = hMaterial;
  m_hMaterial.Invalidate();
}

void ezRmlUiCanvas3DComponent::SetMaterialIndex(ezUInt32 uiMaterialIndex)
{
  if (uiMaterialIndex == m_uiMaterialIndex)
    return;

  const ezUInt32 uiPrevIndex = m_uiMaterialIndex;
  m_uiMaterialIndex = uiMaterialIndex;

  if (m_hMaterial.IsValid() && IsActiveAndInitialized())
  {
    ezMsgSetMeshMaterial msg;

    msg.m_uiMaterialSlot = uiPrevIndex;
    GetOwner()->PostMessageRecursive(msg, ezTime::MakeZero());

    msg.m_uiMaterialSlot = m_uiMaterialIndex;
    msg.m_hMaterial = m_hMaterial;
    GetOwner()->PostMessageRecursive(msg, ezTime::MakeZero());
  }
}

void ezRmlUiCanvas3DComponent::SetTextureSlotName(ezStringView sName)
{
  if (m_sTextureSlotName != sName)
  {
    m_hMaterial.Invalidate();
    m_sTextureSlotName.Assign(sName);
  }
}

void ezRmlUiCanvas3DComponent::SetTextureSize(const ezVec2U32& vSize)
{
  if (m_vSize != vSize)
  {
    m_vSize.x = ezMath::Min(vSize.x, 4096u);
    m_vSize.y = ezMath::Min(vSize.y, 4096u);

    if (m_pContext != nullptr)
    {
      m_pContext->SetSize(m_vSize);
    }

    // recreate the texture next update
    m_hTexture.Invalidate();
  }
}

void ezRmlUiCanvas3DComponent::SetDpiScale(float fDpiScale)
{
  fDpiScale = fDpiScale > 0.0f ? fDpiScale : 1.0f;

  if (fDpiScale == m_fDpiScale)
    return;

  m_fDpiScale = fDpiScale;
  m_bNeedsUpdate = true;

  if (m_pContext != nullptr)
  {
    m_pContext->SetDpiScale(m_fDpiScale);
  }
}

void ezRmlUiCanvas3DComponent::SetClearStaleInput(bool bClearStaleInput)
{
  m_bClearStaleInput = bClearStaleInput;
}

void ezRmlUiCanvas3DComponent::SetInteractive(bool bIsInteractive)
{
  m_bIsInteractive = bIsInteractive;
}

void ezRmlUiCanvas3DComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  if (m_pContext != nullptr && m_hTexture.IsValid())
  {
    ezResourceLock<ezTexture2DResource> pTexture(m_hTexture, ezResourceAcquireMode::BlockTillLoaded);
    // The texture is also used in shadow maps as we don't know what the depth shader compiler is culling or what the material is using the texture for.
    ezRenderWorld::AddViewDependency(*msg.m_pView, pTexture->GetGALTexture(), ezGALResourceState::ShaderResource, ezGALShaderStageFlags::PixelShader);
    if (msg.m_pView->GetCameraUsageHint() != ezCameraUsageHint::MainView && msg.m_pView->GetCameraUsageHint() != ezCameraUsageHint::EditorView)
      return;

    ezRmlUi::GetSingleton()->ExtractContext(*m_pContext, pTexture->GetGALTexture());
  }
}

void ezRmlUiCanvas3DComponent::OnMsgReload(ezMsgRmlUiReload& msg)
{
  SUPER::OnMsgReload(msg);

  m_hMaterial.Invalidate();
  m_hTexture.Invalidate();
}

bool ezRmlUiCanvas3DComponent::UpdateTextureAndMaterial()
{
  if (m_vSize.x == 0 || m_vSize.y == 0 || !m_hBaseMaterial.IsValid() || m_sTextureSlotName.IsEmpty())
    return false;

  bool bNeedsUpdate = false;

  if (!m_hTexture.IsValid())
  {
    bNeedsUpdate = true;

    ezTexture2DResourceDescriptor desc;
    desc.m_DescGAL.m_uiWidth = m_vSize.x;
    desc.m_DescGAL.m_uiHeight = m_vSize.y;
    desc.m_DescGAL.m_Format = ezGALResourceFormat::RGBAUByteNormalized;
    desc.m_DescGAL.m_ResourceAccess.m_bImmutable = false;
    desc.m_SamplerDesc.m_AddressU = ezImageAddressMode::ClampBorder;
    desc.m_SamplerDesc.m_AddressV = ezImageAddressMode::ClampBorder;
    desc.m_SamplerDesc.m_AddressW = ezImageAddressMode::ClampBorder;
    desc.m_SamplerDesc.m_BorderColor = ezColor::MakeZero();
    desc.m_SamplerDesc.m_MinFilter = ezGALTextureFilterMode::Anisotropic;
    desc.m_SamplerDesc.m_MagFilter = ezGALTextureFilterMode::Anisotropic;
    desc.m_SamplerDesc.m_uiMaxAnisotropy = 8;

    if (ezMath::IsPowerOf2(m_vSize.x) && ezMath::IsPowerOf2(m_vSize.y))
    {
      desc.m_DescGAL.m_uiMipLevelCount = ezMath::Max(ezMath::Log2i(m_vSize.x), ezMath::Log2i(m_vSize.y)) - 2;
      desc.m_DescGAL.m_TextureFlags.Add(ezGALTextureUsageFlags::RenderTarget);
    }

    ezStringBuilder resourceName = "RmlUiCanvas3DComponent_Texture";
    resourceName.AppendFormat("_{0}", s_RmlResourceCounter.Increment());

    m_hTexture = ezResourceManager::CreateResource<ezTexture2DResource>(resourceName, std::move(desc));

    m_pContext->SetSize(m_vSize);
    m_pContext->SetDpiScale(m_fDpiScale);

    m_hMaterial.Invalidate();
  }

  if (!m_hMaterial.IsValid())
  {
    bNeedsUpdate = true;

    ezMaterialResourceDescriptor desc;
    desc.m_hBaseMaterial = m_hBaseMaterial;
    auto& tb = desc.m_Texture2DBindings.ExpandAndGetRef();
    tb.m_Name = m_sTextureSlotName;
    tb.m_Value = m_hTexture;

    ezStringBuilder resourceName = "RmlUiCanvas3DComponent_Material";
    resourceName.AppendFormat("_{0}", s_RmlResourceCounter.Increment());

    m_hMaterial = ezResourceManager::CreateResource<ezMaterialResource>(resourceName, std::move(desc));

    ezMsgSetMeshMaterial msg;
    msg.m_hMaterial = m_hMaterial;
    msg.m_uiMaterialSlot = m_uiMaterialIndex;
    GetOwner()->PostMessageRecursive(msg, ezTime::MakeZero());
  }

  return bNeedsUpdate;
}

EZ_STATICLINK_FILE(RmlUiPlugin, RmlUiPlugin_Components_Implementation_RmlUiCanvas3DComponent);
