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
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezRmlUiCanvas3DComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_RESOURCE_ACCESSOR_PROPERTY("Mesh", GetMesh, SetMesh)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Mesh_Static")),
    EZ_ACCESSOR_PROPERTY("TextureSize", GetTextureSize, SetTextureSize)->AddAttributes(new ezSuffixAttribute("px"), new ezDefaultValueAttribute(ezVec2U32(512, 512)), new ezClampValueAttribute(ezVec2U32(0), ezVec2U32(4096))),
    EZ_ACCESSOR_PROPERTY("DpiScale", GetDpiScale, SetDpiScale)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_ACCESSOR_PROPERTY("ClearStaleInput", GetClearStaleInput, SetClearStaleInput)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_ACCESSOR_PROPERTY("IsInteractive", IsInteractive, SetInteractive)->AddAttributes(new ezDefaultValueAttribute(true)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractGeometry, OnMsgExtractGeometry),
  }
  EZ_END_MESSAGEHANDLERS;
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

  s << m_vSize;
  s << m_bClearStaleInput;
  s << m_bIsInteractive;
  s << m_fDpiScale;
}

void ezRmlUiCanvas3DComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = inout_stream.GetStream();

  s >> m_vSize;
  s >> m_bClearStaleInput;
  s >> m_bIsInteractive;
  s >> m_fDpiScale;
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
      m_InputProvider.Update(ezRmlUiInputSnapshot{});
      m_bNeedsUpdate |= m_pContext->UpdateInput(ezVec2::MakeZero(), m_InputProvider);
      m_iInputAge = -1;
    }
  }

  m_bNeedsUpdate |= UpdateTexture();

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

  if (!GetMesh().IsValid())
  {
    ezLog::Dev("ezRmlUiCanvas3DComponent: a canvas doesn't have a mesh");
    return false;
  }

  ezCpuMeshResourceHandle hMesh = ezResourceManager::LoadResource<ezCpuMeshResource>(GetMesh().GetResourceID());
  if (!hMesh.IsValid())
  {
    ezLog::Dev("ezRmlUiCanvas3DComponent: canvas mesh is not valid");
    return false;
  }

  ezResourceLock<ezCpuMeshResource> pMesh(hMesh, ezResourceAcquireMode::AllowLoadingFallback);
  if (pMesh.GetAcquireResult() == ezResourceAcquireResult::LoadingFallback)
  {
    ezLog::Dev("ezRmlUiCanvas3DComponent: canvas mesh is not loaded yet");
    return false;
  }

  ezTransform worldToLocal = GetOwner()->GetGlobalTransform().GetInverse();
  ezVec3 vRayOriginMeshSpace = worldToLocal.TransformPosition(vRayOrigin);
  ezVec3 vRayDirMeshSpace = worldToLocal.TransformDirection(vRayDir).GetNormalized();

  ezVec2 vTexCoords;
  if (!RaycastMeshTexCoords(pMesh.GetPointer(), vRayOriginMeshSpace, vRayDirMeshSpace, vTexCoords))
  {
    ezLog::Dev("ezRmlUiCanvas3DComponent: raycast failed to hit any triangles");
    return false;
  }

  ezVec2 vCursorPos;
  vCursorPos.x = static_cast<float>(m_vSize.x) * vTexCoords.x;
  vCursorPos.y = static_cast<float>(m_vSize.y) * vTexCoords.y;

  ReceiveInput(vCursorPos, input);

  return true;
}

bool ezRmlUiCanvas3DComponent::RaycastMeshTexCoords(const ezCpuMeshResource* pMesh, const ezVec3& vRayOrigin, const ezVec3& vRayDir, ezVec2& out_vTexCoords, float FEpsilon)
{
  const ezMeshBufferResourceDescriptor& mesh = pMesh->GetDescriptor().MeshBufferDesc();

  if (mesh.GetTopology() != ezGALPrimitiveTopology::Triangles)
  {
    ezLog::Dev("RaycastMeshTexCoords: topology {} not supported", mesh.GetTopology());
    return false;
  }

  const ezUInt16* pIndexBuffer = reinterpret_cast<const ezUInt16*>(mesh.GetIndexBufferData().GetPtr());
  ezUInt32 uiNumIndices = mesh.GetIndexBufferData().GetCount() / 2;
  EZ_ASSERT_DEV(mesh.Uses32BitIndices() == false, "not implemented yet");

  float fClosestDist = 1e20f;
  ezUInt16 uiClosestIndex0, uiClosestIndex1, uiClosestIndex2;
  ezVec3 vClosestPos;

  for (ezUInt32 i = 0; i + 2 < uiNumIndices; i += 3)
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

ezResult ezRmlUiCanvas3DComponent::GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg)
{
  if (m_hMesh.IsValid())
  {
    ezResourceLock<ezMeshResource> pMesh(m_hMesh, ezResourceAcquireMode::AllowLoadingFallback);
    ref_bounds = pMesh->GetBounds();
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

void ezRmlUiCanvas3DComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  if (!m_hMesh.IsValid())
    return;

  if (msg.m_pView->GetCameraUsageHint() == ezCameraUsageHint::MainView || msg.m_pView->GetCameraUsageHint() == ezCameraUsageHint::EditorView)
  {
    if (m_pContext != nullptr && m_hTexture.IsInvalidated() == false)
    {
      ezRmlUi::GetSingleton()->ExtractContext(*m_pContext, m_hTexture);
    }
  }

  ezResourceLock<ezMeshResource> pMesh(m_hMesh, ezResourceAcquireMode::AllowLoadingFallback);
  ezArrayPtr<const ezMeshResourceDescriptor::SubMesh> parts = pMesh->GetSubMeshes();

  for (ezUInt32 uiPartIndex = 0; uiPartIndex < parts.GetCount(); ++uiPartIndex)
  {
    ezRmlUiMeshRenderData* pRenderData = ezCreateRenderDataForThisFrame<ezRmlUiMeshRenderData>(GetOwner());
    {
      pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform() * pRenderData->m_GlobalTransform;
      pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds();
      pRenderData->m_fSortingDepthOffset = 0;
      pRenderData->m_hMesh = m_hMesh;
      pRenderData->m_hTexture = m_hTexture;
      pRenderData->m_uiSubMeshIndex = uiPartIndex;
      pRenderData->m_uiUniqueID = GetUniqueIdForRendering();

      pRenderData->FillSortingKey();
    }

    msg.AddRenderData(pRenderData, ezDefaultRenderDataCategories::SimpleOpaque, ezRenderData::Caching::Never);
  }
}

void ezRmlUiCanvas3DComponent::OnMsgExtractGeometry(ezMsgExtractGeometry& ref_msg) const
{
  if (ref_msg.m_Mode != ezWorldGeoExtractionUtil::ExtractionMode::RenderMesh)
    return;

  // ignore invalid and created resources
  {
    ezMeshResourceHandle hRenderMesh = GetMesh();
    if (!hRenderMesh.IsValid())
      return;

    ezResourceLock<ezMeshResource> pRenderMesh(hRenderMesh, ezResourceAcquireMode::PointerOnly);
    if (pRenderMesh->GetBaseResourceFlags().IsAnySet(ezResourceFlags::IsCreatedResource))
      return;
  }

  ref_msg.AddMeshObject(GetOwner()->GetGlobalTransform(), ezResourceManager::LoadResource<ezCpuMeshResource>(GetMesh().GetResourceID()));
}

void ezRmlUiCanvas3DComponent::SetMesh(const ezMeshResourceHandle& hMesh)
{
  if (m_hMesh != hMesh)
  {
    m_hMesh = hMesh;

    TriggerLocalBoundsUpdate();
    InvalidateCachedRenderData();
  }
}

bool ezRmlUiCanvas3DComponent::UpdateTexture()
{
  if (m_vSize.x == 0 || m_vSize.y == 0)
    return false;

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  const ezGALTexture* pTexture = pDevice->GetTexture(m_hTexture);
  if (pTexture == nullptr || pTexture->GetDescription().m_uiWidth != m_vSize.x || pTexture->GetDescription().m_uiHeight != m_vSize.y)
  {
    if (pTexture != nullptr)
    {
      pDevice->DestroyTexture(m_hTexture);
    }

    ezGALTextureCreationDescription desc;
    desc.m_uiWidth = m_vSize.x;
    desc.m_uiHeight = m_vSize.y;
    desc.m_Format = ezGALResourceFormat::RGBAUByteNormalized;
    desc.m_ResourceAccess.m_bImmutable = false;
    if (ezMath::IsPowerOf2(m_vSize.x) && ezMath::IsPowerOf2(m_vSize.y))
    {
      desc.m_uiMipLevelCount = ezMath::Max(ezMath::Log2i(m_vSize.x), ezMath::Log2i(m_vSize.y)) - 2;
      desc.m_bAllowDynamicMipGeneration = true;
    }

    m_hTexture = pDevice->CreateTexture(desc);

    m_pContext->SetSize(m_vSize);
    m_pContext->SetDpiScale(m_fDpiScale);

    return true;
  }

  return false;
}

EZ_STATICLINK_FILE(RmlUiPlugin, RmlUiPlugin_Components_Implementation_RmlUiCanvas3DComponent);
