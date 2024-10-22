#include <GameEngine/GameEnginePCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameComponentsPlugin/Animation/CreatureCrawlComponent.h>
#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>
#include <RendererCore/AnimationSystem/Skeleton.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezCreatureLeg, ezNoBase, 1, ezRTTIDefaultAllocator<ezCreatureLeg>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("LegName", m_sLegObject),
    EZ_MEMBER_PROPERTY("StepGroup", m_uiStepGroup),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_COMPONENT_TYPE(ezCreatureCrawlComponent, 1, ezComponentMode::Dynamic);
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Body", DummyGetter, SetBodyReference)->AddAttributes(new ezGameObjectReferenceAttribute()),
    EZ_MEMBER_PROPERTY("CastUp", m_fCastUp)->AddAttributes(new ezDefaultValueAttribute(0.3f)),
    EZ_MEMBER_PROPERTY("CastDown", m_fCastDown)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("StepDistance", m_fStepDistance)->AddAttributes(new ezDefaultValueAttribute(0.4f)),
    EZ_MEMBER_PROPERTY("MinLegDistance", m_fMinLegDistance)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.1f, 1.0f)),
    EZ_ARRAY_MEMBER_PROPERTY("Legs", m_Legs),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
      new ezCategoryAttribute("Animation"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezCreatureCrawlComponent::ezCreatureCrawlComponent() = default;
ezCreatureCrawlComponent::~ezCreatureCrawlComponent() = default;

void ezCreatureCrawlComponent::SetBodyReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  m_hBody = resolver(szReference, GetHandle(), "Body");
}

void ezCreatureCrawlComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fCastUp;
  s << m_fCastDown;
  s << m_fStepDistance;
  s << m_fMinLegDistance;

  inout_stream.WriteGameObjectHandle(m_hBody);
  s << m_Legs.GetCount();
  for (ezUInt32 i = 0; i < m_Legs.GetCount(); ++i)
  {
    s << m_Legs[i].m_sLegObject;
    s << m_Legs[i].m_uiStepGroup;
  }
}

void ezCreatureCrawlComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_fCastUp;
  s >> m_fCastDown;
  s >> m_fStepDistance;
  s >> m_fMinLegDistance;

  m_hBody = inout_stream.ReadGameObjectHandle();
  ezUInt32 numLegs = 0;
  s >> numLegs;
  m_Legs.SetCount(numLegs);
  for (ezUInt32 i = 0; i < m_Legs.GetCount(); ++i)
  {
    s >> m_Legs[i].m_sLegObject;
    s >> m_Legs[i].m_uiStepGroup;
  }
}

void ezCreatureCrawlComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  const ezTransform invTrans = GetOwner()->GetGlobalTransform().GetInverse();

  for (auto& leg : m_Legs)
  {
    leg.m_vCurTargetPosAbs.SetZero();
    leg.m_fMoveLegFactor = 0.99f;
    leg.m_vRestPositionRelative.SetZero();
    leg.m_hLegObject.Invalidate();

    if (ezGameObject* pLeg = GetOwner()->FindChildByName(leg.m_sLegObject))
    {
      leg.m_hLegObject = pLeg->GetHandle();
      leg.m_vRestPositionRelative = invTrans * pLeg->GetGlobalPosition();
    }
  }
}

void ezCreatureCrawlComponent::Update()
{
  if (m_pPhysicsInterface == nullptr)
  {
    m_pPhysicsInterface = GetWorld()->GetModule<ezPhysicsWorldModuleInterface>();
    return;
  }

  const ezUInt32 uiNumLegs = m_Legs.GetCount();

  const ezVec3 vCenterPos = GetOwner()->GetGlobalPosition();
  const ezQuat qCenterRot = GetOwner()->GetGlobalRotation();

  ezHybridArray<ezVec3, 8> vNewTargetPos;
  vNewTargetPos.SetCount(uiNumLegs);

  ezWorld* pWorld = GetWorld();

  // TODO: make step duration configurable
  // TODO: make step height configurable
  const ezTime tStepDuration = ezTime::MakeFromMilliseconds(150);
  const float fStepHeight = 0.3f;
  const float fMoveAdd = ezMath::Min<float>(1.0f, GetWorld()->GetClock().GetTimeDiff().AsFloatInSeconds() / tStepDuration.AsFloatInSeconds());

  ezHybridArray<bool, 8> bLegMoving;
  bLegMoving.SetCount(uiNumLegs);
  bool bAnyLegMoving = false;

  for (ezUInt32 i = 0; i < uiNumLegs; ++i)
  {
    vNewTargetPos[i] = qCenterRot * (m_Legs[i].m_vRestPositionRelative + ezVec3(0, 0, m_fCastUp));
    bLegMoving[i] = m_Legs[i].m_fMoveLegFactor < 1.0f;
    bAnyLegMoving = bAnyLegMoving || bLegMoving[i];
  }

  bool bComputeBodyTilt = true;

  for (ezUInt32 i = 0; i < uiNumLegs; ++i)
  {
    if (m_Legs[i].m_hLegObject.IsInvalidated())
      continue;

    // find a place where the leg can stand
    // if none found at the rest position, try pulling the leg closer to the body in two steps (unless forbidden, see m_fMinLegDistance)

    ezPhysicsCastResult res;
    if (m_pPhysicsInterface->Raycast(res, vCenterPos + vNewTargetPos[i], ezVec3(0, 0, -1.0f), m_fCastDown, ezPhysicsQueryParameters(0, ezPhysicsShapeType::Static)))
    {
      vNewTargetPos[i] = res.m_vPosition;
    }
    else if (m_fMinLegDistance < 1.0f && m_pPhysicsInterface->Raycast(res, vCenterPos + vNewTargetPos[i] * ezMath::Lerp(m_fMinLegDistance, 1.0f, 0.5f), ezVec3(0, 0, -1.0f), m_fCastDown, ezPhysicsQueryParameters(0, ezPhysicsShapeType::Static)))
    {
      vNewTargetPos[i] = res.m_vPosition;
    }
    else if (m_fMinLegDistance < 1.0f && m_pPhysicsInterface->Raycast(res, vCenterPos + vNewTargetPos[i] * m_fMinLegDistance, ezVec3(0, 0, -1.0f), m_fCastDown, ezPhysicsQueryParameters(0, ezPhysicsShapeType::Static)))
    {
      vNewTargetPos[i] = res.m_vPosition;
    }
    else
    {
      bComputeBodyTilt = false;

      vNewTargetPos[i] += vCenterPos;
      m_Legs[i].m_vCurTargetPosAbs = ezMath::Lerp(m_Legs[i].m_vCurTargetPosAbs, vNewTargetPos[i], fMoveAdd);
    }
  }

  ezGameObject* pBody;
  if (!m_hBody.IsInvalidated() && pWorld->TryGetObject(m_hBody, pBody) && m_Legs.GetCount() == 4 /* TODO: remove safety check*/)
  {
    const ezQuat qOwnerRot = GetOwner()->GetGlobalRotation();

    ezQuat qNewBodyTilt;

    if (bComputeBodyTilt)
    {
      // TODO: compute body tilt from N points:
      // const ezTransform invTrans = GetOwner()->GetGlobalTransform().GetInverse();

      // ezVec3 vAvgDir(0);

      // for (ezUInt32 i = 0; i < uiNumLegs; ++i)
      //{
      //   ezVec3 vDir = invTrans * vNewTargetPos[i];
      //   vAvgDir += vDir;
      // }

      ezVec3 vAvgLegLeft = qOwnerRot.GetInverse() * ((vNewTargetPos[0] + vNewTargetPos[2]) * 0.5f);
      ezVec3 vAvgLegRight = qOwnerRot.GetInverse() * ((vNewTargetPos[1] + vNewTargetPos[3]) * 0.5f);
      vAvgLegLeft.x = 0.0f;
      vAvgLegRight.x = 0.0f;

      ezVec3 vAvgLegFwd = qOwnerRot.GetInverse() * ((vNewTargetPos[2] + vNewTargetPos[3]) * 0.5f);
      ezVec3 vAvgLegBack = qOwnerRot.GetInverse() * ((vNewTargetPos[0] + vNewTargetPos[1]) * 0.5f);
      vAvgLegFwd.y = 0.0f;
      vAvgLegBack.y = 0.0f;

      const ezVec3 vSideTilt = (vAvgLegRight - vAvgLegLeft).GetNormalized();
      const ezQuat qSideTilt = ezQuat::MakeShortestRotation(ezVec3(0, 1, 0), vSideTilt);

      const ezVec3 vFwdTilt = (vAvgLegFwd - vAvgLegBack).GetNormalized();
      const ezQuat qFwdTilt = ezQuat::MakeShortestRotation(ezVec3(1, 0, 0), vFwdTilt);

      qNewBodyTilt = qSideTilt * qFwdTilt;
    }
    else
    {
      qNewBodyTilt = ezQuat::MakeIdentity();
    }

    m_qBodyTilt = ezQuat::MakeSlerp(m_qBodyTilt, qNewBodyTilt, fMoveAdd);
    pBody->SetGlobalRotation(qOwnerRot * m_qBodyTilt);

    pBody->UpdateGlobalTransform();
  }

  if (!bAnyLegMoving)
  {
    float fMaxDistSqr = 0;
    ezUInt32 uiStepGroup = ezInvalidIndex;

    // check whether to move a leg
    for (ezUInt32 i = 0; i < uiNumLegs; ++i)
    {
      const float ds = (m_Legs[i].m_vCurTargetPosAbs - vNewTargetPos[i]).GetLengthSquared();

      if (ds > fMaxDistSqr)
      {
        fMaxDistSqr = ds;
        uiStepGroup = m_Legs[i].m_uiStepGroup;
      }
    }

    const ezTime tNow = pWorld->GetClock().GetAccumulatedTime();

    // TODO: make step delay configurable ?

    if (fMaxDistSqr >= ezMath::Square(m_fStepDistance) ||
        (fMaxDistSqr >= ezMath::Square(m_fStepDistance * 0.25) && (tNow - m_LastMove > ezTime::MakeFromSeconds(0.6f))))
    {
      m_LastMove = tNow;

      for (ezUInt32 i = 0; i < uiNumLegs; ++i)
      {
        if (m_Legs[i].m_uiStepGroup == uiStepGroup)
        {
          // move all legs from the same group simultaneously
          m_Legs[i].m_fMoveLegFactor = 0.0f;
        }
      }
    }
  }

  for (ezUInt32 i = 0; i < uiNumLegs; ++i)
  {
    if (bLegMoving[i])
    {
      // TODO: nicer step curve (sine instead of linear)

      m_Legs[i].m_fMoveLegFactor += fMoveAdd;

      if (m_Legs[i].m_fMoveLegFactor >= 1.0f)
      {
        m_Legs[i].m_fMoveLegFactor = 1.0f;
        m_Legs[i].m_vCurTargetPosAbs = vNewTargetPos[i];
      }
      else
      {
        float fSrcHeight = m_Legs[i].m_vCurTargetPosAbs.z;
        float fDstHeight = vNewTargetPos[i].z;
        float fMidHeight = ezMath::Max(fSrcHeight, fDstHeight) + fStepHeight;

        if (m_Legs[i].m_fMoveLegFactor < 0.5f)
        {
          vNewTargetPos[i].z = ezMath::Lerp(m_Legs[i].m_vCurTargetPosAbs.z, fMidHeight, m_Legs[i].m_fMoveLegFactor * 2.0f);
        }
        else
        {
          vNewTargetPos[i].z = ezMath::Lerp(fMidHeight, vNewTargetPos[i].z, (m_Legs[i].m_fMoveLegFactor - 0.5f) * 2.0f);
        }
      }

      vNewTargetPos[i] = ezMath::Lerp(m_Legs[i].m_vCurTargetPosAbs, vNewTargetPos[i], m_Legs[i].m_fMoveLegFactor);
    }
    else
    {
      vNewTargetPos[i] = m_Legs[i].m_vCurTargetPosAbs;
    }

    ezGameObject* pLegTarget;
    if (pWorld->TryGetObject(m_Legs[i].m_hLegObject, pLegTarget))
    {
      pLegTarget->SetGlobalPosition(vNewTargetPos[i]);
    }
  }
}


EZ_STATICLINK_FILE(GameComponentsPlugin, GameComponentsPlugin_Animation_Implementation_CreatureCrawlComponent);

