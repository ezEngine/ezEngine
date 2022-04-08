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
EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezJoltActorComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
    EZ_ACCESSOR_PROPERTY("Surface", GetSurfaceFile, SetSurfaceFile)->AddAttributes(new ezAssetBrowserAttribute("Surface")),
  }
  EZ_END_PROPERTIES;
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

void ezJoltActorComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_uiCollisionLayer;
  s << m_hSurface;
}


void ezJoltActorComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_uiCollisionLayer;
  s >> m_hSurface;
}

void ezJoltActorComponent::OnDeactivated()
{
  if (m_uiUserDataIndex != ezInvalidIndex)
  {
    ezJoltWorldModule* pModule = GetWorld()->GetModule<ezJoltWorldModule>();

    pModule->DeallocateUserData(m_uiUserDataIndex);
  }

  SUPER::OnDeactivated();
}

void ezJoltActorComponent::SetSurfaceFile(const char* szFile)
{
  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    m_hSurface = ezResourceManager::LoadResource<ezSurfaceResource>(szFile);
  }

  if (m_hSurface.IsValid())
    ezResourceManager::PreloadResource(m_hSurface);
}

const char* ezJoltActorComponent::GetSurfaceFile() const
{
  if (!m_hSurface.IsValid())
    return "";

  return m_hSurface.GetResourceID();
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

ezResult ezJoltActorComponent::CreateShape(JPH::BodyCreationSettings* pSettings, float fDensity)
{
  ezHybridArray<ezJoltSubShape, 16> shapes;
  ezTransform towner = GetOwner()->GetGlobalTransform();
  towner.m_vScale.Set(1.0f); // pretend like there is no scaling at the root, so that each shape applies its scale

  CreateShapes(shapes, towner, fDensity, GetJoltMaterial());
  GatherShapes(shapes, GetOwner(), towner, fDensity, GetJoltMaterial());

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

    for (auto shape : shapes)
    {
      auto pShape = shape.m_pShape;

      if (!shape.m_Transform.m_vScale.IsEqual(ezVec3(1.0f), 0.01f))
      {
        auto* pScaledShape = new JPH::ScaledShape(pShape, ezJoltConversionUtils::ToVec3(shape.m_Transform.m_vScale));
        pShape = pScaledShape;
      }

      opt.AddShape(ezJoltConversionUtils::ToVec3(shape.m_Transform.m_vPosition), ezJoltConversionUtils::ToQuat(shape.m_Transform.m_qRotation), pShape);
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

    if (!shapes[0].m_Transform.m_vPosition.IsZero(0.01f) || shapes[0].m_Transform.m_qRotation != ezQuat::IdentityQuaternion())
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

const ezJoltMaterial* ezJoltActorComponent::GetJoltMaterial() const
{
  if (m_hSurface.IsValid())
  {
    ezResourceLock<ezSurfaceResource> pSurface(m_hSurface, ezResourceAcquireMode::BlockTillLoaded);

    if (pSurface->m_pPhysicsMaterialJolt != nullptr)
    {
      return static_cast<ezJoltMaterial*>(pSurface->m_pPhysicsMaterialJolt);
    }
  }

  return ezJoltCore::GetDefaultMaterial();
}
