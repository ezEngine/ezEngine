#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Profiling/Profiling.h>
#include <GameEngine/XR/ControllerRenderModelComponent.h>
#include <GameEngine/XR/StageSpaceComponent.h>
#include <GameEngine/XR/XRInterface.h>
#include <RendererCore/Meshes/MeshComponent.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezControllerRenderModelComponent, 1, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_ACCESSOR_PROPERTY("DeviceType", ezXRDeviceType, GetDeviceType, SetDeviceType)->AddAttributes(new ezDefaultValueAttribute((ezInt32)ezXRDeviceType::LeftController)),
    EZ_RESOURCE_ACCESSOR_PROPERTY("FallbackMaterial", GetFallbackMaterial, SetFallbackMaterial)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Material")),
    EZ_ACCESSOR_PROPERTY("HideWhenNotTracked", GetHideWhenNotTracked, SetHideWhenNotTracked)->AddAttributes(new ezDefaultValueAttribute(true)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("XR"),
    new ezInDevelopmentAttribute(ezInDevelopmentAttribute::Phase::Alpha),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezControllerRenderModelComponent::ezControllerRenderModelComponent() = default;
ezControllerRenderModelComponent::~ezControllerRenderModelComponent() = default;

void ezControllerRenderModelComponent::SetDeviceType(ezEnum<ezXRDeviceType> type)
{
  if (m_DeviceType != type)
  {
    m_DeviceType = type;
    if (IsActiveAndInitialized())
    {
      UnloadModel();
      LoadModel();
    }
  }
}

ezEnum<ezXRDeviceType> ezControllerRenderModelComponent::GetDeviceType() const
{
  return m_DeviceType;
}

void ezControllerRenderModelComponent::SetFallbackMaterial(ezMaterialResourceHandle hMaterial)
{
  m_hFallbackMaterial = hMaterial;
}

ezMaterialResourceHandle ezControllerRenderModelComponent::GetFallbackMaterial() const
{
  return m_hFallbackMaterial;
}

void ezControllerRenderModelComponent::SetHideWhenNotTracked(bool bHide)
{
  m_bHideWhenNotTracked = bHide;
}

bool ezControllerRenderModelComponent::GetHideWhenNotTracked() const
{
  return m_bHideWhenNotTracked;
}

void ezControllerRenderModelComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  ezStreamWriter& s = stream.GetStream();

  s << m_DeviceType;
  s << m_hFallbackMaterial;
  s << m_bHideWhenNotTracked;
}

void ezControllerRenderModelComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

  s >> m_DeviceType;
  s >> m_hFallbackMaterial;
  s >> m_bHideWhenNotTracked;
}

void ezControllerRenderModelComponent::OnActivated()
{
  SUPER::OnActivated();
  LoadModel();
}

void ezControllerRenderModelComponent::OnDeactivated()
{
  UnloadModel();
  SUPER::OnDeactivated();
}

void ezControllerRenderModelComponent::LoadModel()
{
  // Get the render model interface
  m_pRenderModelInterface = ezSingletonRegistry::GetSingletonInstance<ezXRRenderModelInterface>();
  if (m_pRenderModelInterface == nullptr || !m_pRenderModelInterface->IsSupported())
  {
    ezLog::Debug("XR render models not available");
    return;
  }

  // Enumerate available models
  ezDynamicArray<ezXRRenderModelInfo> models;
  if (!m_pRenderModelInterface->EnumerateInteractionRenderModels(models))
  {
    ezLog::Warning("Failed to enumerate XR render models");
    return;
  }

  if (models.IsEmpty())
  {
    ezLog::Debug("No XR render models available");
    return;
  }

  // For now, use the first available model for each controller
  // TODO: Match model to specific controller type
  ezXRRenderModelHandle handleToLoad = ezInvalidXRRenderModelHandle;

  // Simple heuristic: use first model for left, second for right (if available)
  if (m_DeviceType == ezXRDeviceType::LeftController && models.GetCount() >= 1)
  {
    handleToLoad = models[0].m_Handle;
  }
  else if (m_DeviceType == ezXRDeviceType::RightController && models.GetCount() >= 2)
  {
    handleToLoad = models[1].m_Handle;
  }
  else if (!models.IsEmpty())
  {
    // Fallback to first model
    handleToLoad = models[0].m_Handle;
  }

  if (handleToLoad == ezInvalidXRRenderModelHandle)
  {
    ezLog::Warning("No suitable XR render model found for device type {}", (int)m_DeviceType.GetValue());
    return;
  }

  // Load the model
  if (!m_pRenderModelInterface->LoadRenderModel(handleToLoad))
  {
    ezLog::Warning("Failed to load XR render model");
    return;
  }

  m_ModelHandle = handleToLoad;
  m_bModelLoaded = true;

  // Create the node hierarchy
  CreateNodeHierarchy();
}

void ezControllerRenderModelComponent::UnloadModel()
{
  // Destroy all created child objects
  for (auto& node : m_Nodes)
  {
    if (!node.m_hGameObject.IsInvalidated())
    {
      GetWorld()->DeleteObjectDelayed(node.m_hGameObject);
    }
  }
  m_Nodes.Clear();

  // Unload from the interface
  if (m_pRenderModelInterface != nullptr && m_ModelHandle != ezInvalidXRRenderModelHandle)
  {
    m_pRenderModelInterface->UnloadRenderModel(m_ModelHandle);
  }

  m_ModelHandle = ezInvalidXRRenderModelHandle;
  m_bModelLoaded = false;
  m_bHierarchyCreated = false;
  m_pRenderModelInterface = nullptr;
}

void ezControllerRenderModelComponent::CreateNodeHierarchy()
{
  if (!m_bModelLoaded || m_pRenderModelInterface == nullptr)
    return;

  // Wait for model to be fully loaded
  if (!m_pRenderModelInterface->IsRenderModelLoaded(m_ModelHandle))
    return;

  // Get nodes
  ezDynamicArray<ezXRRenderModelNode> nodes;
  if (!m_pRenderModelInterface->GetRenderModelNodes(m_ModelHandle, nodes))
  {
    ezLog::Warning("Failed to get render model nodes");
    return;
  }

  // Get meshes
  ezDynamicArray<ezXRRenderModelMesh> meshes;
  if (!m_pRenderModelInterface->GetRenderModelMeshes(m_ModelHandle, meshes))
  {
    ezLog::Warning("Failed to get render model meshes");
    return;
  }

  if (nodes.IsEmpty())
  {
    ezLog::Debug("Render model has no nodes");
    return;
  }

  m_Nodes.SetCount(nodes.GetCount());

  // Create game objects for each node
  for (ezUInt32 i = 0; i < nodes.GetCount(); ++i)
  {
    const ezXRRenderModelNode& srcNode = nodes[i];

    ezGameObjectDesc desc;
    desc.m_sName.Assign(srcNode.m_sName.GetData());
    desc.m_LocalPosition = srcNode.m_LocalTransform.m_vPosition;
    desc.m_LocalRotation = srcNode.m_LocalTransform.m_qRotation;
    desc.m_LocalScaling = srcNode.m_LocalTransform.m_vScale;

    // Set parent
    if (srcNode.m_iParentIndex >= 0 && (ezUInt32)srcNode.m_iParentIndex < i)
    {
      desc.m_hParent = m_Nodes[srcNode.m_iParentIndex].m_hGameObject;
    }
    else
    {
      desc.m_hParent = GetOwner()->GetHandle();
    }

    ezGameObject* pNodeObject;
    m_Nodes[i].m_hGameObject = GetWorld()->CreateObject(desc, pNodeObject);
    m_Nodes[i].m_iParentIndex = srcNode.m_iParentIndex;
    m_Nodes[i].m_bIsAnimatable = srcNode.m_bIsAnimatable;
    m_Nodes[i].m_uiAnimatableIndex = srcNode.m_uiAnimatableIndex;
  }

  // Create mesh components for each mesh
  for (const auto& mesh : meshes)
  {
    if (mesh.m_uiNodeIndex >= m_Nodes.GetCount())
      continue;

    ezGameObject* pNodeObject;
    if (!GetWorld()->TryGetObject(m_Nodes[mesh.m_uiNodeIndex].m_hGameObject, pNodeObject))
      continue;

    // Create mesh resource from the mesh data
    if (mesh.m_Positions.IsEmpty() || mesh.m_Indices.IsEmpty())
      continue;

    // Build mesh resource descriptor
    ezMeshResourceDescriptor meshDesc;
    auto& mbDesc = meshDesc.MeshBufferDesc();

    mbDesc.AddCommonStreams();
    mbDesc.AllocateStreams(mesh.m_Positions.GetCount(), ezGALPrimitiveTopology::Triangles, mesh.m_Indices.GetCount() / 3);

    // Copy position data
    ezArrayPtr<ezVec3> positions = mbDesc.GetPositionData();
    positions.CopyFrom(mesh.m_Positions);

    // Copy normal data if available
    if (!mesh.m_Normals.IsEmpty())
    {
      ezUInt32 uiStride = 0;
      ezArrayPtr<const ezUInt8> normalData = mbDesc.GetNormalData(&uiStride);
      // Note: Normals need to be encoded using ezMeshBufferUtils::EncodeNormal
      // For now we skip this - full implementation would handle normal encoding
    }

    // Copy index data
    ezArrayPtr<ezUInt8> indexData = mbDesc.GetIndexBufferData().GetArrayPtr();
    if (mbDesc.GetIndexBufferData().GetCount() >= mesh.m_Indices.GetCount() * sizeof(ezUInt32))
    {
      ezMemoryUtils::Copy(reinterpret_cast<ezUInt32*>(indexData.GetPtr()), mesh.m_Indices.GetData(), mesh.m_Indices.GetCount());
    }

    meshDesc.AddSubMesh(mesh.m_Indices.GetCount() / 3, 0, 0);
    meshDesc.ComputeBounds();

    // Create unique resource name
    ezStringBuilder sResourceName;
    sResourceName.SetFormat("XRControllerMesh_{}_{}", m_ModelHandle, mesh.m_uiNodeIndex);

    ezMeshResourceHandle hMesh = ezResourceManager::CreateResource<ezMeshResource>(sResourceName, std::move(meshDesc));

    // Create mesh component
    ezMeshComponent* pMeshComponent;
    ezMeshComponent::CreateComponent(pNodeObject, pMeshComponent);
    pMeshComponent->SetMesh(hMesh);

    if (m_hFallbackMaterial.IsValid())
    {
      pMeshComponent->SetMaterial(0, m_hFallbackMaterial);
    }
  }

  m_bHierarchyCreated = true;
  ezLog::Info("Created XR controller model hierarchy with {} nodes", nodes.GetCount());
}

void ezControllerRenderModelComponent::UpdateNodeTransforms()
{
  if (!m_bHierarchyCreated || m_pRenderModelInterface == nullptr)
    return;

  ezDynamicArray<ezXRRenderModelNodeState> states;
  if (!m_pRenderModelInterface->GetRenderModelState(m_ModelHandle, states))
    return;

  // Update transforms of animatable nodes
  for (auto& node : m_Nodes)
  {
    if (!node.m_bIsAnimatable)
      continue;

    if (node.m_uiAnimatableIndex >= states.GetCount())
      continue;

    ezGameObject* pNodeObject;
    if (!GetWorld()->TryGetObject(node.m_hGameObject, pNodeObject))
      continue;

    const auto& state = states[node.m_uiAnimatableIndex];

    // The state transform replaces (not composes with) the local transform
    pNodeObject->SetLocalPosition(state.m_Transform.m_vPosition);
    pNodeObject->SetLocalRotation(state.m_Transform.m_qRotation);
    pNodeObject->SetLocalScaling(state.m_Transform.m_vScale);

    // Update visibility
    pNodeObject->SetActiveFlag(state.m_bIsVisible);
  }
}

void ezControllerRenderModelComponent::SetVisibility(bool bVisible)
{
  if (m_bCurrentlyVisible == bVisible)
    return;

  m_bCurrentlyVisible = bVisible;

  for (auto& node : m_Nodes)
  {
    ezGameObject* pNodeObject;
    if (GetWorld()->TryGetObject(node.m_hGameObject, pNodeObject))
    {
      pNodeObject->SetActiveFlag(bVisible);
    }
  }
}

void ezControllerRenderModelComponent::Update()
{
  EZ_PROFILE_SCOPE("ControllerRenderModel::Update");

  if (!IsActiveAndSimulating())
    return;

  // Try to create hierarchy if not done yet (model loading may be async)
  if (m_bModelLoaded && !m_bHierarchyCreated)
  {
    CreateNodeHierarchy();
  }

  if (!m_bHierarchyCreated)
    return;

  // Check if controller is tracked
  if (m_bHideWhenNotTracked)
  {
    if (ezXRInterface* pXRInterface = ezSingletonRegistry::GetSingletonInstance<ezXRInterface>())
    {
      if (pXRInterface->IsInitialized())
      {
        ezXRDeviceID deviceID = pXRInterface->GetXRInput().GetDeviceIDByType(m_DeviceType);
        if (deviceID != -1)
        {
          const ezXRDeviceState& state = pXRInterface->GetXRInput().GetDeviceState(deviceID);
          SetVisibility(state.m_bGripPoseIsValid || state.m_bAimPoseIsValid);
        }
        else
        {
          SetVisibility(false);
        }
      }
    }
  }

  // Update the model's world transform from the interface
  if (m_pRenderModelInterface != nullptr)
  {
    ezTransform modelTransform;
    if (m_pRenderModelInterface->GetRenderModelTransform(m_ModelHandle, modelTransform))
    {
      // Apply stage space transform if needed
      ezTransform add;
      add.SetIdentity();
      if (const ezStageSpaceComponentManager* pStageMan = GetWorld()->GetComponentManager<ezStageSpaceComponentManager>())
      {
        if (const ezStageSpaceComponent* pStage = pStageMan->GetSingletonComponent())
        {
          add = pStage->GetOwner()->GetGlobalTransform();
        }
      }

      const ezTransform globalTransform = add * modelTransform;

      // Convert to local space relative to parent
      ezTransform localTransform;
      if (GetOwner()->GetParent() != nullptr)
      {
        localTransform = ezTransform::MakeLocalTransform(GetOwner()->GetParent()->GetGlobalTransform(), globalTransform);
      }
      else
      {
        localTransform = globalTransform;
      }

      GetOwner()->SetLocalPosition(localTransform.m_vPosition);
      GetOwner()->SetLocalRotation(localTransform.m_qRotation);
    }
  }

  // Update animated node transforms
  UpdateNodeTransforms();
}

EZ_STATICLINK_FILE(GameEngine, GameEngine_XR_Implementation_ControllerRenderModelComponent);

