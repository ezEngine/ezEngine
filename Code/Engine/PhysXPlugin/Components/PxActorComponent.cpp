#include <PhysXPlugin/PCH.h>
#include <PhysXPlugin/Components/PxActorComponent.h>
#include <PhysXPlugin/PhysXSceneModule.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <PhysXPlugin/Shapes/PxShapeComponent.h>

EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezPxActorComponent, 1);
  //EZ_BEGIN_PROPERTIES
  //EZ_END_PROPERTIES
EZ_END_ABSTRACT_COMPONENT_TYPE();

ezPxActorComponent::ezPxActorComponent()
{
}


void ezPxActorComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

}


void ezPxActorComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);



}

void ezPxActorComponent::AddShapesFromObject(ezGameObject* pObject, PxRigidActor* pActor, const ezTransform& ParentTransform)
{
  ezHybridArray<ezPxShapeComponent*, 8> shapes;
  pObject->TryGetComponentsOfBaseType(shapes);

  for (auto pShape : shapes)
  {
    pShape->AddToActor(pActor, ParentTransform);
  }
}

void ezPxActorComponent::AddShapesFromChildren(ezGameObject* pRoot, PxRigidActor* pRigidActor, const ezTransform& ParentTransform)
{
  for (auto itChild = pRoot->GetChildren(); itChild.IsValid(); ++itChild)
  {
    // ignore all children that are actors themselves
    ezPxActorComponent* pActor;
    if (itChild->TryGetComponentOfBaseType<ezPxActorComponent>(pActor))
      continue;

    AddShapesFromObject(itChild, pRigidActor, ParentTransform);
    AddShapesFromChildren(itChild, pRigidActor, ParentTransform);
  }
}

