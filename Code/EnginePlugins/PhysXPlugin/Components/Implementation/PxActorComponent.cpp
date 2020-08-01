#include <PhysXPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <PhysXPlugin/Components/PxActorComponent.h>
#include <PhysXPlugin/Shapes/PxShapeComponent.h>

using namespace physx;

// clang-format off
EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezPxActorComponent, 1)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Physics/Actors"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

ezPxActorComponent::ezPxActorComponent() = default;
ezPxActorComponent::~ezPxActorComponent() = default;

void ezPxActorComponent::AddShapesFromObject(ezGameObject* pObject, PxRigidActor* pRigidActor, const ezSimdTransform& ParentTransform)
{
  ezHybridArray<ezPxShapeComponent*, 8> shapes;
  pObject->TryGetComponentsOfBaseType(shapes);

  for (auto pShape : shapes)
  {
    if (pShape->IsActive())
    {
      pShape->AddToActor(pRigidActor, ParentTransform);
    }
  }

  for (auto itChild = pObject->GetChildren(); itChild.IsValid(); ++itChild)
  {
    // ignore all children that are actors themselves
    ezPxActorComponent* pActorComponent;
    if (itChild->TryGetComponentOfBaseType<ezPxActorComponent>(pActorComponent))
      continue;

    AddShapesFromObject(itChild, pRigidActor, ParentTransform);
  }
}

void ezPxActorComponent::AddShapesToNavMesh(const ezGameObject* pObject, ezMsgExtractGeometry& msg) const
{
  ezHybridArray<const ezPxShapeComponent*, 8> shapes;
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
    const ezPxActorComponent* pActorComponent;
    if (itChild->TryGetComponentOfBaseType<ezPxActorComponent>(pActorComponent))
      continue;

    AddShapesToNavMesh(itChild, msg);
  }
}

EZ_STATICLINK_FILE(PhysXPlugin, PhysXPlugin_Components_Implementation_PxActorComponent);
