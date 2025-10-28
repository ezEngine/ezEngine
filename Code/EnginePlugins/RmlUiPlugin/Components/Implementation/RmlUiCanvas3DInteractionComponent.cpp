#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <RmlUiPlugin/Components/RmlUiCanvas3DInteractionComponent.h>
#include <RmlUiPlugin/Components/RmlUiCanvas3DComponent.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Messages/CommonMessages.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <Core/Input/InputManager.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezRmlUiCanvas3DInteractionComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
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


ezRmlUiCanvas3DInteractionComponent::ezRmlUiCanvas3DInteractionComponent() = default;
ezRmlUiCanvas3DInteractionComponent::~ezRmlUiCanvas3DInteractionComponent() = default;

void ezRmlUiCanvas3DInteractionComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_uiCollisionLayer;
}

void ezRmlUiCanvas3DInteractionComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_uiCollisionLayer;
}

void ezRmlUiCanvas3DInteractionComponent::OnSimulationStarted()
{
  m_pPhysicsWorldModule = GetWorld()->GetOrCreateModule<ezPhysicsWorldModuleInterface>();
}

void ezRmlUiCanvas3DInteractionComponent::Interact(ezRmlUiInputState input, float fMaxDistance)
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

  if (!m_pPhysicsWorldModule->Raycast(hit, vRayOrigin, vRayDir, fMaxDistance, queryParams))
  {
    ezLog::Dev("ezRmlUiCanvas3DInteractionComponent: raycast failed");
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
    ezLog::Dev("ezRmlUiCanvas3DInteractionComponent: raycast result is not a canvas");
    return;
  }

  if (!pCanvas->GetMesh().IsValid())
  {
    ezLog::Dev("ezRmlUiCanvas3DInteractionComponent: a canvas doesn't have a mesh");
    return;
  }

  ezCpuMeshResourceHandle hMesh = ezResourceManager::LoadResource<ezCpuMeshResource>(pCanvas->GetMesh().GetResourceID());
  if (!hMesh.IsValid())
  {
    ezLog::Dev("ezRmlUiCanvas3DInteractionComponent: canvas mesh is not valid");
    return;
  }

  ezResourceLock<ezCpuMeshResource> pMesh(hMesh, ezResourceAcquireMode::AllowLoadingFallback);
  if (pMesh.GetAcquireResult() == ezResourceAcquireResult::LoadingFallback)
  {
    ezLog::Dev("ezRmlUiCanvas3DInteractionComponent: canvas mesh is not loaded yet");
    return;
  }

  ezTransform worldToLocal = pGameObject->GetGlobalTransform().GetInverse();
  ezVec3 vRayOriginMeshSpace = worldToLocal.TransformPosition(vRayOrigin);
  ezVec3 vRayDirMeshSpace = worldToLocal.TransformDirection(vRayDir).GetNormalized();

  ezVec2 vTexCoords;
  if (!RaycastMeshTexCoords(pMesh.GetPointer(), vRayOriginMeshSpace, vRayDirMeshSpace, vTexCoords))
  {
    ezLog::Dev("ezRmlUiCanvas3DInteractionComponent: raycast failed to hit any triangles");
    return;
  }

  // flip tex coords because we do the same thing in the shader TODO: why?
  input.m_vCursorPos.x = static_cast<float>(pCanvas->GetSize().x) * (1.0f - vTexCoords.x);
  input.m_vCursorPos.y = static_cast<float>(pCanvas->GetSize().y) * (1.0f - vTexCoords.y);

  ezLog::Dev("ezRmlUiCanvas3DInteractionComponent: canvas was hit at {}", input.m_vCursorPos);

  pCanvas->ApplyInput(input);
}

void ezRmlUiCanvas3DInteractionComponent::Update()
{
  // TODO: this is just for testing, delete later
  ezRmlUiInputState input;
  ezKeyState::Enum state = ezInputManager::GetInputSlotState(ezInputSlot_MouseButton1);
  if (state == ezKeyState::Pressed)
  {
    input.m_uiMouseButton0Pressed = 1;
    Interact(input, 2.0f);
  }
}

bool ezRmlUiCanvas3DInteractionComponent::RaycastMeshTexCoords(const ezCpuMeshResource* pMesh, const ezVec3& vRayOrigin, const ezVec3& vRayDir, ezVec2& out_vTexCoords, float FEpsilon)
{
  const ezMeshBufferResourceDescriptor& mesh = pMesh->GetDescriptor().MeshBufferDesc();

  if (mesh.GetTopology() != ezGALPrimitiveTopology::Triangles)
  {
    ezLog::Dev("RaycastMeshTexCoords: topology {} not supported", mesh.GetTopology());
    return false;
  }

  const ezUInt16* pIndexBuffer = reinterpret_cast<const ezUInt16*>(mesh.GetIndexBufferData().GetPtr());
  ezUInt32 uiNumIndices = mesh.GetIndexBufferData().GetCount() / 2;
  EZ_ASSERT_DEV(mesh.Uses32BitIndices() == false, "not implemented yet");

  for (ezUInt32 uiIndex = 0; uiIndex + 2 < uiNumIndices; ++uiIndex)
  {
    // perform ray-triangle intersection test as described in https://www.graphics.cornell.edu/pubs/1997/MT97.pdf

    ezUInt16 i0 = pIndexBuffer[uiIndex];
    ezUInt16 i1 = pIndexBuffer[uiIndex + 1];
    ezUInt16 i2 = pIndexBuffer[uiIndex + 2];

    ezVec3 v0 = mesh.GetPosition(i0);
    ezVec3 v1 = mesh.GetPosition(i1);
    ezVec3 v2 = mesh.GetPosition(i2);

    ezVec3 edge1 = v1 - v0;
    ezVec3 edge2 = v2 - v0;

    ezVec3 pvec = vRayDir.CrossRH(edge2);

    float det = edge1.Dot(pvec);
    if (det < FEpsilon)
      continue;

    ezVec3 tvec = vRayOrigin - v0;

    float u = tvec.Dot(pvec);
    if (u < 0 || u > det)
      continue;

    ezVec3 qvec = tvec.CrossRH(edge1);

    float v = qvec.Dot(vRayDir);
    if (v < 0 || u + v > det)
      continue;

    float t = edge2.Dot(qvec);
    float inv_det = 1.0f / det;
    t *= inv_det;
    u *= inv_det;
    v *= inv_det;

    out_vTexCoords = ezVec2::MakeZero();
    out_vTexCoords += mesh.GetTexCoord0(i0) * (1.0f - u - v);
    out_vTexCoords += mesh.GetTexCoord0(i1) * u;
    out_vTexCoords += mesh.GetTexCoord0(i2) * v;

    ezLog::Dev("RaycastMeshTexCoords - success {}", out_vTexCoords);

    return true;
  }

  return false;
}


EZ_STATICLINK_FILE(RmlUiPlugin, RmlUiPlugin_Components_Implementation_RmlUiCanvas3DInteractionComponent);
