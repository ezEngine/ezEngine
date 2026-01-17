#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Gameplay/BlackboardComponent.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RmlUiPlugin/Components/RmlUiCanvasComponentBase.h>
#include <RmlUiPlugin/Implementation/BlackboardDataBinding.h>
#include <RmlUiPlugin/RmlUiContext.h>
#include <RmlUiPlugin/RmlUiSingleton.h>

// clang-format off
EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezRmlUiCanvasComponentBase, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_RESOURCE_ACCESSOR_PROPERTY("RmlFile", GetRmlResource, SetRmlResource)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Rml_UI")),
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

static ezAtomicInteger32 s_RmlContextIdCounter;

ezRmlUiCanvasComponentBase::ezRmlUiCanvasComponentBase() = default;
ezRmlUiCanvasComponentBase::~ezRmlUiCanvasComponentBase() = default;
ezRmlUiCanvasComponentBase& ezRmlUiCanvasComponentBase::operator=(ezRmlUiCanvasComponentBase&& rhs) = default;

void ezRmlUiCanvasComponentBase::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  ezStreamWriter& s = inout_stream.GetStream();

  s << m_hResource;
  s << m_bAutobindBlackboards;
  s << m_bOnDemandUpdate;
}

void ezRmlUiCanvasComponentBase::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = inout_stream.GetStream();

  s >> m_hResource;
  s >> m_bAutobindBlackboards;
  s >> m_bOnDemandUpdate;
}

void ezRmlUiCanvasComponentBase::Initialize()
{
  SUPER::Initialize();

  UpdateAutobinding();
}

void ezRmlUiCanvasComponentBase::Deinitialize()
{
  SUPER::Deinitialize();

  if (m_pContext != nullptr)
  {
    ezRmlUi::GetSingleton()->DeleteContext(m_pContext);
    m_pContext = nullptr;
  }

  m_DataBindings.Clear();
}

void ezRmlUiCanvasComponentBase::OnDeactivated()
{
  m_pContext->HideDocument();

  SUPER::OnDeactivated();
}

void ezRmlUiCanvasComponentBase::Update()
{
  if (m_pContext == nullptr)
    return;

  const ezTime tDiff = ezClock::GetGlobalClock()->GetTimeDiff();
  m_bNeedsUpdate |= m_pContext->GetNextUpdateDelay() < ezMath::Max(tDiff.GetSeconds(), 1.0 / 240.0);

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

    m_bNeedsUpdate = false;
  }
}

bool ezRmlUiCanvasComponentBase::ReceiveInput(const ezVec2& vMousePosInsideCanvas, ezRmlUiInputSnapshot input)
{
  if (m_pContext == nullptr)
    return false;

  m_InputProvider.Update(input);
  m_bNeedsUpdate |= m_pContext->UpdateInput(vMousePosInsideCanvas, m_InputProvider);

  return true;
}

void ezRmlUiCanvasComponentBase::SetRmlResource(const ezRmlUiResourceHandle& hResource)
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

void ezRmlUiCanvasComponentBase::SetAutobindBlackboards(bool bAutobind)
{
  if (m_bAutobindBlackboards != bAutobind)
  {
    m_bAutobindBlackboards = bAutobind;

    UpdateAutobinding();
  }
}

void ezRmlUiCanvasComponentBase::SetOnDemandUpdate(bool bOnDemandUpdate)
{
  m_bOnDemandUpdate = bOnDemandUpdate;
  m_bNeedsUpdate = true;
}

ezUInt32 ezRmlUiCanvasComponentBase::AddDataBinding(ezUniquePtr<ezRmlUiDataBinding>&& pDataBinding)
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

void ezRmlUiCanvasComponentBase::RemoveDataBinding(ezUInt32 uiDataBindingIndex)
{
  auto& pDataBinding = m_DataBindings[uiDataBindingIndex];

  if (m_pContext != nullptr)
  {
    pDataBinding->Deinitialize(*m_pContext);
  }

  m_DataBindings[uiDataBindingIndex] = nullptr;
}

ezUInt32 ezRmlUiCanvasComponentBase::AddBlackboardBinding(const ezSharedPtr<ezBlackboard>& pBlackboard)
{
  auto pDataBinding = EZ_DEFAULT_NEW(ezRmlUiInternal::BlackboardDataBinding, pBlackboard);
  return AddDataBinding(pDataBinding);
}

void ezRmlUiCanvasComponentBase::RemoveBlackboardBinding(ezUInt32 uiDataBindingIndex)
{
  RemoveDataBinding(uiDataBindingIndex);
}

ezResult ezRmlUiCanvasComponentBase::GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg)
{
  ref_bAlwaysVisible = true;
  return EZ_SUCCESS;
}

ezRmlUiContext* ezRmlUiCanvasComponentBase::GetOrCreateRmlContext()
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

  if (m_uiContextID == 0)
  {
    m_uiContextID = s_RmlContextIdCounter.Increment();
  }

  sName.AppendFormat("_{}", m_uiContextID);

  m_pContext = ezRmlUi::GetSingleton()->CreateContext(sName, m_vSize);
  EZ_ASSERT_DEV(m_pContext != nullptr, "RML UI context creation failed");

  for (auto& pDataBinding : m_DataBindings)
  {
    pDataBinding->Initialize(*m_pContext).IgnoreResult();
  }

  m_pContext->LoadDocumentFromResource(m_hResource).IgnoreResult();

  UpdateCachedValues();

  return m_pContext;
}

void ezRmlUiCanvasComponentBase::OnMsgReload(ezMsgRmlUiReload& msg)
{
  if (m_pContext != nullptr)
  {
    m_pContext->ReloadDocumentFromResource(m_hResource).IgnoreResult();
    m_pContext->ShowDocument();

    UpdateCachedValues();
  }
}

void ezRmlUiCanvasComponentBase::UpdateCachedValues()
{
  m_ResourceEventUnsubscriber.Unsubscribe();
  m_vReferenceResolution.SetZero();

  if (m_hResource.IsValid())
  {
    ezResourceLock pResource(m_hResource, ezResourceAcquireMode::BlockTillLoaded);

    if (pResource->GetScaleMode() == ezRmlUiScaleMode::WithScreenSize)
    {
      m_vReferenceResolution = pResource->GetReferenceResolution();
    }

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

void ezRmlUiCanvasComponentBase::UpdateAutobinding()
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


EZ_STATICLINK_FILE(RmlUiPlugin, RmlUiPlugin_Components_Implementation_RmlUiCanvasComponentBase);
