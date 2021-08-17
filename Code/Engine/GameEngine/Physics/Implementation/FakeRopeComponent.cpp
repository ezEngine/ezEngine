#include <GameEngine/GameEnginePCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Interfaces/WindWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Physics/FakeRopeComponent.h>
#include <RendererCore/AnimationSystem/Declarations.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezFakeRopeComponent, 2, ezComponentMode::Static)
  {
    EZ_BEGIN_PROPERTIES
    {
      EZ_ACCESSOR_PROPERTY("Anchor", DummyGetter, SetAnchorReference)->AddAttributes(new ezGameObjectReferenceAttribute()),
      EZ_ACCESSOR_PROPERTY("AttachToOrigin", GetAttachToOrigin, SetAttachToOrigin)->AddAttributes(new ezDefaultValueAttribute(true)),
      EZ_ACCESSOR_PROPERTY("AttachToAnchor", GetAttachToAnchor, SetAttachToAnchor)->AddAttributes(new ezDefaultValueAttribute(true)),
      EZ_MEMBER_PROPERTY("Pieces", m_uiPieces)->AddAttributes(new ezDefaultValueAttribute(32), new ezClampValueAttribute(2, 200)),
      EZ_ACCESSOR_PROPERTY("Slack", GetSlack, SetSlack)->AddAttributes(new ezDefaultValueAttribute(0.2f)),
      EZ_MEMBER_PROPERTY("Damping", m_fDamping)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, 1.0f)),
      EZ_MEMBER_PROPERTY("WindInfluence", m_fWindInfluence)->AddAttributes(new ezDefaultValueAttribute(0.2f), new ezClampValueAttribute(0.0f, 10.0f)),
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

  s << m_fWindInfluence;
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

  ezSimdVec4f anchorB;

  ezGameObject* pAnchor = nullptr;
  if (!GetWorld()->TryGetObject(m_hAnchor, pAnchor))
  {
    // never set up so far
    if (m_RopeSim.m_Nodes.IsEmpty())
      return EZ_FAILURE;

    if (m_RopeSim.m_bLastNodeIsFixed)
    {
      anchorB = m_RopeSim.m_Nodes.PeekBack().m_vPosition;
      m_RopeSim.m_bLastNodeIsFixed = false;
      m_uiSleepCounter = 0;
    }
  }
  else
  {
    anchorB = ezSimdConversion::ToVec3(pAnchor->GetGlobalPosition());
  }

  // only early out, if we are not in edit mode
  m_bIsDynamic = !IsActiveAndSimulating() || GetOwner()->IsDynamic() || (pAnchor != nullptr && pAnchor->IsDynamic());

  const ezSimdVec4f anchorA = ezSimdConversion::ToVec3(GetOwner()->GetGlobalPosition());

  m_RopeSim.m_fDampingFactor = ezMath::Lerp(1.0f, 0.97f, m_fDamping);

  if (m_RopeSim.m_fSegmentLength < 0)
  {
    const float len = (anchorA - anchorB).GetLength<3>();
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
      m_RopeSim.m_Nodes[i].m_vPosition = anchorA + ((anchorB - anchorA) * (float)i / (m_uiPieces - 1));
      m_RopeSim.m_Nodes[i].m_vPreviousPosition = m_RopeSim.m_Nodes[i].m_vPosition;
    }
  }

  if (!m_RopeSim.m_Nodes.IsEmpty())
  {
    if (m_RopeSim.m_bFirstNodeIsFixed)
    {
      if ((m_RopeSim.m_Nodes[0].m_vPosition != anchorA).AnySet<3>())
      {
        m_uiSleepCounter = 0;
        m_RopeSim.m_Nodes[0].m_vPosition = anchorA;
      }
    }

    if (m_RopeSim.m_bLastNodeIsFixed)
    {
      if ((m_RopeSim.m_Nodes.PeekBack().m_vPosition != anchorB).AnySet<3>())
      {
        m_uiSleepCounter = 0;
        m_RopeSim.m_Nodes.PeekBack().m_vPosition = anchorB;
      }
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

  ezUInt64 uiFramesVisible = GetOwner()->GetNumFramesSinceVisible();
  if (uiFramesVisible > 60)
  {
    return;
  }


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

      dir.NormalizeIfNotZero<3>();

      tGlobal.m_vPosition = ezSimdConversion::ToVec3(p0);
      tGlobal.m_qRotation.SetShortestRotation(ezVec3::UnitXAxis(), ezSimdConversion::ToVec3(dir));

      pieces[i].SetLocalTransform(tRoot, tGlobal);
    }

    {
      tGlobal.m_vPosition = ezSimdConversion::ToVec3(m_RopeSim.m_Nodes.PeekBack().m_vPosition);
      // tGlobal.m_qRotation is the same as from the previous bone

      pieces.PeekBack().SetLocalTransform(tRoot, tGlobal);
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
  m_bIsDynamic = true;
  m_uiSleepCounter = 0;
}

void ezFakeRopeComponent::SetSlack(float val)
{
  m_fSlack = val;
  m_RopeSim.m_fSegmentLength = -1.0f;
  m_bIsDynamic = true;
  m_uiSleepCounter = 0;
}

void ezFakeRopeComponent::SetAttachToOrigin(bool val)
{
  m_RopeSim.m_bFirstNodeIsFixed = val;
  m_bIsDynamic = true;
  m_uiSleepCounter = 0;
}

bool ezFakeRopeComponent::GetAttachToOrigin() const
{
  return m_RopeSim.m_bFirstNodeIsFixed;
}

void ezFakeRopeComponent::SetAttachToAnchor(bool val)
{
  m_RopeSim.m_bLastNodeIsFixed = val;
  m_bIsDynamic = true;
  m_uiSleepCounter = 0;
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
