#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <RmlUiPlugin/Components/RmlUiCanvas3DInteractionExampleComponent.h>
#include <RmlUiPlugin/Components/RmlUiCanvas3DComponent.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Messages/CommonMessages.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <Core/Input/InputManager.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezRmlUiCanvas3DInteractionExampleComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
    EZ_MEMBER_PROPERTY("MaxDistance", m_fMaxDistance)->AddAttributes(new ezDefaultValueAttribute(2.0f)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Input/RmlUi"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on


ezRmlUiCanvas3DInteractionExampleComponent::ezRmlUiCanvas3DInteractionExampleComponent() = default;
ezRmlUiCanvas3DInteractionExampleComponent::~ezRmlUiCanvas3DInteractionExampleComponent() = default;

void ezRmlUiCanvas3DInteractionExampleComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_uiCollisionLayer;
  s << m_fMaxDistance;
}

void ezRmlUiCanvas3DInteractionExampleComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_uiCollisionLayer;
  s >> m_fMaxDistance;
}

void ezRmlUiCanvas3DInteractionExampleComponent::OnSimulationStarted()
{
  m_pPhysicsWorldModule = GetWorld()->GetOrCreateModule<ezPhysicsWorldModuleInterface>();
}

void ezRmlUiCanvas3DInteractionExampleComponent::Interact(ezRmlUiInputSnapshot input)
{
  if (!m_pPhysicsWorldModule)
  {
    return;
  }

  ezVec3 vRayOrigin = GetOwner()->GetGlobalPosition();
  ezVec3 vRayDir = GetOwner()->GetGlobalDirForwards().GetNormalized();

  ezPhysicsCastResult hit;

  ezPhysicsQueryParameters queryParams{m_uiCollisionLayer};
  queryParams.m_bIgnoreInitialOverlap = true;
  queryParams.m_ShapeTypes = ezPhysicsShapeType::Static | ezPhysicsShapeType::Dynamic | ezPhysicsShapeType::Trigger;

  if (!m_pPhysicsWorldModule->Raycast(hit, vRayOrigin, vRayDir, m_fMaxDistance, queryParams))
  {
    return;
  }
  
  ezGameObject* pGameObject = nullptr;
  if (!GetWorld()->TryGetObject(hit.m_hShapeObject, pGameObject))
  {
    ezLog::Dev("ezRmlUiCanvas3DInteractionComponent: failed to acquire a game object from raycast result");
    return;
  }

  ezRmlUiCanvas3DComponent* pCanvas = nullptr;
  if (!pGameObject->TryGetComponentOfBaseType(pCanvas))
  {
    return;
  }

  pCanvas->RaycastInput(vRayOrigin, vRayDir, input);
}

void ezRmlUiCanvas3DInteractionExampleComponent::Update()
{
  ezRmlUiInputSnapshot input{};
  if (ezInputManager::GetInputSlotState(ezInputSlot_MouseButton1) == ezKeyState::Down)
    input.m_Buttons |= ezRmlUiInputButtons::Mouse0;
  if (ezInputManager::GetInputSlotState(ezInputSlot_MouseWheelUp) == ezKeyState::Pressed)
    input.m_Buttons |= ezRmlUiInputButtons::MouseWheelUp;
  if (ezInputManager::GetInputSlotState(ezInputSlot_MouseWheelDown) == ezKeyState::Pressed)
    input.m_Buttons |= ezRmlUiInputButtons::MouseWheelDown;
  input.m_uiLastCharacter = ezInputManager::RetrieveLastCharacter(false);

  Interact(input);
}

EZ_STATICLINK_FILE(RmlUiPlugin, RmlUiPlugin_Components_Implementation_RmlUiCanvas3DInteractionComponent);
