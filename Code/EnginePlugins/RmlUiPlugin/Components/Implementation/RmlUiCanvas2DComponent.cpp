#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <RmlUiPlugin/Components/RmlUiCanvas2DComponent.h>
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
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezRmlUiCanvas2DComponent, 3, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_RESOURCE_ACCESSOR_PROPERTY("RmlFile", GetRmlResource, SetRmlResource)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Rml_UI")),
    EZ_ACCESSOR_PROPERTY("AnchorPoint", GetAnchorPoint, SetAnchorPoint)->AddAttributes(new ezClampValueAttribute(ezVec2(0), ezVec2(1))),
    EZ_ACCESSOR_PROPERTY("Size", GetSize, SetSize)->AddAttributes(new ezSuffixAttribute("px"), new ezMinValueTextAttribute("Auto")),
    EZ_ACCESSOR_PROPERTY("Offset", GetOffset, SetOffset)->AddAttributes(new ezDefaultValueAttribute(ezVec2::MakeZero()), new ezSuffixAttribute("px")),    
    EZ_ACCESSOR_PROPERTY("PassInput", GetPassInput, SetPassInput)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_ACCESSOR_PROPERTY("AutobindBlackboards", GetAutobindBlackboards, SetAutobindBlackboards)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_ACCESSOR_PROPERTY("OnDemandUpdate", GetOnDemandUpdate, SetOnDemandUpdate)->AddAttributes(new ezDefaultValueAttribute(true)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
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

ezRmlUiCanvas2DComponent::ezRmlUiCanvas2DComponent() = default;
ezRmlUiCanvas2DComponent::~ezRmlUiCanvas2DComponent() = default;
ezRmlUiCanvas2DComponent& ezRmlUiCanvas2DComponent::operator=(ezRmlUiCanvas2DComponent&& rhs) = default;

void ezRmlUiCanvas2DComponent::Initialize()
{
  SUPER::Initialize();

  UpdateAutobinding();
}

void ezRmlUiCanvas2DComponent::Deinitialize()
{
  SUPER::Deinitialize();

  if (m_hTexture.IsInvalidated() == false)
  {
    ezGALDevice::GetDefaultDevice()->DestroyTexture(m_hTexture);
    m_hTexture.Invalidate();
  }

  if (m_pContext != nullptr)
  {
    ezRmlUi::GetSingleton()->DeleteContext(m_pContext);
    m_pContext = nullptr;
  }

  m_DataBindings.Clear();
}

void ezRmlUiCanvas2DComponent::OnActivated()
{
  SUPER::OnActivated();

  GetOrCreateRmlContext()->ShowDocument();

  // Update once to ensure correct initial state
  Update();
}

void ezRmlUiCanvas2DComponent::OnDeactivated()
{
  m_pContext->HideDocument();

  SUPER::OnDeactivated();
}

void ezRmlUiCanvas2DComponent::Update()
{
  if (m_pContext == nullptr)
    return;

  ezTime tDiff = GetWorld()->GetClock().GetTimeDiff();
  bool bNeedsUpdate = m_pContext->GetNextUpdateDelay() < ezMath::Max(tDiff.GetSeconds(), 1.0 / 240.0);

  ezVec2 viewSize = ezVec2::MakeZero();
  bNeedsUpdate |= UpdateSizeOffsetAndTexture(viewSize);

  if (m_bPassInput && GetWorld()->GetWorldSimulationEnabled())
  {
    ezVec2 mousePos;
    ezInputManager::GetInputSlotState(ezInputSlot_MousePositionX, &mousePos.x);
    ezInputManager::GetInputSlotState(ezInputSlot_MousePositionY, &mousePos.y);

    mousePos = mousePos.CompMul(viewSize) - m_vFinalOffset;
    bNeedsUpdate |= m_pContext->UpdateInput(mousePos);
  }

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

void ezRmlUiCanvas2DComponent::SetRmlResource(const ezRmlUiResourceHandle& hResource)
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

void ezRmlUiCanvas2DComponent::SetOffset(const ezVec2I32& vOffset)
{
  m_vOffset = vOffset;
}

void ezRmlUiCanvas2DComponent::SetSize(const ezVec2U32& vSize)
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

void ezRmlUiCanvas2DComponent::SetAnchorPoint(const ezVec2& vAnchorPoint)
{
  m_vAnchorPoint = vAnchorPoint;
}

void ezRmlUiCanvas2DComponent::SetPassInput(bool bPassInput)
{
  m_bPassInput = bPassInput;
}

void ezRmlUiCanvas2DComponent::SetAutobindBlackboards(bool bAutobind)
{
  if (m_bAutobindBlackboards != bAutobind)
  {
    m_bAutobindBlackboards = bAutobind;

    UpdateAutobinding();
  }
}

void ezRmlUiCanvas2DComponent::SetOnDemandUpdate(bool bOnDemandUpdate)
{
  m_bOnDemandUpdate = bOnDemandUpdate;
}

ezUInt32 ezRmlUiCanvas2DComponent::AddDataBinding(ezUniquePtr<ezRmlUiDataBinding>&& pDataBinding)
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

void ezRmlUiCanvas2DComponent::RemoveDataBinding(ezUInt32 uiDataBindingIndex)
{
  auto& pDataBinding = m_DataBindings[uiDataBindingIndex];

  if (m_pContext != nullptr)
  {
    pDataBinding->Deinitialize(*m_pContext);
  }

  m_DataBindings[uiDataBindingIndex] = nullptr;
}

ezUInt32 ezRmlUiCanvas2DComponent::AddBlackboardBinding(const ezSharedPtr<ezBlackboard>& pBlackboard)
{
  auto pDataBinding = EZ_DEFAULT_NEW(ezRmlUiInternal::BlackboardDataBinding, pBlackboard);
  return AddDataBinding(pDataBinding);
}

void ezRmlUiCanvas2DComponent::RemoveBlackboardBinding(ezUInt32 uiDataBindingIndex)
{
  RemoveDataBinding(uiDataBindingIndex);
}

ezRmlUiContext* ezRmlUiCanvas2DComponent::GetOrCreateRmlContext()
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

void ezRmlUiCanvas2DComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  ezStreamWriter& s = inout_stream.GetStream();

  s << m_hResource;
  s << m_vOffset;
  s << m_vSize;
  s << m_vAnchorPoint;
  s << m_bPassInput;
  s << m_bAutobindBlackboards;
  s << m_bOnDemandUpdate;
}

void ezRmlUiCanvas2DComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = inout_stream.GetStream();

  s >> m_hResource;
  s >> m_vOffset;
  s >> m_vSize;
  s >> m_vAnchorPoint;
  s >> m_bPassInput;

  if (uiVersion >= 2)
  {
    s >> m_bAutobindBlackboards;
  }

  if (uiVersion >= 3)
  {
    s >> m_bOnDemandUpdate;
  }
}

ezResult ezRmlUiCanvas2DComponent::GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg)
{
  ref_bAlwaysVisible = true;
  return EZ_SUCCESS;
}

void ezRmlUiCanvas2DComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  if (msg.m_pView->GetCameraUsageHint() != ezCameraUsageHint::MainView && msg.m_pView->GetCameraUsageHint() != ezCameraUsageHint::EditorView && msg.m_pView->GetCameraUsageHint() != ezCameraUsageHint::Thumbnail)
    return;

  // Don't extract render data for selection.
  if (msg.m_OverrideCategory != ezInvalidRenderDataCategory)
    return;

  if (m_pContext != nullptr && m_hTexture.IsInvalidated() == false)
  {
    ezRmlUi::GetSingleton()->ExtractContext(*m_pContext, m_hTexture);

    auto pRenderData = ezCreateRenderDataForThisFrame<ezRmlUiRenderData>(GetOwner());
    pRenderData->m_hTexture = m_hTexture;
    pRenderData->m_vOffset = m_vFinalOffset;

    msg.AddRenderData(pRenderData, ezDefaultRenderDataCategories::GUI, ezRenderData::Caching::Never);
  }
}

void ezRmlUiCanvas2DComponent::OnMsgReload(ezMsgRmlUiReload& msg)
{
  if (m_pContext != nullptr)
  {
    m_pContext->ReloadDocumentFromResource(m_hResource).IgnoreResult();
    m_pContext->ShowDocument();

    UpdateCachedValues();
  }
}

bool ezRmlUiCanvas2DComponent::UpdateSizeOffsetAndTexture(ezVec2& out_viewSize)
{
  out_viewSize = ezVec2(1.0f);
  if (ezView* pView = ezRenderWorld::GetViewByUsageHint(ezCameraUsageHint::MainView, ezCameraUsageHint::EditorView, GetWorld()))
  {
    out_viewSize.x = pView->GetViewport().width;
    out_viewSize.y = pView->GetViewport().height;
  }

  float fScale = 1.0f;
  if (m_vReferenceResolution.x > 0 && m_vReferenceResolution.y > 0)
  {
    fScale = out_viewSize.y / m_vReferenceResolution.y;
  }

  ezVec2 size = ezVec2(static_cast<float>(m_vSize.x), static_cast<float>(m_vSize.y)) * fScale;
  if (size.x <= 0.0f)
  {
    size.x = out_viewSize.x;
  }
  if (size.y <= 0.0f)
  {
    size.y = out_viewSize.y;
  }
  const ezVec2U32 sizeU32 = ezVec2U32(static_cast<ezUInt32>(size.x), static_cast<ezUInt32>(size.y));
  if (sizeU32.IsZero())
    return false;

  m_pContext->SetSize(sizeU32);
  m_pContext->SetDpiScale(fScale);

  ezVec2 offset = ezVec2(static_cast<float>(m_vOffset.x), static_cast<float>(m_vOffset.y)) * fScale;
  offset = (out_viewSize - size).CompMul(m_vAnchorPoint) - offset.CompMul(m_vAnchorPoint * 2.0f - ezVec2(1.0f));
  m_vFinalOffset.x = ezMath::Round(offset.x);
  m_vFinalOffset.y = ezMath::Round(offset.y);

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

    m_hTexture = pDevice->CreateTexture(desc);

    return true;
  }

  return false;
}

void ezRmlUiCanvas2DComponent::UpdateCachedValues()
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

void ezRmlUiCanvas2DComponent::UpdateAutobinding()
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


EZ_STATICLINK_FILE(RmlUiPlugin, RmlUiPlugin_Components_Implementation_RmlUiCanvas2DComponent);
