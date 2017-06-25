#include <PCH.h>
#include <RecastPlugin/Components/SoldierComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <GameEngine/Interfaces/PhysicsWorldModule.h>

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_COMPONENT_TYPE(ezSoldierComponent, 1)
{
  //EZ_BEGIN_PROPERTIES
  //{
  //}
  //EZ_END_PROPERTIES
}
EZ_END_COMPONENT_TYPE

ezSoldierComponent::ezSoldierComponent() { }
ezSoldierComponent::~ezSoldierComponent() { }

void ezSoldierComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  ezStreamWriter& s = stream.GetStream();

}

void ezSoldierComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  //const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

}


void ezSoldierComponent::OnSimulationStarted()
{
  //ezCharacterControllerComponent* pCC = nullptr;
  //if (GetOwner()->TryGetComponentOfBaseType<ezCharacterControllerComponent>(pCC))
  //{
  //  m_hCharacterController = pCC->GetHandle();
  //}
}

void ezSoldierComponent::Update()
{
  if (!IsActiveAndSimulating())
    return;

}

