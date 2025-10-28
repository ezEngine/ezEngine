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
EZ_BEGIN_COMPONENT_TYPE(ezRmlUiCanvas3DComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_RESOURCE_ACCESSOR_PROPERTY("Mesh", GetMesh, SetMesh)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Mesh_Static")),
    EZ_RESOURCE_ACCESSOR_PROPERTY("RmlFile", GetRmlResource, SetRmlResource)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Rml_UI")),
    EZ_ACCESSOR_PROPERTY("AnchorPoint", GetAnchorPoint, SetAnchorPoint)->AddAttributes(new ezClampValueAttribute(ezVec2(0), ezVec2(1))),
    EZ_ACCESSOR_PROPERTY("Size", GetSize, SetSize)->AddAttributes(new ezSuffixAttribute("px"), new ezMinValueTextAttribute("Auto")),
    EZ_ACCESSOR_PROPERTY("Offset", GetOffset, SetOffset)->AddAttributes(new ezDefaultValueAttribute(ezVec2::MakeZero()), new ezSuffixAttribute("px")),    
    EZ_ACCESSOR_PROPERTY("AutobindBlackboards", GetAutobindBlackboards, SetAutobindBlackboards)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_ACCESSOR_PROPERTY("OnDemandUpdate", GetOnDemandUpdate, SetOnDemandUpdate)->AddAttributes(new ezDefaultValueAttribute(true)),
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
  bool bNeedsUpdate = m_bDirty;
  m_bDirty = false;

  if (m_pContext == nullptr)
    return;

  const ezTime tDiff = ezClock::GetGlobalClock()->GetTimeDiff();
  bNeedsUpdate |= m_pContext->GetNextUpdateDelay() < ezMath::Max(tDiff.GetSeconds(), 1.0 / 240.0);

  ezVec2 viewSize = ezVec2::MakeZero();
  bNeedsUpdate |= UpdateSizeOffsetAndTexture(viewSize);

  for (auto& pDataBinding : m_DataBindings)
  {
    if (pDataBinding != nullptr)
    {
      bNeedsUpdate |= pDataBinding->Update();
    }
  }

  if (bNeedsUpdate || m_bOnDemandUpdate == false)
  {
    m_pContext->Update();
  }
}

void ezRmlUiCanvas3DComponent::ApplyInput(const ezRmlUiInputState& input)
{
  if (input == m_LastInput)
  {
    return;
  }

  m_bDirty |= m_pContext->UpdateInput(m_LastInput, input);
  m_LastInput = input;
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

void ezRmlUiCanvas3DComponent::SetOffset(const ezVec2I32& vOffset)
{
  m_vOffset = vOffset;
}

void ezRmlUiCanvas3DComponent::SetSize(const ezVec2U32& vSize)
{
  if (m_vSize != vSize)
  {
    m_vSize = vSize;

    if (m_pContext != nullptr)
    {
      m_pContext->SetSize(m_vSize);
    }
  }
}

void ezRmlUiCanvas3DComponent::SetAnchorPoint(const ezVec2& vAnchorPoint)
{
  m_vAnchorPoint = vAnchorPoint;
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

  m_pContext = ezRmlUi::GetSingleton()->CreateContext(sName, m_vSize);

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
  s << m_vOffset;
  s << m_vSize;
  s << m_vAnchorPoint;
  s << m_bAutobindBlackboards;
  s << m_bOnDemandUpdate;
}

void ezRmlUiCanvas3DComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = inout_stream.GetStream();

  s >> m_hResource;
  s >> m_vOffset;
  s >> m_vSize;
  s >> m_vAnchorPoint;
  s >> m_bAutobindBlackboards;
  s >> m_bOnDemandUpdate;
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
    // Don't extract render data for selection.
    //if (msg.m_OverrideCategory != ezInvalidRenderDataCategory)
    //  return;

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

    ezRenderData::Category category = ezDefaultRenderDataCategories::SimpleOpaque;

    msg.AddRenderData(pRenderData, category, ezRenderData::Caching::Never);
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

bool ezRmlUiCanvas3DComponent::UpdateSizeOffsetAndTexture(ezVec2& out_viewSize)
{
  const ezVec2U32 sizeU32 = ezVec2U32(static_cast<ezUInt32>(m_vSize.x), static_cast<ezUInt32>(m_vSize.y));
  if (sizeU32.x == 0 || sizeU32.y == 0)
    return false;

  m_pContext->SetSize(sizeU32);
  m_pContext->SetDpiScale(1.0f);

  // Recreate texture if necessary
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  const ezGALTexture* pTexture = pDevice->GetTexture(m_hTexture);
  if (pTexture == nullptr || pTexture->GetDescription().m_uiWidth != sizeU32.x || pTexture->GetDescription().m_uiHeight != sizeU32.y)
  {
    if (pTexture != nullptr)
    {
      pDevice->DestroyTexture(m_hTexture);
    }

    ezGALTextureCreationDescription desc;
    desc.m_uiWidth = sizeU32.x;
    desc.m_uiHeight = sizeU32.y;
    desc.m_Format = ezGALResourceFormat::RGBAUByteNormalized;
    desc.m_ResourceAccess.m_bImmutable = false;
    if (ezMath::IsPowerOf2(sizeU32.x) && ezMath::IsPowerOf2(sizeU32.y))
    {
      desc.m_uiMipLevelCount = ezMath::Max(ezMath::Log2i(sizeU32.x), ezMath::Log2i(sizeU32.y)) - 2;
      desc.m_bAllowDynamicMipGeneration = true;
    }

    m_hTexture = pDevice->CreateTexture(desc);

    return true;
  }

  return false;
}

void ezRmlUiCanvas3DComponent::UpdateCachedValues()
{
  m_ResourceEventUnsubscriber.Unsubscribe();
  m_vReferenceResolution.SetZero();

  if (m_hResource.IsValid())
  {
    {
      ezResourceLock pResource(m_hResource, ezResourceAcquireMode::BlockTillLoaded);

      if (pResource->GetScaleMode() == ezRmlUiScaleMode::WithScreenSize)
      {
        m_vReferenceResolution = pResource->GetReferenceResolution();
      }
    }

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
