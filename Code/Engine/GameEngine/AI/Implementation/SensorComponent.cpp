#include <GameEngine/GameEnginePCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/AI/SensorComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>

// clang-format off
EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgSensorDetectedObjectsChanged);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgSensorDetectedObjectsChanged, 1, ezRTTIDefaultAllocator<ezMsgSensorDetectedObjectsChanged>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ARRAY_MEMBER_PROPERTY_READ_ONLY("DetectedObjects", m_DetectedObjects),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezSensorComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("UpdateRate", ezUpdateRate, m_UpdateRate),
    EZ_ACCESSOR_PROPERTY("SpatialCategory", GetSpatialCategory, SetSpatialCategory)->AddAttributes(new ezDynamicStringEnumAttribute("SpatialDataCategoryEnum")),
    EZ_MEMBER_PROPERTY("TestVisibility", m_bTestVisibility)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),    
    EZ_ACCESSOR_PROPERTY("ShowDebugInfo", GetShowDebugInfo, SetShowDebugInfo),
    EZ_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new ezDefaultValueAttribute(ezColorScheme::LightUI(ezColorScheme::Orange))),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("AI/Sensors"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

ezSensorComponent::ezSensorComponent() = default;
ezSensorComponent::~ezSensorComponent() = default;

void ezSensorComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_sSpatialCategory;
  s << m_bTestVisibility;
  s << m_uiCollisionLayer;
  s << m_UpdateRate;
  s << m_bShowDebugInfo;
  s << m_Color;
}

void ezSensorComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_sSpatialCategory;
  s >> m_bTestVisibility;
  s >> m_uiCollisionLayer;
  s >> m_UpdateRate;
  s >> m_bShowDebugInfo;
  s >> m_Color;
}

void ezSensorComponent::OnActivated()
{
  SUPER::OnActivated();

  UpdateSpatialCategory();
  UpdateScheduling();
  UpdateDebugInfo();
}

void ezSensorComponent::OnDeactivated()
{
  auto pModule = GetWorld()->GetOrCreateModule<ezSensorWorldModule>();
  pModule->RemoveComponentToSchedule(this);

  UpdateDebugInfo();
}

void ezSensorComponent::SetSpatialCategory(const char* szCategory)
{
  m_sSpatialCategory.Assign(szCategory);

  if (IsActiveAndInitialized())
  {
    UpdateSpatialCategory();
  }
}

const char* ezSensorComponent::GetSpatialCategory() const
{
  return m_sSpatialCategory;
}

void ezSensorComponent::SetUpdateRate(const ezEnum<ezUpdateRate>& updateRate)
{
  if (m_UpdateRate == updateRate)
    return;

  m_UpdateRate = updateRate;

  if (IsActiveAndInitialized())
  {
    UpdateScheduling();
  }
}

const ezEnum<ezUpdateRate>& ezSensorComponent::GetUpdateRate() const
{
  return m_UpdateRate;
}

void ezSensorComponent::SetShowDebugInfo(bool bShow)
{
  if (m_bShowDebugInfo == bShow)
    return;

  m_bShowDebugInfo = bShow;

  if (IsActiveAndInitialized())
  {
    UpdateDebugInfo();
  }
}

bool ezSensorComponent::GetShowDebugInfo() const
{
  return m_bShowDebugInfo;
}

void ezSensorComponent::SetColor(ezColorGammaUB color)
{
  m_Color = color;
}

ezColorGammaUB ezSensorComponent::GetColor() const
{
  return m_Color;
}

void ezSensorComponent::UpdateSpatialCategory()
{
  if (!m_sSpatialCategory.IsEmpty())
  {
    m_SpatialCategory = ezSpatialData::RegisterCategory(m_sSpatialCategory, ezSpatialData::Flags::None);
  }
  else
  {
    m_SpatialCategory = ezInvalidSpatialDataCategory;
  }
}

void ezSensorComponent::UpdateScheduling()
{
  auto pModule = GetWorld()->GetOrCreateModule<ezSensorWorldModule>();
  pModule->AddComponentToSchedule(this, m_UpdateRate);
}

void ezSensorComponent::UpdateDebugInfo()
{
  auto pModule = GetWorld()->GetOrCreateModule<ezSensorWorldModule>();
  if (IsActiveAndInitialized() && m_bShowDebugInfo)
  {
    pModule->AddComponentForDebugRendering(this);
  }
  else
  {
    pModule->RemoveComponentForDebugRendering(this);
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezSensorSphereComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new ezDefaultValueAttribute(10.0f), new ezClampValueAttribute(0.0f, ezVariant())),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezSphereManipulatorAttribute("Radius"),
    new ezSphereVisualizerAttribute("Radius", ezColor::White, "Color"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezSensorSphereComponent::ezSensorSphereComponent() = default;
ezSensorSphereComponent::~ezSensorSphereComponent() = default;

void ezSensorSphereComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fRadius;
}

void ezSensorSphereComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_fRadius;
}

void ezSensorSphereComponent::GetObjectsInSensorVolume(ezDynamicArray<ezGameObject*>& out_objects) const
{
  const ezGameObject* pOwner = GetOwner();

  const float scale = pOwner->GetGlobalTransformSimd().GetMaxScale();
  ezBoundingSphere sphere = ezBoundingSphere(pOwner->GetGlobalPosition(), m_fRadius * scale);

  ezSpatialSystem::QueryParams params;
  params.m_uiCategoryBitmask = m_SpatialCategory.GetBitmask();

  ezSimdMat4f toLocalSpace = pOwner->GetGlobalTransformSimd().GetAsMat4().GetInverse();
  ezSimdFloat radiusSquared = m_fRadius * m_fRadius;

  GetWorld()->GetSpatialSystem()->FindObjectsInSphere(sphere, params, [&](ezGameObject* pObject) {
    ezSimdVec4f localSpacePos = toLocalSpace.TransformPosition(pObject->GetGlobalPositionSimd());
    const bool bInRadius = localSpacePos.GetLengthSquared<3>() <= radiusSquared;

    if (bInRadius)
    {
      out_objects.PushBack(pObject);
    }

    return ezVisitorExecution::Continue; });
}

void ezSensorSphereComponent::DebugDrawSensorShape() const
{
  ezBoundingSphere sphere = ezBoundingSphere(ezVec3::ZeroVector(), m_fRadius);
  ezDebugRenderer::DrawLineSphere(GetWorld(), sphere, m_Color, GetOwner()->GetGlobalTransform());
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezSensorCylinderComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new ezDefaultValueAttribute(10.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("Height", m_fHeight)->AddAttributes(new ezDefaultValueAttribute(10.0f), new ezClampValueAttribute(0.0f, ezVariant())),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezCylinderVisualizerAttribute(ezBasisAxis::PositiveZ, "Height", "Radius", ezColor::White, "Color"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezSensorCylinderComponent::ezSensorCylinderComponent() = default;
ezSensorCylinderComponent::~ezSensorCylinderComponent() = default;

void ezSensorCylinderComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fRadius;
  s << m_fHeight;
}

void ezSensorCylinderComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_fRadius;
  s >> m_fHeight;
}

void ezSensorCylinderComponent::GetObjectsInSensorVolume(ezDynamicArray<ezGameObject*>& out_objects) const
{
  const ezGameObject* pOwner = GetOwner();

  const ezVec3 scale = pOwner->GetGlobalScaling().Abs();
  const float xyScale = ezMath::Max(scale.x, scale.y);

  const float sphereRadius = ezVec2(m_fRadius * xyScale, m_fHeight * 0.5f * scale.z).GetLength();
  ezBoundingSphere sphere = ezBoundingSphere(pOwner->GetGlobalPosition(), sphereRadius);

  ezSpatialSystem::QueryParams params;
  params.m_uiCategoryBitmask = m_SpatialCategory.GetBitmask();

  ezSimdMat4f toLocalSpace = pOwner->GetGlobalTransformSimd().GetAsMat4().GetInverse();
  ezSimdFloat radiusSquared = m_fRadius * m_fRadius;
  ezSimdFloat halfHeight = m_fHeight * 0.5f;

  GetWorld()->GetSpatialSystem()->FindObjectsInSphere(sphere, params, [&](ezGameObject* pObject) {
    ezSimdVec4f localSpacePos = toLocalSpace.TransformPosition(pObject->GetGlobalPositionSimd());
    const bool bInRadius = localSpacePos.GetLengthSquared<2>() <= radiusSquared;
    const bool bInHeight = localSpacePos.Abs().z() <= halfHeight;

    if (bInRadius && bInHeight)
    {
      out_objects.PushBack(pObject);
    }

    return ezVisitorExecution::Continue; });
}

void ezSensorCylinderComponent::DebugDrawSensorShape() const
{
  ezTransform pt = GetOwner()->GetGlobalTransform();

  ezQuat r;
  r.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(-90.0f));
  ezTransform t = ezTransform(ezVec3(0, 0, -0.5f * m_fHeight * pt.m_vScale.z), r, ezVec3(pt.m_vScale.z, pt.m_vScale.y, pt.m_vScale.x));

  pt.m_vScale.Set(1);
  t = pt * t;

  ezColor solidColor = ezColor::Black.WithAlpha(0.0f); // lines only
  ezDebugRenderer::DrawCylinder(GetWorld(), m_fRadius, m_fRadius, m_fHeight, solidColor, m_Color, t);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezSensorConeComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("NearDistance", m_fNearDistance)->AddAttributes(new ezDefaultValueAttribute(0.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("FarDistance", m_fFarDistance)->AddAttributes(new ezDefaultValueAttribute(10.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("Angle", m_Angle)->AddAttributes(new ezDefaultValueAttribute(ezAngle::Degree(90.0f)), new ezClampValueAttribute(0.0f, ezAngle::Degree(180.0f))),
  }
  EZ_END_PROPERTIES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezSensorConeComponent::ezSensorConeComponent() = default;
ezSensorConeComponent::~ezSensorConeComponent() = default;

void ezSensorConeComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fNearDistance;
  s << m_fFarDistance;
  s << m_Angle;
}

void ezSensorConeComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_fNearDistance;
  s >> m_fFarDistance;
  s >> m_Angle;
}

void ezSensorConeComponent::GetObjectsInSensorVolume(ezDynamicArray<ezGameObject*>& out_objects) const
{
  const ezGameObject* pOwner = GetOwner();

  const float scale = pOwner->GetGlobalTransformSimd().GetMaxScale();
  ezBoundingSphere sphere = ezBoundingSphere(pOwner->GetGlobalPosition(), m_fFarDistance * scale);

  ezSpatialSystem::QueryParams params;
  params.m_uiCategoryBitmask = m_SpatialCategory.GetBitmask();

  ezSimdMat4f toLocalSpace = pOwner->GetGlobalTransformSimd().GetAsMat4().GetInverse();
  const ezSimdFloat nearSquared = m_fNearDistance * m_fNearDistance;
  const ezSimdFloat farSquared = m_fFarDistance * m_fFarDistance;
  const ezSimdFloat cosAngle = ezMath::Cos(m_Angle * 0.5f);

  GetWorld()->GetSpatialSystem()->FindObjectsInSphere(sphere, params, [&](ezGameObject* pObject) {
    ezSimdVec4f localSpacePos = toLocalSpace.TransformPosition(pObject->GetGlobalPositionSimd());
    const ezSimdFloat fDistanceSquared = localSpacePos.GetLengthSquared<3>();
    const bool bInDistance = fDistanceSquared >= nearSquared && fDistanceSquared <= farSquared;

    const ezSimdVec4f normalizedPos = localSpacePos * fDistanceSquared.GetInvSqrt();
    const bool bInAngle = normalizedPos.x() >= cosAngle;

    if (bInDistance && bInAngle)
    {
      out_objects.PushBack(pObject);
    }

    return ezVisitorExecution::Continue; });
}

void ezSensorConeComponent::DebugDrawSensorShape() const
{
  constexpr ezUInt32 MIN_SEGMENTS = 3;
  constexpr ezUInt32 MAX_SEGMENTS = 16;
  constexpr ezUInt32 CIRCLE_SEGMENTS = MAX_SEGMENTS * 2;
  constexpr ezUInt32 NUM_LINES = MAX_SEGMENTS * 4 + CIRCLE_SEGMENTS * 2 + 4;

  ezDebugRenderer::Line lines[NUM_LINES];
  ezUInt32 curLine = 0;

  const ezUInt32 numSegments = ezMath::Clamp(static_cast<ezUInt32>(m_Angle / ezAngle::Degree(180)) * MAX_SEGMENTS, MIN_SEGMENTS, MAX_SEGMENTS);
  const ezAngle stepAngle = m_Angle / static_cast<float>(numSegments);
  const ezAngle circleStepAngle = ezAngle::Degree(360.0f / CIRCLE_SEGMENTS);

  for (ezUInt32 i = 0; i < 2; ++i)
  {
    ezAngle curAngle = m_Angle * -0.5f;

    ezQuat q;
    float fX = ezMath::Cos(curAngle);
    float fCircleRadius = ezMath::Sin(curAngle);

    if (i == 0)
    {
      q.SetIdentity();
      fX *= m_fNearDistance;
      fCircleRadius *= m_fNearDistance;
    }
    else
    {
      q.SetFromAxisAndAngle(ezVec3::UnitXAxis(), ezAngle::Degree(90));
      fX *= m_fFarDistance;
      fCircleRadius *= m_fFarDistance;
    }

    for (ezUInt32 s = 0; s < numSegments; ++s)
    {
      const ezAngle nextAngle = curAngle + stepAngle;

      const float fCos1 = ezMath::Cos(curAngle);
      const float fCos2 = ezMath::Cos(nextAngle);

      const float fSin1 = ezMath::Sin(curAngle);
      const float fSin2 = ezMath::Sin(nextAngle);

      curAngle = nextAngle;

      const ezVec3 p1 = q * ezVec3(fCos1, fSin1, 0.0f);
      const ezVec3 p2 = q * ezVec3(fCos2, fSin2, 0.0f);

      lines[curLine].m_start = p1 * m_fNearDistance;
      lines[curLine].m_end = p2 * m_fNearDistance;
      ++curLine;

      lines[curLine].m_start = p1 * m_fFarDistance;
      lines[curLine].m_end = p2 * m_fFarDistance;
      ++curLine;

      if (s == 0)
      {
        lines[curLine].m_start = p1 * m_fNearDistance;
        lines[curLine].m_end = p1 * m_fFarDistance;
        ++curLine;
      }
      else if (s == numSegments - 1)
      {
        lines[curLine].m_start = p2 * m_fNearDistance;
        lines[curLine].m_end = p2 * m_fFarDistance;
        ++curLine;
      }
    }

    curAngle = ezAngle::Degree(0.0f);
    for (ezUInt32 s = 0; s < CIRCLE_SEGMENTS; ++s)
    {
      const ezAngle nextAngle = curAngle + circleStepAngle;

      const float fCos1 = ezMath::Cos(curAngle);
      const float fCos2 = ezMath::Cos(nextAngle);

      const float fSin1 = ezMath::Sin(curAngle);
      const float fSin2 = ezMath::Sin(nextAngle);

      curAngle = nextAngle;

      const ezVec3 p1 = ezVec3(fX, fCos1 * fCircleRadius, fSin1 * fCircleRadius);
      const ezVec3 p2 = ezVec3(fX, fCos2 * fCircleRadius, fSin2 * fCircleRadius);

      lines[curLine].m_start = p1;
      lines[curLine].m_end = p2;
      ++curLine;
    }
  }

  EZ_ASSERT_DEV(curLine <= NUM_LINES, "");
  ezDebugRenderer::DrawLines(GetWorld(), ezMakeArrayPtr(lines, curLine), m_Color, GetOwner()->GetGlobalTransform());
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_IMPLEMENT_WORLD_MODULE(ezSensorWorldModule);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSensorWorldModule, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezSensorWorldModule::ezSensorWorldModule(ezWorld* pWorld)
  : ezWorldModule(pWorld)
{
}

void ezSensorWorldModule::Initialize()
{
  SUPER::Initialize();

  {
    auto updateDesc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezSensorWorldModule::UpdateSensors, this);
    updateDesc.m_Phase = ezWorldModule::UpdateFunctionDesc::Phase::Async;
    updateDesc.m_bOnlyUpdateWhenSimulating = true;

    RegisterUpdateFunction(updateDesc);
  }

  {
    auto updateDesc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezSensorWorldModule::DebugDrawSensors, this);
    updateDesc.m_Phase = ezWorldModule::UpdateFunctionDesc::Phase::PostTransform;

    RegisterUpdateFunction(updateDesc);
  }

  m_pPhysicsWorldModule = GetWorld()->GetOrCreateModule<ezPhysicsWorldModuleInterface>();
}

void ezSensorWorldModule::AddComponentToSchedule(ezSensorComponent* pComponent, ezUpdateRate::Enum updateRate)
{
  m_Scheduler.AddOrUpdateWork(pComponent->GetHandle(), ezUpdateRate::GetInterval(updateRate));
}

void ezSensorWorldModule::RemoveComponentToSchedule(ezSensorComponent* pComponent)
{
  m_Scheduler.RemoveWork(pComponent->GetHandle());
}

void ezSensorWorldModule::AddComponentForDebugRendering(ezSensorComponent* pComponent)
{
  ezComponentHandle hComponent = pComponent->GetHandle();
  if (m_DebugComponents.Contains(hComponent) == false)
  {
    m_DebugComponents.PushBack(hComponent);
  }
}

void ezSensorWorldModule::RemoveComponentForDebugRendering(ezSensorComponent* pComponent)
{
  m_DebugComponents.RemoveAndSwap(pComponent->GetHandle());
}

void ezSensorWorldModule::UpdateSensors(const ezWorldModule::UpdateContext& context)
{
  if (m_pPhysicsWorldModule == nullptr)
    return;

  const ezTime deltaTime = GetWorld()->GetClock().GetTimeDiff();
  m_Scheduler.Update(deltaTime, [this](const ezComponentHandle& hComponent, ezTime deltaTime) {
    const ezWorld* pWorld = GetWorld();
    const ezSensorComponent* pSensorComponent = nullptr;
    EZ_VERIFY(pWorld->TryGetComponent(hComponent, pSensorComponent), "Invalid component handle");

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
    pSensorComponent->m_LastOccludedObjectPositions.Clear();
#endif

    m_ObjectsInSensorVolume.Clear();

    pSensorComponent->GetObjectsInSensorVolume(m_ObjectsInSensorVolume);
    const ezGameObject* pSensorOwner = pSensorComponent->GetOwner();

    m_DetectedObjects.Clear();

    if (pSensorComponent->m_bTestVisibility)
    {
      const ezVec3 rayStart = pSensorOwner->GetGlobalPosition();
      for (auto pObject : m_ObjectsInSensorVolume)
      {
        const ezVec3 rayEnd = pObject->GetGlobalPosition();
        ezVec3 rayDir = rayEnd - rayStart;
        const float fDistance = rayDir.GetLengthAndNormalize();

        ezPhysicsCastResult hitResult;
        ezPhysicsQueryParameters params(pSensorComponent->m_uiCollisionLayer);
        params.m_bIgnoreInitialOverlap = true;
        params.m_ShapeTypes = ezPhysicsShapeType::Default;

        // TODO: probably best to expose the ezPhysicsShapeType bitflags on the component
        params.m_ShapeTypes.Remove(ezPhysicsShapeType::Rope);
        params.m_ShapeTypes.Remove(ezPhysicsShapeType::Ragdoll);
        params.m_ShapeTypes.Remove(ezPhysicsShapeType::Trigger);
        params.m_ShapeTypes.Remove(ezPhysicsShapeType::Query);
        params.m_ShapeTypes.Remove(ezPhysicsShapeType::Character);

        if (m_pPhysicsWorldModule->Raycast(hitResult, rayStart, rayDir, fDistance, params))
        {
          // hit something in between -> not visible
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
          pSensorComponent->m_LastOccludedObjectPositions.PushBack(rayEnd);
#endif

          continue;
        }

        m_DetectedObjects.PushBack(pObject->GetHandle());
      }
    }
    else
    {
      for (auto pObject : m_ObjectsInSensorVolume)
      {
        m_DetectedObjects.PushBack(pObject->GetHandle());
      }
    }

    m_DetectedObjects.Sort();
    if (m_DetectedObjects != pSensorComponent->m_LastDetectedObjects)
    {
      m_DetectedObjects.Swap(pSensorComponent->m_LastDetectedObjects);

      ezMsgSensorDetectedObjectsChanged msg;
      msg.m_DetectedObjects = pSensorComponent->m_LastDetectedObjects;

      pSensorOwner->PostEventMessage(msg, pSensorComponent, ezTime::Zero(), ezObjectMsgQueueType::PostAsync);
    } });
}

void ezSensorWorldModule::DebugDrawSensors(const ezWorldModule::UpdateContext& context)
{
  ezHybridArray<ezDebugRenderer::Line, 256> lines;
  const ezWorld* pWorld = GetWorld();

  for (ezComponentHandle hComponent : m_DebugComponents)
  {
    lines.Clear();

    const ezSensorComponent* pSensorComponent = nullptr;
    EZ_VERIFY(pWorld->TryGetComponent(hComponent, pSensorComponent), "Invalid component handle");

    pSensorComponent->DebugDrawSensorShape();

    const ezVec3 sensorPos = pSensorComponent->GetOwner()->GetGlobalPosition();
    for (ezGameObjectHandle hObject : pSensorComponent->m_LastDetectedObjects)
    {
      const ezGameObject* pObject = nullptr;
      if (pWorld->TryGetObject(hObject, pObject) == false)
        continue;

      lines.PushBack({sensorPos, pObject->GetGlobalPosition(), ezColor::Lime});
    }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
    for (const ezVec3& occludedPos : pSensorComponent->m_LastOccludedObjectPositions)
    {
      lines.PushBack({sensorPos, occludedPos, ezColor::Red});
    }
#endif

    ezDebugRenderer::DrawLines(pWorld, lines, ezColor::White);
  }
}


EZ_STATICLINK_FILE(GameEngine, GameEngine_AI_Implementation_SensorComponent);
