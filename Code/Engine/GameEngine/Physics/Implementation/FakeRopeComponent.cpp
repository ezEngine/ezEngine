#include <GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Physics/FakeRopeComponent.h>
#include <RendererCore/AnimationSystem/Declarations.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezFakeRopeComponent, 1, ezComponentMode::Static)
  {
    EZ_BEGIN_PROPERTIES
    {
      EZ_ACCESSOR_PROPERTY("AnchorA", DummyGetter, SetAnchorAReference)->AddAttributes(new ezGameObjectReferenceAttribute()),
      EZ_ACCESSOR_PROPERTY("AnchorB", DummyGetter, SetAnchorBReference)->AddAttributes(new ezGameObjectReferenceAttribute()),
      EZ_ACCESSOR_PROPERTY("AttachToA", GetAttachToA, SetAttachToA)->AddAttributes(new ezDefaultValueAttribute(true)),
      EZ_ACCESSOR_PROPERTY("AttachToB", GetAttachToB, SetAttachToB)->AddAttributes(new ezDefaultValueAttribute(true)),
      EZ_ACCESSOR_PROPERTY("Length", GetLength, SetLength)->AddAttributes(new ezDefaultValueAttribute(5.0f)),
      EZ_MEMBER_PROPERTY("Pieces", m_uiPieces)->AddAttributes(new ezDefaultValueAttribute(32), new ezClampValueAttribute(2, 200)),
      EZ_MEMBER_PROPERTY("Damping", m_fDamping)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, 1.0f)),
    }
    EZ_END_PROPERTIES;
    EZ_BEGIN_ATTRIBUTES
    {
      new ezCategoryAttribute("Effects"),
    }
    EZ_END_ATTRIBUTES;
  }
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezFakeRopeComponent::ezFakeRopeComponent() = default;
ezFakeRopeComponent::~ezFakeRopeComponent() = default;

void ezFakeRopeComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_uiPieces;
  s << m_fLength;
  s << m_fDamping;
  s << m_RopeSim.m_bFirstNodeIsFixed;
  s << m_RopeSim.m_bLastNodeIsFixed;

  stream.WriteGameObjectHandle(m_hAnchorA);
  stream.WriteGameObjectHandle(m_hAnchorB);
}

void ezFakeRopeComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_uiPieces;
  s >> m_fLength;
  s >> m_fDamping;
  s >> m_RopeSim.m_bFirstNodeIsFixed;
  s >> m_RopeSim.m_bLastNodeIsFixed;

  m_hAnchorA = stream.ReadGameObjectHandle();
  m_hAnchorB = stream.ReadGameObjectHandle();
}

void ezFakeRopeComponent::OnActivated()
{
  m_vPreviewRefPos.SetZero();
  m_RopeSim.m_Nodes.Clear();

  UpdatePreview();
}

void ezFakeRopeComponent::OnDeactivated()
{
  // tell the render components, that the rope is gone
  m_RopeSim.m_Nodes.Clear();
  SendCurrentPose();

  SUPER::OnDeactivated();
}

ezVec3 ezFakeRopeComponent::GetAnchorPosition(const ezGameObjectHandle& hTarget) const
{
  const ezGameObject* pObj;
  if (GetWorld()->TryGetObject(hTarget, pObj))
  {
    return pObj->GetGlobalPosition();
  }

  return GetOwner()->GetGlobalPosition();
}

void ezFakeRopeComponent::SetupSimulator()
{
  if (m_uiPieces < m_RopeSim.m_Nodes.GetCount())
  {
    m_RopeSim.m_Nodes.SetCount(m_uiPieces);
  }
  else if (m_uiPieces > m_RopeSim.m_Nodes.GetCount())
  {
    ezVec3 anchorA = GetAnchorPosition(m_hAnchorA);
    ezVec3 anchorB = GetAnchorPosition(m_hAnchorB);

    ezUInt32 uiOldNum = m_RopeSim.m_Nodes.GetCount();

    m_RopeSim.m_Nodes.SetCount(m_uiPieces);

    for (ezUInt32 i = uiOldNum; i < m_uiPieces; ++i)
    {
      m_RopeSim.m_Nodes[i].m_vPosition = ezMath::Lerp(anchorA, anchorB, (float)i / (m_uiPieces - 1));
      m_RopeSim.m_Nodes[i].m_vPreviousPosition = m_RopeSim.m_Nodes[i].m_vPosition;
    }
  }

  if (!m_RopeSim.m_Nodes.IsEmpty())
  {
    if (m_RopeSim.m_bFirstNodeIsFixed)
    {
      m_RopeSim.m_Nodes[0].m_vPosition = GetAnchorPosition(m_hAnchorA);
    }

    if (m_RopeSim.m_bLastNodeIsFixed)
    {
      m_RopeSim.m_Nodes.PeekBack().m_vPosition = GetAnchorPosition(m_hAnchorB);
    }
  }
}

void ezFakeRopeComponent::UpdatePoses(bool force)
{
  if (!IsActiveAndInitialized())
    return;

  m_RopeSim.m_fDampingFactor = ezMath::Lerp(1.0f, 0.97f, m_fDamping);
  m_RopeSim.m_fSegmentLength = m_fLength / m_uiPieces;
  m_RopeSim.m_vAcceleration.Set(0, 0, -10);

  SetupSimulator();

  if (force)
  {
    m_RopeSim.SimulateTillEquilibrium(0.003f, 100);
  }
  else
  {
    m_RopeSim.SimulateRope(GetWorld()->GetClock().GetTimeDiff());
  }

  SendCurrentPose();
}

void ezFakeRopeComponent::UpdatePreview()
{
  if (!IsActiveAndInitialized() || IsActiveAndSimulating())
    return;

  ezVec3 vNewPreviewRefPos = GetOwner()->GetGlobalPosition();

  ezGameObject* pObj;
  if (GetWorld()->TryGetObject(m_hAnchorA, pObj))
    vNewPreviewRefPos += pObj->GetGlobalPosition();
  if (GetWorld()->TryGetObject(m_hAnchorB, pObj))
    vNewPreviewRefPos += pObj->GetGlobalPosition();

  if (vNewPreviewRefPos != m_vPreviewRefPos)
  {
    m_vPreviewRefPos = vNewPreviewRefPos;

    UpdatePoses(true);
  }
}

void ezFakeRopeComponent::SendCurrentPose()
{
  ezMsgRopePoseUpdated poseMsg;

  ezDynamicArray<ezTransform> pieces(ezFrameAllocator::GetCurrentAllocator());

  if (m_RopeSim.m_Nodes.GetCount() >= 2)
  {
    const ezTransform tRoot = GetOwner()->GetGlobalTransform();

    pieces.SetCountUninitialized(m_RopeSim.m_Nodes.GetCount());

    for (ezUInt32 i = 0; i < pieces.GetCount(); ++i)
    {
      ezTransform tGlobal;
      tGlobal.SetIdentity();
      tGlobal.m_vPosition = m_RopeSim.m_Nodes[i].m_vPosition;
      // TODO: rotation

      pieces[i].SetLocalTransform(tRoot, tGlobal);
    }

    poseMsg.m_LinkTransforms = pieces;
  }

  GetOwner()->PostMessage(poseMsg, ezTime::Zero(), ezObjectMsgQueueType::AfterInitialized);
}

void ezFakeRopeComponent::SetAnchorAReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetAnchorA(resolver(szReference, GetHandle(), "AnchorA"));

  UpdatePreview();
}

void ezFakeRopeComponent::SetAnchorBReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetAnchorB(resolver(szReference, GetHandle(), "AnchorB"));

  UpdatePreview();
}

void ezFakeRopeComponent::SetAnchorA(ezGameObjectHandle hActor)
{
  m_hAnchorA = hActor;
}

void ezFakeRopeComponent::SetAnchorB(ezGameObjectHandle hActor)
{
  m_hAnchorB = hActor;
}

void ezFakeRopeComponent::SetLength(float val)
{
  m_fLength = val;

  UpdatePreview();
}

void ezFakeRopeComponent::SetAttachToA(bool val)
{
  m_RopeSim.m_bFirstNodeIsFixed = val;
}

bool ezFakeRopeComponent::GetAttachToA() const
{
  return m_RopeSim.m_bFirstNodeIsFixed;
}

void ezFakeRopeComponent::SetAttachToB(bool val)
{
  m_RopeSim.m_bLastNodeIsFixed = val;
}

bool ezFakeRopeComponent::GetAttachToB() const
{
  return m_RopeSim.m_bLastNodeIsFixed;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezFakeRopeComponentManager::ezFakeRopeComponentManager(ezWorld* pWorld)
  : ezComponentManager(pWorld)
{
}

ezFakeRopeComponentManager::~ezFakeRopeComponentManager() = default;

void ezFakeRopeComponentManager::Initialize()
{
  SUPER::Initialize();

  {
    auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezFakeRopeComponentManager::Update, this);
    desc.m_Phase = ezWorldModule::UpdateFunctionDesc::Phase::PostAsync;
    desc.m_bOnlyUpdateWhenSimulating = false;

    this->RegisterUpdateFunction(desc);
  }
}

void ezFakeRopeComponentManager::Update(const ezWorldModule::UpdateContext& context)
{
  if (!GetWorld()->GetWorldSimulationEnabled())
  {
    for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
    {
      if (it->IsActiveAndInitialized())
      {
        it->UpdatePreview();
      }
    }

    return;
  }

  if (GetWorld()->GetWorldSimulationEnabled())
  {
    for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
    {
      if (it->IsActiveAndInitialized())
      {
        it->UpdatePoses(false);
      }
    }
  }
}
