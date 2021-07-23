#include <GameEnginePCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Physics/FakeRopeComponent.h>
#include <RendererCore/AnimationSystem/Declarations.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezFakeRopeComponent, 1, ezComponentMode::Static)
  {
    EZ_BEGIN_PROPERTIES
    {
      EZ_ACCESSOR_PROPERTY("Anchor", DummyGetter, SetAnchorReference)->AddAttributes(new ezGameObjectReferenceAttribute()),
      EZ_ACCESSOR_PROPERTY("AttachToOrigin", GetAttachToOrigin, SetAttachToOrigin)->AddAttributes(new ezDefaultValueAttribute(true)),
      EZ_ACCESSOR_PROPERTY("AttachToAnchor", GetAttachToAnchor, SetAttachToAnchor)->AddAttributes(new ezDefaultValueAttribute(true)),
      EZ_ACCESSOR_PROPERTY("Slack", GetSlack, SetSlack)->AddAttributes(new ezDefaultValueAttribute(0.2f)),
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
  s << m_fSlack;
  s << m_fDamping;
  s << m_RopeSim.m_bFirstNodeIsFixed;
  s << m_RopeSim.m_bLastNodeIsFixed;

  stream.WriteGameObjectHandle(m_hAnchor);
}

void ezFakeRopeComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_uiPieces;
  s >> m_fSlack;
  s >> m_fDamping;
  s >> m_RopeSim.m_bFirstNodeIsFixed;
  s >> m_RopeSim.m_bLastNodeIsFixed;

  m_hAnchor = stream.ReadGameObjectHandle();
}

void ezFakeRopeComponent::OnActivated()
{
  m_uiPreviewHash = 0;
  m_RopeSim.m_Nodes.Clear();
  m_RopeSim.m_fSegmentLength = -1.0f;

  SendPreviewPose();
}

void ezFakeRopeComponent::OnDeactivated()
{
  // tell the render components, that the rope is gone
  m_RopeSim.m_Nodes.Clear();
  SendCurrentPose();

  SUPER::OnDeactivated();
}

ezResult ezFakeRopeComponent::ConfigureRopeSimulator()
{
  if (!IsActiveAndInitialized())
    return EZ_FAILURE;

  ezGameObject* pAnchor;
  if (!GetWorld()->TryGetObject(m_hAnchor, pAnchor))
    return EZ_FAILURE;

  const ezVec3 anchorA = GetOwner()->GetGlobalPosition();
  const ezVec3 anchorB = pAnchor->GetGlobalPosition();

  m_RopeSim.m_fDampingFactor = ezMath::Lerp(1.0f, 0.97f, m_fDamping);

  if (m_RopeSim.m_fSegmentLength < 0)
  {
    const float len = (anchorA - anchorB).GetLength();
    m_RopeSim.m_fSegmentLength = (len + len * m_fSlack) / m_uiPieces;
  }

  if (const ezPhysicsWorldModuleInterface* pModule = GetWorld()->GetModuleReadOnly<ezPhysicsWorldModuleInterface>())
  {
    m_RopeSim.m_vAcceleration = pModule->GetGravity();
  }

  if (m_uiPieces < m_RopeSim.m_Nodes.GetCount())
  {
    m_RopeSim.m_Nodes.SetCount(m_uiPieces);
  }
  else if (m_uiPieces > m_RopeSim.m_Nodes.GetCount())
  {

    const ezUInt32 uiOldNum = m_RopeSim.m_Nodes.GetCount();

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
      m_RopeSim.m_Nodes[0].m_vPosition = anchorA;
    }

    if (m_RopeSim.m_bLastNodeIsFixed)
    {
      m_RopeSim.m_Nodes.PeekBack().m_vPosition = anchorB;
    }
  }

  return EZ_SUCCESS;
}

void ezFakeRopeComponent::SendPreviewPose()
{
  if (!IsActiveAndInitialized() || IsActiveAndSimulating())
    return;

  ezUInt32 uiHash = 0;

  ezGameObject* pAnchor;
  if (!GetWorld()->TryGetObject(m_hAnchor, pAnchor))
    return;

  ezVec3 pos = GetOwner()->GetGlobalPosition();
  uiHash = ezHashingUtils::xxHash32(&pos, sizeof(ezVec3), uiHash);

  pos = pAnchor->GetGlobalPosition();
  uiHash = ezHashingUtils::xxHash32(&pos, sizeof(ezVec3), uiHash);

  uiHash = ezHashingUtils::xxHash32(&m_fSlack, sizeof(float), uiHash);
  uiHash = ezHashingUtils::xxHash32(&m_fDamping, sizeof(float), uiHash);
  uiHash = ezHashingUtils::xxHash32(&m_uiPieces, sizeof(ezUInt16), uiHash);

  if (uiHash == m_uiPreviewHash)
    return;

  m_uiPreviewHash = uiHash;
  m_RopeSim.m_fSegmentLength = -1.0f;

  if (ConfigureRopeSimulator().Failed())
    return;

  m_RopeSim.SimulateTillEquilibrium(0.003f, 100);

  SendCurrentPose();
}

void ezFakeRopeComponent::RuntimeUpdate()
{
  if (ConfigureRopeSimulator().Failed())
    return;

  // TODO: detect no change and early out
  m_RopeSim.SimulateRope(GetWorld()->GetClock().GetTimeDiff());

  SendCurrentPose();
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

void ezFakeRopeComponent::SetAnchorReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetAnchor(resolver(szReference, GetHandle(), "Anchor"));
}

void ezFakeRopeComponent::SetAnchor(ezGameObjectHandle hActor)
{
  m_hAnchor = hActor;
}

void ezFakeRopeComponent::SetSlack(float val)
{
  m_fSlack = val;
  m_RopeSim.m_fSegmentLength = -1.0f;
}

void ezFakeRopeComponent::SetAttachToOrigin(bool val)
{
  m_RopeSim.m_bFirstNodeIsFixed = val;
}

bool ezFakeRopeComponent::GetAttachToOrigin() const
{
  return m_RopeSim.m_bFirstNodeIsFixed;
}

void ezFakeRopeComponent::SetAttachToAnchor(bool val)
{
  m_RopeSim.m_bLastNodeIsFixed = val;
}

bool ezFakeRopeComponent::GetAttachToAnchor() const
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
    desc.m_Phase = ezWorldModule::UpdateFunctionDesc::Phase::Async;
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
        it->SendPreviewPose();
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
        it->RuntimeUpdate();
      }
    }
  }
}
