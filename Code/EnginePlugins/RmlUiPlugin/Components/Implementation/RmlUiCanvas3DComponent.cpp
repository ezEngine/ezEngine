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
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezRmlUiCanvas3DComponent, 3, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_RESOURCE_ACCESSOR_PROPERTY("Mesh", GetMesh, SetMesh)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Mesh_Static")),
    EZ_RESOURCE_ACCESSOR_PROPERTY("RmlFile", GetRmlResource, SetRmlResource)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Rml_UI")),
    EZ_ACCESSOR_PROPERTY("TextureSize", GetTextureSize, SetTextureSize)->AddAttributes(new ezSuffixAttribute("px"), new ezDefaultValueAttribute(ezVec2U32(512, 512)), new ezClampValueAttribute(ezVec2U32(0), ezVec2U32(4096))),
    EZ_ACCESSOR_PROPERTY("AutobindBlackboards", GetAutobindBlackboards, SetAutobindBlackboards)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_ACCESSOR_PROPERTY("OnDemandUpdate", GetOnDemandUpdate, SetOnDemandUpdate)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_ACCESSOR_PROPERTY("ClearStaleInput", GetClearStaleInput, SetClearStaleInput)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_ACCESSOR_PROPERTY("IsInteractive", IsInteractive, SetInteractive)->AddAttributes(new ezDefaultValueAttribute(true)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractGeometry, OnMsgExtractGeometry),
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnMsgExtractRenderData),
    EZ_MESSAGE_HANDLER(ezMsgRmlUiReload, OnMsgReload)
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

ezRmlUiCanvas3DComponent::ezRmlUiCanvas3DComponent() = default;
ezRmlUiCanvas3DComponent::~ezRmlUiCanvas3DComponent() = default;
ezRmlUiCanvas3DComponent& ezRmlUiCanvas3DComponent::operator=(ezRmlUiCanvas3DComponent&& rhs) = default;

void ezRmlUiCanvas3DComponent::Initialize()
{
  SUPER::Initialize();

  UpdateAutobinding();
}

void ezRmlUiCanvas3DComponent::Deinitialize()
{
  SUPER::Deinitialize();

  ezGALDevice::GetDefaultDevice()->DestroyTexture(m_hTexture);

  if (m_pContext != nullptr)
  {
    ezRmlUi::GetSingleton()->DeleteContext(m_pContext);
    m_pContext = nullptr;
  }

  m_DataBindings.Clear();
}

void ezRmlUiCanvas3DComponent::OnActivated()
{
  SUPER::OnActivated();

  GetOrCreateRmlContext()->ShowDocument();

  // Update once to ensure correct initial state
  Update();
}

void ezRmlUiCanvas3DComponent::OnDeactivated()
{
  m_pContext->HideDocument();

  SUPER::OnDeactivated();
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

  const ezTime tDiff = ezClock::GetGlobalClock()->GetTimeDiff();
  m_bNeedsUpdate |= m_pContext->GetNextUpdateDelay() < ezMath::Max(tDiff.GetSeconds(), 1.0 / 240.0);

  m_bNeedsUpdate |= UpdateTexture();

  for (auto& pDataBinding : m_DataBindings)
  {
    if (pDataBinding != nullptr)
    {
      m_bNeedsUpdate |= pDataBinding->Update();
    }
  }

  if (m_bNeedsUpdate || m_bOnDemandUpdate == false)
  {
    m_pContext->Update();
  }

  m_bNeedsUpdate = false;
}

void ezRmlUiCanvas3DComponent::ReceiveInput(const ezVec2& vMousePosInsideCanvas, ezRmlUiInputSnapshot input)
{
  if (m_pContext == nullptr || !IsInteractive())
    return;

  m_InputProvider.Update(input);
  m_bNeedsUpdate |= m_pContext->UpdateInput(vMousePosInsideCanvas, m_InputProvider);
  m_iInputAge = 0;
}

void ezRmlUiCanvas3DComponent::RaycastInput(const ezVec3& vRayOrigin, const ezVec3& vRayDir, ezRmlUiInputSnapshot input)
{
  if (m_pContext == nullptr || !IsInteractive())
    return;

  if (!GetMesh().IsValid())
  {
    ezLog::Dev("ezRmlUiCanvas3DComponent: a canvas doesn't have a mesh");
    return;
  }

  ezCpuMeshResourceHandle hMesh = ezResourceManager::LoadResource<ezCpuMeshResource>(GetMesh().GetResourceID());
  if (!hMesh.IsValid())
  {
    ezLog::Dev("ezRmlUiCanvas3DComponent: canvas mesh is not valid");
    return;
  }

  ezResourceLock<ezCpuMeshResource> pMesh(hMesh, ezResourceAcquireMode::AllowLoadingFallback);
  if (pMesh.GetAcquireResult() == ezResourceAcquireResult::LoadingFallback)
  {
    ezLog::Dev("ezRmlUiCanvas3DComponent: canvas mesh is not loaded yet");
    return;
  }

  ezTransform worldToLocal = GetOwner()->GetGlobalTransform().GetInverse();
  ezVec3 vRayOriginMeshSpace = worldToLocal.TransformPosition(vRayOrigin);
  ezVec3 vRayDirMeshSpace = worldToLocal.TransformDirection(vRayDir).GetNormalized();

  ezVec2 vTexCoords;
  if (!RaycastMeshTexCoords(pMesh.GetPointer(), vRayOriginMeshSpace, vRayDirMeshSpace, vTexCoords))
  {
    ezLog::Dev("ezRmlUiCanvas3DComponent: raycast failed to hit any triangles");
    return;
  }

  ezVec2 vCursorPos;
  vCursorPos.x = static_cast<float>(m_vTextureSize.x) * vTexCoords.x;
  vCursorPos.y = static_cast<float>(m_vTextureSize.y) * vTexCoords.y;

  ReceiveInput(vCursorPos, input);
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

  for (ezUInt32 uiIndex = 0; uiIndex + 2 < uiNumIndices; uiIndex += 3)
  {
    // perform ray-triangle intersection test as described in https://www.graphics.cornell.edu/pubs/1997/MT97.pdf

    ezUInt16 i0 = pIndexBuffer[uiIndex];
    ezUInt16 i1 = pIndexBuffer[uiIndex + 1];
    ezUInt16 i2 = pIndexBuffer[uiIndex + 2];

    ezVec3 v0 = mesh.GetPosition(i0);
    ezVec3 v1 = mesh.GetPosition(i1);
    ezVec3 v2 = mesh.GetPosition(i2);

    ezVec3 edge1 = v1 - v0;
    ezVec3 edge2 = v2 - v0;

    ezVec3 pvec = vRayDir.CrossRH(edge2);

    float det = edge1.Dot(pvec);
    if (det < FEpsilon)
      continue;

    ezVec3 tvec = vRayOrigin - v0;

    float u = tvec.Dot(pvec);
    if (u < 0 || u > det)
      continue;

    ezVec3 qvec = tvec.CrossRH(edge1);

    float v = qvec.Dot(vRayDir);
    if (v < 0 || u + v > det)
      continue;

    float t = edge2.Dot(qvec);
    float inv_det = 1.0f / det;
    t *= inv_det;
    u *= inv_det;
    v *= inv_det;

    out_vTexCoords = ezVec2::MakeZero();
    out_vTexCoords += mesh.GetTexCoord0(i0) * (1.0f - u - v);
    out_vTexCoords += mesh.GetTexCoord0(i1) * u;
    out_vTexCoords += mesh.GetTexCoord0(i2) * v;

    return true;
  }

  return false;
}

void ezRmlUiCanvas3DComponent::SetRmlResource(const ezRmlUiResourceHandle& hResource)
{
  if (m_hResource != hResource)
  {
    m_hResource = hResource;

    if (m_pContext != nullptr)
    {
      if (m_pContext->LoadDocumentFromResource(m_hResource).Succeeded() && IsActive())
      {
        m_pContext->ShowDocument();
      }

      UpdateCachedValues();
    }
  }
}

void ezRmlUiCanvas3DComponent::SetTextureSize(const ezVec2U32& vSize)
{
  if (m_vTextureSize != vSize)
  {
    m_vTextureSize.x = ezMath::Min(vSize.x, 4096u);
    m_vTextureSize.y = ezMath::Min(vSize.y, 4096u);

    if (m_pContext != nullptr)
    {
      m_pContext->SetSize(m_vTextureSize);
    }
  }
}

void ezRmlUiCanvas3DComponent::SetAutobindBlackboards(bool bAutobind)
{
  if (m_bAutobindBlackboards != bAutobind)
  {
    m_bAutobindBlackboards = bAutobind;

    UpdateAutobinding();
  }
}

void ezRmlUiCanvas3DComponent::SetOnDemandUpdate(bool bOnDemandUpdate)
{
  m_bOnDemandUpdate = bOnDemandUpdate;
}

void ezRmlUiCanvas3DComponent::SetClearStaleInput(bool bClearStaleInput)
{
  m_bClearStaleInput = bClearStaleInput;
}

void ezRmlUiCanvas3DComponent::SetInteractive(bool bIsInteractive)
{
  m_bIsInteractive = bIsInteractive;
}

ezUInt32 ezRmlUiCanvas3DComponent::AddDataBinding(ezUniquePtr<ezRmlUiDataBinding>&& pDataBinding)
{
  // Document needs to be loaded again since data bindings have to be set before document load
  if (m_pContext != nullptr)
  {
    if (pDataBinding->Initialize(*m_pContext).Succeeded())
    {
      if (m_pContext->LoadDocumentFromResource(m_hResource).Succeeded() && IsActive())
      {
        m_pContext->ShowDocument();
      }
    }
  }

  for (ezUInt32 i = 0; i < m_DataBindings.GetCount(); ++i)
  {
    if (pDataBinding == nullptr)
    {
      m_DataBindings[i] = std::move(pDataBinding);
      return i;
    }
  }

  ezUInt32 uiDataBindingIndex = m_DataBindings.GetCount();
  m_DataBindings.PushBack(std::move(pDataBinding));
  return uiDataBindingIndex;
}

void ezRmlUiCanvas3DComponent::RemoveDataBinding(ezUInt32 uiDataBindingIndex)
{
  auto& pDataBinding = m_DataBindings[uiDataBindingIndex];

  if (m_pContext != nullptr)
  {
    pDataBinding->Deinitialize(*m_pContext);
  }

  m_DataBindings[uiDataBindingIndex] = nullptr;
}

ezUInt32 ezRmlUiCanvas3DComponent::AddBlackboardBinding(const ezSharedPtr<ezBlackboard>& pBlackboard)
{
  auto pDataBinding = EZ_DEFAULT_NEW(ezRmlUiInternal::BlackboardDataBinding, pBlackboard);
  return AddDataBinding(pDataBinding);
}

void ezRmlUiCanvas3DComponent::RemoveBlackboardBinding(ezUInt32 uiDataBindingIndex)
{
  RemoveDataBinding(uiDataBindingIndex);
}

ezRmlUiContext* ezRmlUiCanvas3DComponent::GetOrCreateRmlContext()
{
  if (m_pContext != nullptr)
  {
    return m_pContext;
  }

  ezStringBuilder sName = "RmlUi_";
  if (m_hResource.IsValid())
  {
    ezStringView sResourceID = m_hResource.GetResourceIdOrDescription();
    sName.Append(sResourceID.GetFileName());
  }
  sName.AppendFormat("_{}", ezArgP(this));

  m_pContext = ezRmlUi::GetSingleton()->CreateContext(sName, m_vTextureSize);

  for (auto& pDataBinding : m_DataBindings)
  {
    pDataBinding->Initialize(*m_pContext).IgnoreResult();
  }

  m_pContext->LoadDocumentFromResource(m_hResource).IgnoreResult();

  UpdateCachedValues();

  return m_pContext;
}

void ezRmlUiCanvas3DComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  ezStreamWriter& s = inout_stream.GetStream();

  s << m_hResource;
  s << m_vTextureSize;
  s << m_bAutobindBlackboards;
  s << m_bOnDemandUpdate;
  s << m_bClearStaleInput;
  s << m_bIsInteractive;
}

void ezRmlUiCanvas3DComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = inout_stream.GetStream();

  ezVec2I32 vOffset;
  ezVec2 vAnchorPoint;

  s >> m_hResource;
  s >> m_vTextureSize;
  s >> m_bAutobindBlackboards;
  s >> m_bOnDemandUpdate;
  if (uiVersion >= 2)
    s >> m_bClearStaleInput;
  if (uiVersion >= 3)
    s >> m_bIsInteractive;
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

void ezRmlUiCanvas3DComponent::OnMsgReload(ezMsgRmlUiReload& msg)
{
  if (m_pContext != nullptr)
  {
    m_pContext->ReloadDocumentFromResource(m_hResource).IgnoreResult();
    m_pContext->ShowDocument();

    UpdateCachedValues();
  }
}

bool ezRmlUiCanvas3DComponent::UpdateTexture()
{
  if (m_vTextureSize.x == 0 || m_vTextureSize.y == 0)
    return false;

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  const ezGALTexture* pTexture = pDevice->GetTexture(m_hTexture);
  if (pTexture == nullptr || pTexture->GetDescription().m_uiWidth != m_vTextureSize.x || pTexture->GetDescription().m_uiHeight != m_vTextureSize.y)
  {
    if (pTexture != nullptr)
    {
      pDevice->DestroyTexture(m_hTexture);
    }

    ezGALTextureCreationDescription desc;
    desc.m_uiWidth = m_vTextureSize.x;
    desc.m_uiHeight = m_vTextureSize.y;
    desc.m_Format = ezGALResourceFormat::RGBAUByteNormalized;
    desc.m_ResourceAccess.m_bImmutable = false;
    if (ezMath::IsPowerOf2(m_vTextureSize.x) && ezMath::IsPowerOf2(m_vTextureSize.y))
    {
      desc.m_uiMipLevelCount = ezMath::Max(ezMath::Log2i(m_vTextureSize.x), ezMath::Log2i(m_vTextureSize.y)) - 2;
      desc.m_bAllowDynamicMipGeneration = true;
    }

    m_hTexture = pDevice->CreateTexture(desc);

    m_pContext->SetSize(m_vTextureSize);
    m_pContext->SetDpiScale(1.0f);

    return true;
  }

  return false;
}

void ezRmlUiCanvas3DComponent::UpdateCachedValues()
{
  m_ResourceEventUnsubscriber.Unsubscribe();

  if (m_hResource.IsValid())
  {
    ezResourceLock pResource(m_hResource, ezResourceAcquireMode::PointerOnly);

    pResource->m_ResourceEvents.AddEventHandler(
      [hComponent = GetHandle(), pWorld = GetWorld()](const ezResourceEvent& e)
      {
        if (e.m_Type == ezResourceEvent::Type::ResourceContentUnloading)
        {
          pWorld->PostMessage(hComponent, ezMsgRmlUiReload(), ezTime::MakeZero());
        }
      },
      m_ResourceEventUnsubscriber);
  }
}

void ezRmlUiCanvas3DComponent::UpdateAutobinding()
{
  for (ezUInt32 uiIndex : m_AutoBindings)
  {
    RemoveDataBinding(uiIndex);
  }

  m_AutoBindings.Clear();

  if (m_bAutobindBlackboards)
  {
    ezHybridArray<ezBlackboardComponent*, 4> blackboardComponents;

    ezGameObject* pObject = GetOwner();
    while (pObject != nullptr)
    {
      pObject->TryGetComponentsOfBaseType(blackboardComponents);

      for (auto pBlackboardComponent : blackboardComponents)
      {
        pBlackboardComponent->EnsureInitialized();

        m_AutoBindings.PushBack(AddBlackboardBinding(pBlackboardComponent->GetBoard()));
      }

      pObject = pObject->GetParent();
    }
  }
}


EZ_STATICLINK_FILE(RmlUiPlugin, RmlUiPlugin_Components_Implementation_RmlUiCanvas3DComponent);
