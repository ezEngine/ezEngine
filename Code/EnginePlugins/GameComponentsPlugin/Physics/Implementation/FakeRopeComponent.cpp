#include <GameComponentsPlugin/GameComponentsPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Interfaces/WindWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameComponentsPlugin/Physics/FakeRopeComponent.h>
#include <RendererCore/AnimationSystem/Declarations.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezFakeRopeComponent, 3, ezComponentMode::Static)
  {
    EZ_BEGIN_PROPERTIES
    {
      EZ_ACCESSOR_PROPERTY("Anchor1", DummyGetter, SetAnchor1Reference)->AddAttributes(new ezGameObjectReferenceAttribute()),
      EZ_ACCESSOR_PROPERTY("Anchor2", DummyGetter, SetAnchor2Reference)->AddAttributes(new ezGameObjectReferenceAttribute()),
      EZ_ACCESSOR_PROPERTY("AttachToAnchor1", GetAttachToAnchor1, SetAttachToAnchor1)->AddAttributes(new ezDefaultValueAttribute(true)),
      EZ_ACCESSOR_PROPERTY("AttachToAnchor2", GetAttachToAnchor2, SetAttachToAnchor2)->AddAttributes(new ezDefaultValueAttribute(true)),
      EZ_MEMBER_PROPERTY("Pieces", m_uiPieces)->AddAttributes(new ezDefaultValueAttribute(32), new ezClampValueAttribute(2, 200)),
      EZ_ACCESSOR_PROPERTY("Slack", GetSlack, SetSlack)->AddAttributes(new ezDefaultValueAttribute(0.2f)),
      EZ_MEMBER_PROPERTY("Damping", m_fDamping)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, 1.0f)),
      EZ_MEMBER_PROPERTY("WindInfluence", m_fWindInfluence)->AddAttributes(new ezDefaultValueAttribute(0.2f), new ezClampValueAttribute(0.0f, 10.0f)),
    }
    EZ_END_PROPERTIES;
    EZ_BEGIN_ATTRIBUTES
    {
      new ezCategoryAttribute("Effects/Ropes"),
    }
    EZ_END_ATTRIBUTES;
  }
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezFakeRopeComponent::ezFakeRopeComponent() = default;
ezFakeRopeComponent::~ezFakeRopeComponent() = default;

void ezFakeRopeComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_uiPieces;
  s << m_fSlack;
  s << m_fDamping;
  s << m_RopeSim.m_bFirstNodeIsFixed;
  s << m_RopeSim.m_bLastNodeIsFixed;

  inout_stream.WriteGameObjectHandle(m_hAnchor1);
  inout_stream.WriteGameObjectHandle(m_hAnchor2);

  s << m_fWindInfluence;
}

void ezFakeRopeComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_uiPieces;
  s >> m_fSlack;
  s >> m_fDamping;
  s >> m_RopeSim.m_bFirstNodeIsFixed;
  s >> m_RopeSim.m_bLastNodeIsFixed;

  if (uiVersion >= 3)
  {
    m_hAnchor1 = inout_stream.ReadGameObjectHandle();
  }

  m_hAnchor2 = inout_stream.ReadGameObjectHandle();

  if (uiVersion >= 2)
  {
    s >> m_fWindInfluence;
  }
}

void ezFakeRopeComponent::OnActivated()
{
  m_uiPreviewHash = 0;
  m_RopeSim.m_Nodes.Clear();
  m_RopeSim.m_fSegmentLength = -1.0f;

  m_uiCheckEquilibriumCounter = GetOwner()->GetStableRandomSeed() & 63;

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
  if (!m_bIsDynamic)
    return EZ_SUCCESS;

  if (!IsActiveAndInitialized())
    return EZ_FAILURE;

  ezGameObjectHandle hAnchor1 = m_hAnchor1;
  ezGameObjectHandle hAnchor2 = m_hAnchor2;

  if (hAnchor1.IsInvalidated())
    hAnchor1 = GetOwner()->GetHandle();
  if (hAnchor2.IsInvalidated())
    hAnchor2 = GetOwner()->GetHandle();

  if (hAnchor1 == hAnchor2)
    return EZ_FAILURE;

  ezSimdVec4f anchor1;
  ezSimdVec4f anchor2;

  ezGameObject* pAnchor1 = nullptr;
  ezGameObject* pAnchor2 = nullptr;

  if (!GetWorld()->TryGetObject(hAnchor1, pAnchor1))
  {
    // never set up so far
    if (m_RopeSim.m_Nodes.IsEmpty())
      return EZ_FAILURE;

    if (m_RopeSim.m_bFirstNodeIsFixed)
    {
      anchor1 = m_RopeSim.m_Nodes[0].m_vPosition;
      m_RopeSim.m_bFirstNodeIsFixed = false;
      m_uiSleepCounter = 0;
    }
  }
  else
  {
    anchor1 = ezSimdConversion::ToVec3(pAnchor1->GetGlobalPosition());
  }

  if (!GetWorld()->TryGetObject(hAnchor2, pAnchor2))
  {
    // never set up so far
    if (m_RopeSim.m_Nodes.IsEmpty())
      return EZ_FAILURE;

    if (m_RopeSim.m_bLastNodeIsFixed)
    {
      anchor2 = m_RopeSim.m_Nodes.PeekBack().m_vPosition;
      m_RopeSim.m_bLastNodeIsFixed = false;
      m_uiSleepCounter = 0;
    }
  }
  else
  {
    anchor2 = ezSimdConversion::ToVec3(pAnchor2->GetGlobalPosition());
  }

  // only early out, if we are not in edit mode
  m_bIsDynamic = !IsActiveAndSimulating() || (pAnchor1 != nullptr && pAnchor1->IsDynamic()) || (pAnchor2 != nullptr && pAnchor2->IsDynamic());

  m_RopeSim.m_fDampingFactor = ezMath::Lerp(1.0f, 0.97f, m_fDamping);

  if (m_RopeSim.m_fSegmentLength < 0)
  {
    const float len = (anchor1 - anchor2).GetLength<3>();
    m_RopeSim.m_fSegmentLength = (len + len * m_fSlack) / m_uiPieces;
  }

  if (const ezPhysicsWorldModuleInterface* pModule = GetWorld()->GetModuleReadOnly<ezPhysicsWorldModuleInterface>())
  {
    if (m_RopeSim.m_vAcceleration != pModule->GetGravity())
    {
      m_uiSleepCounter = 0;
      m_RopeSim.m_vAcceleration = pModule->GetGravity();
    }
  }

  if (m_uiPieces < m_RopeSim.m_Nodes.GetCount())
  {
    m_uiSleepCounter = 0;
    m_RopeSim.m_Nodes.SetCount(m_uiPieces);
  }
  else if (m_uiPieces > m_RopeSim.m_Nodes.GetCount())
  {
    m_uiSleepCounter = 0;
    const ezUInt32 uiOldNum = m_RopeSim.m_Nodes.GetCount();

    m_RopeSim.m_Nodes.SetCount(m_uiPieces);

    for (ezUInt32 i = uiOldNum; i < m_uiPieces; ++i)
    {
      m_RopeSim.m_Nodes[i].m_vPosition = anchor1 + ((anchor2 - anchor1) * (float)i / (m_uiPieces - 1));
      m_RopeSim.m_Nodes[i].m_vPreviousPosition = m_RopeSim.m_Nodes[i].m_vPosition;
    }
  }

  if (!m_RopeSim.m_Nodes.IsEmpty())
  {
    if (m_RopeSim.m_bFirstNodeIsFixed)
    {
      if ((m_RopeSim.m_Nodes[0].m_vPosition != anchor1).AnySet<3>())
      {
        m_uiSleepCounter = 0;
        m_RopeSim.m_Nodes[0].m_vPosition = anchor1;
      }
    }

    if (m_RopeSim.m_bLastNodeIsFixed)
    {
      if ((m_RopeSim.m_Nodes.PeekBack().m_vPosition != anchor2).AnySet<3>())
      {
        m_uiSleepCounter = 0;
        m_RopeSim.m_Nodes.PeekBack().m_vPosition = anchor2;
      }
    }
  }

  return EZ_SUCCESS;
}

void ezFakeRopeComponent::SendPreviewPose()
{
  if (!IsActiveAndInitialized() || IsActiveAndSimulating())
    return;

  ezGameObject* pAnchor1 = nullptr;
  ezGameObject* pAnchor2 = nullptr;
  if (!GetWorld()->TryGetObject(m_hAnchor1, pAnchor1))
    pAnchor1 = GetOwner();
  if (!GetWorld()->TryGetObject(m_hAnchor2, pAnchor2))
    pAnchor2 = GetOwner();

  if (pAnchor1 == pAnchor2)
    return;

  ezUInt32 uiHash = 0;

  ezVec3 pos = GetOwner()->GetGlobalPosition();
  uiHash = ezHashingUtils::xxHash32(&pos, sizeof(ezVec3), uiHash);

  pos = pAnchor1->GetGlobalPosition();
  uiHash = ezHashingUtils::xxHash32(&pos, sizeof(ezVec3), uiHash);

  pos = pAnchor2->GetGlobalPosition();
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

  ezVec3 acc(0);

  if (const ezPhysicsWorldModuleInterface* pModule = GetWorld()->GetModuleReadOnly<ezPhysicsWorldModuleInterface>())
  {
    acc += pModule->GetGravity();
  }
  else
  {
    acc += ezVec3(0, 0, -9.81f);
  }

  if (m_fWindInfluence > 0.0f)
  {
    if (const ezWindWorldModuleInterface* pWind = GetWorld()->GetModuleReadOnly<ezWindWorldModuleInterface>())
    {
      const ezSimdVec4f ropeDir = m_RopeSim.m_Nodes.PeekBack().m_vPosition - m_RopeSim.m_Nodes[0].m_vPosition;

      ezVec3 vWind = pWind->GetWindAt(ezSimdConversion::ToVec3(m_RopeSim.m_Nodes.PeekBack().m_vPosition));
      vWind += pWind->GetWindAt(ezSimdConversion::ToVec3(m_RopeSim.m_Nodes[0].m_vPosition));
      vWind *= 0.5f * m_fWindInfluence;

      acc += vWind;
      acc += pWind->ComputeWindFlutter(vWind, ezSimdConversion::ToVec3(ropeDir), 0.5f, GetOwner()->GetStableRandomSeed());
    }
  }

  if (m_RopeSim.m_vAcceleration != acc)
  {
    m_RopeSim.m_vAcceleration = acc;
    m_uiSleepCounter = 0;
  }

  if (m_uiSleepCounter > 10)
    return;

  ezVisibilityState::Enum visType = GetOwner()->GetVisibilityState();

  if (visType == ezVisibilityState::Invisible)
    return;

  m_RopeSim.SimulateRope(GetWorld()->GetClock().GetTimeDiff());

  ++m_uiCheckEquilibriumCounter;
  if (m_uiCheckEquilibriumCounter > 64)
  {
    m_uiCheckEquilibriumCounter = 0;

    if (m_RopeSim.HasEquilibrium(0.01f))
    {
      ++m_uiSleepCounter;
    }
    else
    {
      m_uiSleepCounter = 0;
    }
  }

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

    ezTransform tGlobal;
    tGlobal.m_vScale.Set(1);

    for (ezUInt32 i = 0; i < pieces.GetCount() - 1; ++i)
    {
      const ezSimdVec4f p0 = m_RopeSim.m_Nodes[i].m_vPosition;
      const ezSimdVec4f p1 = m_RopeSim.m_Nodes[i + 1].m_vPosition;
      ezSimdVec4f dir = p1 - p0;

      if (dir.IsZero<3>(0.0001f))
      {
        dir.Set(1, 0, 0, 0);
      }
      else
      {
        dir.Normalize<3>();
      }

      tGlobal.m_vPosition = ezSimdConversion::ToVec3(p0);
      tGlobal.m_qRotation = ezQuat::MakeShortestRotation(ezVec3::MakeAxisX(), ezSimdConversion::ToVec3(dir));

      pieces[i] = ezTransform::MakeLocalTransform(tRoot, tGlobal);
    }

    {
      tGlobal.m_vPosition = ezSimdConversion::ToVec3(m_RopeSim.m_Nodes.PeekBack().m_vPosition);
      // tGlobal.m_qRotation is the same as from the previous bone

      pieces.PeekBack() = ezTransform::MakeLocalTransform(tRoot, tGlobal);
    }

    poseMsg.m_LinkTransforms = pieces;
  }

  GetOwner()->PostMessage(poseMsg, ezTime::MakeZero(), ezObjectMsgQueueType::AfterInitialized);
}

void ezFakeRopeComponent::SetAnchor1Reference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetAnchor1(resolver(szReference, GetHandle(), "Anchor1"));
}

void ezFakeRopeComponent::SetAnchor2Reference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetAnchor2(resolver(szReference, GetHandle(), "Anchor2"));
}

void ezFakeRopeComponent::SetAnchor1(ezGameObjectHandle hActor)
{
  m_hAnchor1 = hActor;
  m_bIsDynamic = true;
  m_uiSleepCounter = 0;
}

void ezFakeRopeComponent::SetAnchor2(ezGameObjectHandle hActor)
{
  m_hAnchor2 = hActor;
  m_bIsDynamic = true;
  m_uiSleepCounter = 0;
}

void ezFakeRopeComponent::SetSlack(float fVal)
{
  m_fSlack = fVal;
  m_RopeSim.m_fSegmentLength = -1.0f;
  m_bIsDynamic = true;
  m_uiSleepCounter = 0;
}

void ezFakeRopeComponent::SetAttachToAnchor1(bool bVal)
{
  m_RopeSim.m_bFirstNodeIsFixed = bVal;
  m_bIsDynamic = true;
  m_uiSleepCounter = 0;
}

void ezFakeRopeComponent::SetAttachToAnchor2(bool bVal)
{
  m_RopeSim.m_bLastNodeIsFixed = bVal;
  m_bIsDynamic = true;
  m_uiSleepCounter = 0;
}

bool ezFakeRopeComponent::GetAttachToAnchor1() const
{
  return m_RopeSim.m_bFirstNodeIsFixed;
}

bool ezFakeRopeComponent::GetAttachToAnchor2() const
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
    desc.m_Phase = ezWorldUpdatePhase::Async;
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


EZ_STATICLINK_FILE(GameComponentsPlugin, GameComponentsPlugin_Physics_Implementation_FakeRopeComponent);
