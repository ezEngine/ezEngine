#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Physics/SurfaceResource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <JoltPlugin/Actors/JoltActorComponent.h>
#include <JoltPlugin/Shapes/JoltShapeComponent.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>

// clang-format off
EZ_IMPLEMENT_MESSAGE_TYPE(ezJoltMsgDisconnectConstraints);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezJoltMsgDisconnectConstraints, 1, ezRTTIDefaultAllocator<ezJoltMsgDisconnectConstraints>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

// clang-format off
EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezJoltActorComponent, 2)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(GetObjectFilterID),
  }
  EZ_END_FUNCTIONS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Physics/Jolt/Actors"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

ezJoltActorComponent::ezJoltActorComponent() = default;
ezJoltActorComponent::~ezJoltActorComponent() = default;

void ezJoltActorComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_uiCollisionLayer;
}

void ezJoltActorComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_uiCollisionLayer;
}

void ezJoltActorComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (m_uiObjectFilterID == ezInvalidIndex)
  {
    // only create a new filter ID, if none has been passed in manually

    ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();
    m_uiObjectFilterID = pModule->CreateObjectFilterID();
  }
}

void ezJoltActorComponent::OnDeactivated()
{
  ezJoltWorldModule* pModule = GetWorld()->GetModule<ezJoltWorldModule>();

  JPH::BodyID bodyId(m_uiJoltBodyID);

  if (!bodyId.IsInvalid())
  {
    auto* pSystem = pModule->GetJoltSystem();
    auto* pBodies = &pSystem->GetBodyInterface();

    if (pBodies->IsAdded(bodyId))
    {
      pBodies->RemoveBody(bodyId);
    }

    pBodies->DestroyBody(bodyId);
    m_uiJoltBodyID = JPH::BodyID::cInvalidBodyID;
  }

  pModule->DeallocateUserData(m_uiUserDataIndex);
  pModule->DeleteObjectFilterID(m_uiObjectFilterID);

  SUPER::OnDeactivated();
}

void ezJoltActorComponent::GatherShapes(ezDynamicArray<ezJoltSubShape>& shapes, ezGameObject* pObject, const ezTransform& rootTransform, float fDensity, const ezJoltMaterial* pMaterial)
{
  ezHybridArray<ezJoltShapeComponent*, 8> shapeComps;
  pObject->TryGetComponentsOfBaseType(shapeComps);

  for (auto pShape : shapeComps)
  {
    if (pShape->IsActive())
    {
      pShape->CreateShapes(shapes, rootTransform, fDensity, pMaterial);
    }
  }

  for (auto itChild = pObject->GetChildren(); itChild.IsValid(); ++itChild)
  {
    // ignore all children that are actors themselves
    const ezJoltActorComponent* pActorComponent;
    if (itChild->TryGetComponentOfBaseType<ezJoltActorComponent>(pActorComponent))
      continue;

    GatherShapes(shapes, itChild, rootTransform, fDensity, pMaterial);
  }
}

ezResult ezJoltActorComponent::CreateShape(JPH::BodyCreationSettings* pSettings, float fDensity, const ezJoltMaterial* pMaterial)
{
  ezHybridArray<ezJoltSubShape, 16> shapes;
  ezTransform towner = GetOwner()->GetGlobalTransform();
  towner.m_vScale.Set(1.0f); // pretend like there is no scaling at the root, so that each shape applies its scale

  CreateShapes(shapes, towner, fDensity, pMaterial);
  GatherShapes(shapes, GetOwner(), towner, fDensity, pMaterial);

  auto cleanShapes = [&]()
  {
    for (auto& s : shapes)
    {
      if (s.m_pShape)
      {
        s.m_pShape->Release();
      }
    }
  };

  EZ_SCOPE_EXIT(cleanShapes());

  if (shapes.IsEmpty())
    return EZ_FAILURE;

  if (shapes.GetCount() > 0)
  {
    JPH::StaticCompoundShapeSettings opt;

    for (const auto& shape : shapes)
    {
      auto pShape = shape.m_pShape;

      if (!shape.m_Transform.m_vScale.IsEqual(ezVec3(1.0f), 0.01f))
      {
        auto* pScaledShape = new JPH::ScaledShape(pShape, ezJoltConversionUtils::ToVec3(shape.m_Transform.m_vScale));
        pShape = pScaledShape;
      }

      opt.AddShape(ezJoltConversionUtils::ToVec3(shape.m_Transform.m_vPosition), ezJoltConversionUtils::ToQuat(shape.m_Transform.m_qRotation).Normalized(), pShape);
    }

    auto res = opt.Create();
    if (!res.IsValid())
      return EZ_FAILURE;

    pSettings->SetShape(res.Get());
    return EZ_SUCCESS;
  }
  else
  {
    JPH::Shape* pShape = shapes[0].m_pShape;

    if (!shapes[0].m_Transform.m_vScale.IsEqual(ezVec3(1.0f), 0.01f))
    {
      auto* pScaledShape = new JPH::ScaledShape(pShape, ezJoltConversionUtils::ToVec3(shapes[0].m_Transform.m_vScale));
      pShape = pScaledShape;
    }

    if (!shapes[0].m_Transform.m_vPosition.IsZero(0.01f) || shapes[0].m_Transform.m_qRotation != ezQuat::MakeIdentity())
    {
      JPH::RotatedTranslatedShapeSettings opt(ezJoltConversionUtils::ToVec3(shapes[0].m_Transform.m_vPosition), ezJoltConversionUtils::ToQuat(shapes[0].m_Transform.m_qRotation), pShape);

      auto res = opt.Create();
      if (!res.IsValid())
        return EZ_FAILURE;

      pShape = res.Get();
    }

    pSettings->SetShape(pShape);
    return EZ_SUCCESS;
  }
}

void ezJoltActorComponent::ExtractSubShapeGeometry(const ezGameObject* pObject, ezMsgExtractGeometry& msg) const
{
  ezHybridArray<const ezJoltShapeComponent*, 8> shapes;
  pObject->TryGetComponentsOfBaseType(shapes);

  for (auto pShape : shapes)
  {
    if (pShape->IsActive())
    {
      pShape->ExtractGeometry(msg);
    }
  }

  for (auto itChild = pObject->GetChildren(); itChild.IsValid(); ++itChild)
  {
    // ignore all children that are actors themselves
    const ezJoltActorComponent* pActorComponent;
    if (itChild->TryGetComponentOfBaseType<ezJoltActorComponent>(pActorComponent))
      continue;

    ExtractSubShapeGeometry(itChild, msg);
  }
}

const ezJoltUserData* ezJoltActorComponent::GetUserData() const
{
  const ezJoltWorldModule* pModule = GetWorld()->GetModule<ezJoltWorldModule>();

  return &pModule->GetUserData(m_uiUserDataIndex);
}

void ezJoltActorComponent::SetInitialObjectFilterID(ezUInt32 uiObjectFilterID)
{
  EZ_ASSERT_DEBUG(!IsActiveAndSimulating(), "The object filter ID can't be changed after simulation has started.");
  m_uiObjectFilterID = uiObjectFilterID;
}


EZ_STATICLINK_FILE(JoltPlugin, JoltPlugin_Actors_Implementation_JoltActorComponent);
