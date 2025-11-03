#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <RmlUiPlugin/Components/RmlUiCanvas3DComponent.h>
#include <RmlUiPlugin/Implementation/BlackboardDataBinding.h>
#include <RmlUiPlugin/Implementation/RmlUiRenderData.h>
#include <RmlUiPlugin/RmlUiContext.h>
#include <RmlUiPlugin/RmlUiSingleton.h>

#include <Core/Input/InputManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Gameplay/BlackboardComponent.h>
#include <Foundation/Math/Intersection.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezRmlUiCanvas3DComponent, 2, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_RESOURCE_ACCESSOR_PROPERTY("ProxyMesh", GetProxyMesh, SetProxyMesh)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Mesh_Static")),
    EZ_RESOURCE_ACCESSOR_PROPERTY("BaseMaterial", GetBaseMaterial, SetBaseMaterial)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Material")),
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

  s << m_hMaterial;
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

  s >> m_hMaterial;
  s >> m_uiMaterialIndex;
  s >> m_sTextureSlotName;
  s >> m_vSize;
  s >> m_bClearStaleInput;
  s >> m_bIsInteractive;
  s >> m_fDpiScale;
  if (uiVersion >= 2)
    s >> m_hProxyMesh;
}

void ezRmlUiCanvas3DComponent::OnActivated()
{
  SUPER::OnActivated();

  m_bNeedsUpdate |= UpdateTextureAndMaterial();

  if (m_hMaterial.IsValid())
  {
    ezMsgSetMeshMaterial msg;
    msg.m_hMaterial = m_hMaterial;
    msg.m_uiMaterialSlot = m_uiMaterialIndex;

    GetOwner()->SendMessage(msg);
  }
}

void ezRmlUiCanvas3DComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  if (m_hTexture.IsValid())
  {
    ezMsgSetMeshMaterial msg;
    msg.m_uiMaterialSlot = m_uiMaterialIndex;

    GetOwner()->SendMessage(msg);
  }
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

  ezMeshResourceHandle hMesh = m_hProxyMesh;
  bool bUsingProxyMesh = m_hProxyMesh.IsValid();

  if (!bUsingProxyMesh)
  {
    ezMeshComponent* pMeshComponent = nullptr;
    if (GetOwner()->TryGetComponentOfBaseType(pMeshComponent))
      hMesh = pMeshComponent->GetMesh();
  }

  if (!hMesh.IsValid())
  {
    ezLog::Warning("ezRmlUiCanvas3DComponent '{0}' has no mesh to raycast against.", GetOwner()->GetName());
    return false;
  }

  ezCpuMeshResourceHandle hCpuMesh = ezResourceManager::LoadResource<ezCpuMeshResource>(hMesh.GetResourceID());
  if (!hCpuMesh.IsValid())
  {
    ezLog::Warning("Raycast against '{0}' failed because mesh resource is invalid.", GetOwner()->GetName());
    return false;
  }

  ezResourceLock<ezCpuMeshResource> pMesh(hCpuMesh, ezResourceAcquireMode::AllowLoadingFallback);
  if (pMesh.GetAcquireResult() == ezResourceAcquireResult::LoadingFallback)
  {
    ezLog::Warning("Raycast against '{0}' failed because mesh resource was not available.", GetOwner()->GetName());
    return false;
  }

  ezTransform worldToLocal = GetOwner()->GetGlobalTransform().GetInverse();
  ezVec3 vRayOriginMeshSpace = worldToLocal.TransformPosition(vRayOrigin);
  ezVec3 vRayDirMeshSpace = worldToLocal.TransformDirection(vRayDir).GetNormalized();

  const ezMeshResourceDescriptor& desc = pMesh->GetDescriptor();
  for (ezUInt32 uiSubMeshIndex = 0; uiSubMeshIndex < desc.GetSubMeshes().GetCount(); ++uiSubMeshIndex) {
    const ezMeshResourceDescriptor::SubMesh& submesh = desc.GetSubMeshes()[uiSubMeshIndex];
    if (!bUsingProxyMesh && submesh.m_uiMaterialIndex != m_uiMaterialIndex)
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

  ezUInt32 uiPrevIndex = m_uiMaterialIndex;
  m_uiMaterialIndex = uiMaterialIndex;

  if (m_hMaterial.IsValid() && IsActiveAndInitialized())
  {
    ezMsgSetMeshMaterial msg;

    msg.m_uiMaterialSlot = uiPrevIndex;
    GetOwner()->SendMessage(msg);

    msg.m_uiMaterialSlot = m_uiMaterialIndex;
    msg.m_hMaterial = m_hMaterial;
    GetOwner()->SendMessage(msg);
  }
}

void ezRmlUiCanvas3DComponent::SetTextureSlotName(const char* szName)
{
  ezHashedString sPrevTextureSlotName = m_sTextureSlotName;

  m_sTextureSlotName.Assign(szName);

  m_bRebindTexture = m_sTextureSlotName != sPrevTextureSlotName;
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
  if (msg.m_pView->GetCameraUsageHint() == ezCameraUsageHint::MainView || msg.m_pView->GetCameraUsageHint() == ezCameraUsageHint::EditorView)
  {
    if (m_pContext != nullptr && m_hTexture.IsValid())
    {
      ezResourceLock<ezTexture2DResource> pTexture(m_hTexture, ezResourceAcquireMode::AllowLoadingFallback);
      if (pTexture.GetAcquireResult() != ezResourceAcquireResult::Final)
        return;

      ezRmlUi::GetSingleton()->ExtractContext(*m_pContext, pTexture->GetGALTexture());
    }
  }
}

bool ezRmlUiCanvas3DComponent::UpdateTextureAndMaterial()
{
  if (m_vSize.x == 0 || m_vSize.y == 0)
    return false;

  bool bShouldRecreateTexture = !m_hTexture.IsValid();

  if (m_hTexture.IsValid())
  {
    ezResourceLock<ezTexture2DResource> pTexture(m_hTexture, ezResourceAcquireMode::AllowLoadingFallback);
    if (pTexture.GetAcquireResult() != ezResourceAcquireResult::Final)
      return false;

    bShouldRecreateTexture = pTexture->GetWidth() != m_vSize.x || pTexture->GetHeight() != m_vSize.y;
  }

  if (bShouldRecreateTexture)
  {
    ezTexture2DResourceDescriptor desc;
    desc.m_DescGAL.m_uiWidth = m_vSize.x;
    desc.m_DescGAL.m_uiHeight = m_vSize.y;
    desc.m_DescGAL.m_Format = ezGALResourceFormat::RGBAUByteNormalized;
    desc.m_DescGAL.m_ResourceAccess.m_bImmutable = false;
    if (ezMath::IsPowerOf2(m_vSize.x) && ezMath::IsPowerOf2(m_vSize.y))
    {
      desc.m_DescGAL.m_uiMipLevelCount = ezMath::Max(ezMath::Log2i(m_vSize.x), ezMath::Log2i(m_vSize.y)) - 2;
      desc.m_DescGAL.m_bAllowDynamicMipGeneration = true;
    }
    
    ezStringBuilder resourceName = "RmlUiCanvas3DComponent_Texture";
    resourceName.AppendFormat("_{0}", ezUuid::MakeUuid());

    m_hTexture = ezResourceManager::CreateResource<ezTexture2DResource>(resourceName, std::move(desc));

    m_pContext->SetSize(m_vSize);
    m_pContext->SetDpiScale(m_fDpiScale);

    m_bRebindTexture = true;
  }

  if (!m_hMaterial.IsValid())
  {
    if (!m_hBaseMaterial.IsValid())
      return bShouldRecreateTexture;

    ezMaterialResourceDescriptor desc;
    desc.m_hBaseMaterial = m_hBaseMaterial;

    ezStringBuilder resourceName = "RmlUiCanvas3DComponent_Material";
    resourceName.AppendFormat("_{0}", ezUuid::MakeUuid());

    m_hMaterial = ezResourceManager::CreateResource<ezMaterialResource>(resourceName, std::move(desc));

    ezMsgSetMeshMaterial msg;
    msg.m_hMaterial = m_hMaterial;
    msg.m_uiMaterialSlot = m_uiMaterialIndex;
    GetOwner()->SendMessage(msg);

    m_bRebindTexture = true;
  }

  if (m_bRebindTexture)
  {
    ezResourceLock<ezMaterialResource> pMaterial(m_hMaterial, ezResourceAcquireMode::BlockTillLoaded);

    pMaterial->ResetResource();
    pMaterial->SetTexture2DBinding(m_sTextureSlotName, m_hTexture);

    m_bRebindTexture = false;
  }

  return bShouldRecreateTexture;
}

EZ_STATICLINK_FILE(RmlUiPlugin, RmlUiPlugin_Components_Implementation_RmlUiCanvas3DComponent);
