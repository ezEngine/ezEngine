#include <RmlUiPluginPCH.h>

#include <Core/Input/InputManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RmlUiPlugin/Components/RmlUiCanvas2DComponent.h>
#include <RmlUiPlugin/Implementation/BlackboardDataBinding.h>
#include <RmlUiPlugin/RmlUiContext.h>
#include <RmlUiPlugin/RmlUiSingleton.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezRmlUiCanvas2DComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("RmlFile", GetRmlFile, SetRmlFile)->AddAttributes(new ezAssetBrowserAttribute("RmlUi")),
    EZ_ACCESSOR_PROPERTY("AnchorPoint", GetAnchorPoint, SetAnchorPoint)->AddAttributes(new ezClampValueAttribute(ezVec2(0), ezVec2(1))),
    EZ_ACCESSOR_PROPERTY("Size", GetSize, SetSize)->AddAttributes(new ezSuffixAttribute("px"), new ezMinValueTextAttribute("Auto")),
    EZ_ACCESSOR_PROPERTY("Offset", GetOffset, SetOffset)->AddAttributes(new ezDefaultValueAttribute(ezVec2::ZeroVector()), new ezSuffixAttribute("px")),    
    EZ_ACCESSOR_PROPERTY("PassInput", GetPassInput, SetPassInput)->AddAttributes(new ezDefaultValueAttribute(true)),
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
    new ezCategoryAttribute("RmlUi"),
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

  ezStringBuilder sName = "Context_";
  if (m_hResource.IsValid())
  {
    sName.Append(m_hResource.GetResourceID().GetView());
  }
  sName.AppendFormat("_{}", ezArgP(this));

  m_pContext = ezRmlUi::GetSingleton()->CreateContext(sName, m_Size);

  for (auto& pDataBinding : m_DataBindings)
  {
    pDataBinding->Setup(*m_pContext).IgnoreResult();
  }

  m_pContext->LoadDocumentFromResource(m_hResource).IgnoreResult();

  UpdateCachedValues();
}

void ezRmlUiCanvas2DComponent::Deinitialize()
{
  SUPER::Deinitialize();

  ezRmlUi::GetSingleton()->DeleteContext(m_pContext);
  m_pContext = nullptr;

  m_DataBindings.Clear();
}

void ezRmlUiCanvas2DComponent::OnActivated()
{
  SUPER::OnActivated();

  m_pContext->ShowDocument();
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

  ezVec2 viewSize = ezVec2(1.0f);
  if (ezView* pView = ezRenderWorld::GetViewByUsageHint(ezCameraUsageHint::MainView, ezCameraUsageHint::EditorView, GetWorld()))
  {
    viewSize.x = pView->GetViewport().width;
    viewSize.y = pView->GetViewport().height;
  }

  float fScale = 1.0f;
  if (m_ReferenceResolution.x > 0 && m_ReferenceResolution.y > 0)
  {
    fScale = viewSize.y / m_ReferenceResolution.y;
  }

  ezVec2 size = ezVec2(static_cast<float>(m_Size.x), static_cast<float>(m_Size.y)) * fScale;
  if (size.x <= 0.0f)
  {
    size.x = viewSize.x;
  }
  if (size.y <= 0.0f)
  {
    size.y = viewSize.y;
  }
  m_pContext->SetSize(ezVec2U32(static_cast<ezUInt32>(size.x), static_cast<ezUInt32>(size.y)));

  ezVec2 offset = ezVec2(static_cast<float>(m_Offset.x), static_cast<float>(m_Offset.y)) * fScale;
  offset = (viewSize - size).CompMul(m_AnchorPoint) - offset.CompMul(m_AnchorPoint * 2.0f - ezVec2(1.0f));
  m_pContext->SetOffset(ezVec2I32(static_cast<int>(offset.x), static_cast<int>(offset.y)));

  m_pContext->SetDpiScale(fScale);

  if (m_bPassInput)
  {
    ezVec2 mousePos;
    ezInputManager::GetInputSlotState(ezInputSlot_MousePositionX, &mousePos.x);
    ezInputManager::GetInputSlotState(ezInputSlot_MousePositionY, &mousePos.y);

    mousePos = mousePos.CompMul(viewSize) - offset;
    m_pContext->UpdateInput(mousePos);
  }

  for (auto& pDataBinding : m_DataBindings)
  {
    pDataBinding->Update();
  }

  m_pContext->Update();
}

void ezRmlUiCanvas2DComponent::SetRmlFile(const char* szFile)
{
  ezRmlUiResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezRmlUiResource>(szFile);
    ezResourceManager::PreloadResource(hResource);
  }

  SetRmlResource(hResource);
}

const char* ezRmlUiCanvas2DComponent::GetRmlFile() const
{
  if (!m_hResource.IsValid())
    return "";

  return m_hResource.GetResourceID();
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

void ezRmlUiCanvas2DComponent::SetOffset(const ezVec2I32& offset)
{
  m_Offset = offset;
}

void ezRmlUiCanvas2DComponent::SetSize(const ezVec2U32& size)
{
  if (m_Size != size)
  {
    m_Size = size;

    if (m_pContext != nullptr)
    {
      m_pContext->SetSize(m_Size);
    }
  }
}

void ezRmlUiCanvas2DComponent::SetAnchorPoint(const ezVec2& anchorPoint)
{
  m_AnchorPoint = anchorPoint;
}

void ezRmlUiCanvas2DComponent::SetPassInput(bool bPassInput)
{
  m_bPassInput = bPassInput;
}

ezUInt32 ezRmlUiCanvas2DComponent::AddDataBinding(ezUniquePtr<ezRmlUiDataBinding>&& dataBinding)
{
  // Document needs to be loaded again since data bindings have to be set before document load
  if (m_pContext != nullptr)
  {
    if (dataBinding->Setup(*m_pContext).Succeeded())
    {
      if (m_pContext->LoadDocumentFromResource(m_hResource).Succeeded() && IsActive())
      {
        m_pContext->ShowDocument();
      }
    }
  }

  ezUInt32 uiDataBindingIndex = m_DataBindings.GetCount();
  m_DataBindings.PushBack(std::move(dataBinding));
  return uiDataBindingIndex;
}

void ezRmlUiCanvas2DComponent::RemoveDataBinding(ezUInt32 uiDataBindingIndex)
{
  // Can't remove data bindings atm
  EZ_ASSERT_NOT_IMPLEMENTED;
}

ezUInt32 ezRmlUiCanvas2DComponent::AddBlackboardBinding(ezBlackboard& blackboard, const char* szModelName)
{
  auto pDataBinding = EZ_DEFAULT_NEW(ezRmlUiInternal::BlackboardDataBinding, blackboard, szModelName);
  return AddDataBinding(pDataBinding);
}

void ezRmlUiCanvas2DComponent::RemoveBlackboardBinding(ezUInt32 uiDataBindingIndex)
{
  RemoveDataBinding(uiDataBindingIndex);
}

void ezRmlUiCanvas2DComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  ezStreamWriter& s = stream.GetStream();

  s << m_hResource;
  s << m_Offset;
  s << m_Size;
  s << m_AnchorPoint;
  s << m_bPassInput;
}

void ezRmlUiCanvas2DComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

  s >> m_hResource;
  s >> m_Offset;
  s >> m_Size;
  s >> m_AnchorPoint;
  s >> m_bPassInput;
}

ezResult ezRmlUiCanvas2DComponent::GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible)
{
  bAlwaysVisible = true;
  return EZ_SUCCESS;
}

void ezRmlUiCanvas2DComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  if (msg.m_pView->GetCameraUsageHint() != ezCameraUsageHint::MainView && msg.m_pView->GetCameraUsageHint() != ezCameraUsageHint::EditorView && msg.m_pView->GetCameraUsageHint() != ezCameraUsageHint::Thumbnail)
    return;

  // Don't extract render data for selection.
  if (msg.m_OverrideCategory != ezInvalidRenderDataCategory)
    return;

  if (m_pContext != nullptr)
  {
    ezRmlUi::GetSingleton()->ExtractContext(*m_pContext, msg);
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

void ezRmlUiCanvas2DComponent::UpdateCachedValues()
{
  m_ResourceEventUnsubscriber.Unsubscribe();
  m_ReferenceResolution.SetZero();

  if (m_hResource.IsValid())
  {
    {
      ezResourceLock pResource(m_hResource, ezResourceAcquireMode::BlockTillLoaded);

      if (pResource->GetScaleMode() == ezRmlUiScaleMode::WithScreenSize)
      {
        m_ReferenceResolution = pResource->GetReferenceResolution();
      }
    }

    {
      ezResourceLock pResource(m_hResource, ezResourceAcquireMode::PointerOnly);

      pResource->m_ResourceEvents.AddEventHandler(
        [hComponent = GetHandle(), pWorld = GetWorld()](const ezResourceEvent& e) {
          if (e.m_Type == ezResourceEvent::Type::ResourceContentUnloading)
          {
            pWorld->PostMessage(hComponent, ezMsgRmlUiReload(), ezTime::Zero());
          }
        },
        m_ResourceEventUnsubscriber);
    }
  }
}
