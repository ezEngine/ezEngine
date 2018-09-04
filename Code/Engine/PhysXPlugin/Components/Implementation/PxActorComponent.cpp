#include <PCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <PhysXPlugin/Components/PxActorComponent.h>
#include <PhysXPlugin/Shapes/PxShapeComponent.h>

using namespace physx;

EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezPxActorComponent, 1)
// EZ_BEGIN_PROPERTIES
// EZ_END_PROPERTIES;
EZ_END_ABSTRACT_COMPONENT_TYPE

ezPxActorComponent::ezPxActorComponent() {}

void ezPxActorComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
}

void ezPxActorComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
}

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
  ezHybridArray<ezPxShapeComponent*, 8> shapes;
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
    ezPxActorComponent* pActorComponent;
    if (itChild->TryGetComponentOfBaseType<ezPxActorComponent>(pActorComponent))
      continue;

    AddShapesToNavMesh(itChild, msg);
  }
}

EZ_STATICLINK_FILE(PhysXPlugin, PhysXPlugin_Components_Implementation_PxActorComponent);
