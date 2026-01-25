#include <OpenXRPlugin/OpenXRPluginPCH.h>

#include <OpenXRPlugin/OpenXRDeclarations.h>
#include <OpenXRPlugin/OpenXRSingleton.h>
#include <OpenXRPlugin/RenderModels/OpenXRRenderModel.h>
#include <OpenXRPlugin/RenderModels/OpenXRRenderModelManager.h>
#include <OpenXRPlugin/Utils/OpenXRConversionUtils.h>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

//////////////////////////////////////////////////////////////////////////
// ezOpenXRRenderModel Implementation
//////////////////////////////////////////////////////////////////////////

ezOpenXRRenderModel::ezOpenXRRenderModel() = default;

ezOpenXRRenderModel::~ezOpenXRRenderModel()
{
  // Cleanup is done by the manager
}

bool ezOpenXRRenderModel::GetNodeStates(ezDynamicArray<ezXRRenderModelNodeState>& out_nodeStates, XrTime predictedDisplayTime) const
{
  if (m_RenderModelHandle == XR_NULL_HANDLE || m_pOpenXR == nullptr)
    return false;

  auto* pManager = ezSingletonRegistry::GetSingletonInstance<ezOpenXRRenderModelManager>();
  if (pManager == nullptr || !pManager->IsSupported())
    return false;

  out_nodeStates.SetCount(m_uiAnimatableNodeCount);
  if (m_uiAnimatableNodeCount == 0)
    return true;

  ezHybridArray<XrRenderModelNodeStateEXT, 16> xrNodeStates;
  xrNodeStates.SetCount(m_uiAnimatableNodeCount);

  XrRenderModelStateGetInfoEXT stateGetInfo{XR_TYPE_RENDER_MODEL_STATE_GET_INFO_EXT};
  stateGetInfo.displayTime = predictedDisplayTime;

  XrRenderModelStateEXT state{XR_TYPE_RENDER_MODEL_STATE_EXT};
  state.nodeStateCount = m_uiAnimatableNodeCount;
  state.nodeStates = xrNodeStates.GetData();

  if (pManager->m_pfn_xrGetRenderModelStateEXT(m_RenderModelHandle, &stateGetInfo, &state) != XR_SUCCESS)
    return false;

  for (ezUInt32 i = 0; i < m_uiAnimatableNodeCount; ++i)
  {
    out_nodeStates[i].m_Transform = ezOpenXRConversionUtils::ConvertPoseToTransform(xrNodeStates[i].nodePose);
    out_nodeStates[i].m_bIsVisible = xrNodeStates[i].isVisible == XR_TRUE;
  }

  return true;
}

bool ezOpenXRRenderModel::LocateModel(XrSpace baseSpace, XrTime time, ezTransform& out_transform) const
{
  if (m_ModelSpace == XR_NULL_HANDLE)
    return false;

  XrSpaceLocation location{XR_TYPE_SPACE_LOCATION};
  if (xrLocateSpace(m_ModelSpace, baseSpace, time, &location) != XR_SUCCESS)
    return false;

  const XrSpaceLocationFlags requiredFlags = XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;
  if ((location.locationFlags & requiredFlags) != requiredFlags)
    return false;

  out_transform = ezOpenXRConversionUtils::ConvertPoseToTransform(location.pose);
  return true;
}

//////////////////////////////////////////////////////////////////////////
// ezOpenXRRenderModelManager Implementation
//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_SINGLETON(ezOpenXRRenderModelManager);

ezOpenXRRenderModelManager::ezOpenXRRenderModelManager(ezOpenXR* pOpenXR)
  : m_SingletonRegistrar(this)
  , m_pOpenXR(pOpenXR)
{
}

ezOpenXRRenderModelManager::~ezOpenXRRenderModelManager()
{
  Deinitialize();
}

XrResult ezOpenXRRenderModelManager::LoadExtensionFunctions(XrInstance instance)
{
  // XR_EXT_render_model functions
  XR_SUCCEED_OR_RETURN_LOG(xrGetInstanceProcAddr(instance, "xrCreateRenderModelEXT", reinterpret_cast<PFN_xrVoidFunction*>(&m_pfn_xrCreateRenderModelEXT)));
  XR_SUCCEED_OR_RETURN_LOG(xrGetInstanceProcAddr(instance, "xrDestroyRenderModelEXT", reinterpret_cast<PFN_xrVoidFunction*>(&m_pfn_xrDestroyRenderModelEXT)));
  XR_SUCCEED_OR_RETURN_LOG(xrGetInstanceProcAddr(instance, "xrGetRenderModelPropertiesEXT", reinterpret_cast<PFN_xrVoidFunction*>(&m_pfn_xrGetRenderModelPropertiesEXT)));
  XR_SUCCEED_OR_RETURN_LOG(xrGetInstanceProcAddr(instance, "xrCreateRenderModelSpaceEXT", reinterpret_cast<PFN_xrVoidFunction*>(&m_pfn_xrCreateRenderModelSpaceEXT)));
  XR_SUCCEED_OR_RETURN_LOG(xrGetInstanceProcAddr(instance, "xrCreateRenderModelAssetEXT", reinterpret_cast<PFN_xrVoidFunction*>(&m_pfn_xrCreateRenderModelAssetEXT)));
  XR_SUCCEED_OR_RETURN_LOG(xrGetInstanceProcAddr(instance, "xrDestroyRenderModelAssetEXT", reinterpret_cast<PFN_xrVoidFunction*>(&m_pfn_xrDestroyRenderModelAssetEXT)));
  XR_SUCCEED_OR_RETURN_LOG(xrGetInstanceProcAddr(instance, "xrGetRenderModelAssetDataEXT", reinterpret_cast<PFN_xrVoidFunction*>(&m_pfn_xrGetRenderModelAssetDataEXT)));
  XR_SUCCEED_OR_RETURN_LOG(xrGetInstanceProcAddr(instance, "xrGetRenderModelStateEXT", reinterpret_cast<PFN_xrVoidFunction*>(&m_pfn_xrGetRenderModelStateEXT)));

  return XR_SUCCESS;
}

XrResult ezOpenXRRenderModelManager::Initialize(XrSession session)
{
  m_Session = session;

  // Try to load extension functions
  if (LoadExtensionFunctions(m_pOpenXR->GetInstance()) != XR_SUCCESS)
  {
    ezLog::Warning("OpenXR: XR_EXT_render_model extension functions not available");
    m_bSupported = false;
    return XR_SUCCESS; // Not a fatal error
  }

  m_bSupported = true;

  // Check for interaction render model extension
  PFN_xrEnumerateInteractionRenderModelIdsEXT pfnEnumIds = nullptr;
  if (xrGetInstanceProcAddr(m_pOpenXR->GetInstance(), "xrEnumerateInteractionRenderModelIdsEXT",
        reinterpret_cast<PFN_xrVoidFunction*>(&pfnEnumIds)) == XR_SUCCESS)
  {
    m_pfn_xrEnumerateInteractionRenderModelIdsEXT = pfnEnumIds;
    m_bInteractionRenderModelSupported = true;
  }

  m_bEnumerationDirty = true;

  ezLog::Info("OpenXR: Render model support initialized (interaction models: {})",
    m_bInteractionRenderModelSupported ? "yes" : "no");

  return XR_SUCCESS;
}

void ezOpenXRRenderModelManager::Deinitialize()
{
  // Destroy all cached models
  for (auto& pair : m_CachedModels)
  {
    auto& model = pair.Value();
    if (model)
    {
      if (model->m_ModelSpace != XR_NULL_HANDLE)
      {
        xrDestroySpace(model->m_ModelSpace);
        model->m_ModelSpace = XR_NULL_HANDLE;
      }
      if (model->m_AssetHandle != XR_NULL_HANDLE && m_pfn_xrDestroyRenderModelAssetEXT)
      {
        m_pfn_xrDestroyRenderModelAssetEXT(model->m_AssetHandle);
        model->m_AssetHandle = XR_NULL_HANDLE;
      }
      if (model->m_RenderModelHandle != XR_NULL_HANDLE && m_pfn_xrDestroyRenderModelEXT)
      {
        m_pfn_xrDestroyRenderModelEXT(model->m_RenderModelHandle);
        model->m_RenderModelHandle = XR_NULL_HANDLE;
      }
    }
  }

  m_CachedModels.Clear();
  m_CachedInteractionModels.Clear();
  m_Session = XR_NULL_HANDLE;
}

XrResult ezOpenXRRenderModelManager::EnumerateInteractionRenderModelIds(ezDynamicArray<XrRenderModelIdEXT>& out_modelIds)
{
  out_modelIds.Clear();

  if (!m_bInteractionRenderModelSupported || m_pfn_xrEnumerateInteractionRenderModelIdsEXT == nullptr)
  {
    return XR_ERROR_FEATURE_UNSUPPORTED;
  }

  XrInteractionRenderModelIdsEnumerateInfoEXT enumInfo{XR_TYPE_INTERACTION_RENDER_MODEL_IDS_ENUMERATE_INFO_EXT};

  // First call to get count
  ezUInt32 modelCount = 0;
  XrResult result = m_pfn_xrEnumerateInteractionRenderModelIdsEXT(m_Session, &enumInfo, 0, &modelCount, nullptr);
  if (result != XR_SUCCESS)
    return result;

  if (modelCount == 0)
  {
    return XR_SUCCESS;
  }

  // Second call to get IDs
  out_modelIds.SetCount(modelCount);
  result = m_pfn_xrEnumerateInteractionRenderModelIdsEXT(m_Session, &enumInfo, modelCount, &modelCount, out_modelIds.GetData());
  return result;
}

bool ezOpenXRRenderModelManager::EnumerateInteractionRenderModels(ezDynamicArray<ezXRRenderModelInfo>& out_models)
{
  out_models.Clear();

  if (!m_bInteractionRenderModelSupported || m_pfn_xrEnumerateInteractionRenderModelIdsEXT == nullptr)
  {
    return false;
  }

  if (!m_bEnumerationDirty && !m_CachedInteractionModels.IsEmpty())
  {
    out_models = m_CachedInteractionModels;
    return true;
  }

  ezDynamicArray<XrRenderModelIdEXT> modelIds;
  if (EnumerateInteractionRenderModelIds(modelIds) != XR_SUCCESS)
    return false;

  m_CachedInteractionModels.Clear();
  m_CachedInteractionModels.Reserve(modelIds.GetCount());

  for (const auto& id : modelIds)
  {
    ezXRRenderModelInfo info;
    info.m_Handle = static_cast<ezXRRenderModelHandle>(id);
    info.m_bIsInteractionDevice = true;
    ezStringBuilder sName;
    sName.SetFormat("InteractionModel_{}", id);
    info.m_sName = sName;
    m_CachedInteractionModels.PushBack(info);
  }

  m_bEnumerationDirty = false;
  out_models = m_CachedInteractionModels;
  return true;
}

ezSharedPtr<ezOpenXRRenderModel> ezOpenXRRenderModelManager::GetOrLoadRenderModel(XrRenderModelIdEXT modelId)
{
  if (!m_bSupported || modelId == XR_NULL_RENDER_MODEL_ID_EXT)
    return nullptr;

  // Check cache first
  if (auto* pExisting = m_CachedModels.GetValue(modelId))
  {
    return *pExisting;
  }

  // Create the render model
  XrRenderModelCreateInfoEXT createInfo{XR_TYPE_RENDER_MODEL_CREATE_INFO_EXT};
  createInfo.renderModelId = modelId;
  createInfo.gltfExtensionCount = 0;
  createInfo.gltfExtensions = nullptr;

  XrRenderModelEXT renderModel = XR_NULL_HANDLE;
  XrResult result = m_pfn_xrCreateRenderModelEXT(m_Session, &createInfo, &renderModel);
  if (result != XR_SUCCESS)
  {
    ezLog::Warning("OpenXR: Failed to create render model {}: {}", modelId, result);
    return nullptr;
  }

  // Get properties
  XrRenderModelPropertiesGetInfoEXT propsGetInfo{XR_TYPE_RENDER_MODEL_PROPERTIES_GET_INFO_EXT};
  XrRenderModelPropertiesEXT properties{XR_TYPE_RENDER_MODEL_PROPERTIES_EXT};
  result = m_pfn_xrGetRenderModelPropertiesEXT(renderModel, &propsGetInfo, &properties);
  if (result != XR_SUCCESS)
  {
    m_pfn_xrDestroyRenderModelEXT(renderModel);
    ezLog::Warning("OpenXR: Failed to get render model properties: {}", result);
    return nullptr;
  }

  // Create asset
  XrRenderModelAssetCreateInfoEXT assetCreateInfo{XR_TYPE_RENDER_MODEL_ASSET_CREATE_INFO_EXT};
  assetCreateInfo.cacheId = properties.cacheId;

  XrRenderModelAssetEXT asset = XR_NULL_HANDLE;
  result = m_pfn_xrCreateRenderModelAssetEXT(m_Session, &assetCreateInfo, &asset);
  if (result != XR_SUCCESS)
  {
    m_pfn_xrDestroyRenderModelEXT(renderModel);
    ezLog::Warning("OpenXR: Failed to create render model asset: {}", result);
    return nullptr;
  }

  // Get asset data (GLB)
  XrRenderModelAssetDataGetInfoEXT dataGetInfo{XR_TYPE_RENDER_MODEL_ASSET_DATA_GET_INFO_EXT};
  XrRenderModelAssetDataEXT assetData{XR_TYPE_RENDER_MODEL_ASSET_DATA_EXT};
  assetData.bufferCapacityInput = 0;
  assetData.buffer = nullptr;

  // First call to get size
  result = m_pfn_xrGetRenderModelAssetDataEXT(asset, &dataGetInfo, &assetData);
  if (result != XR_SUCCESS)
  {
    m_pfn_xrDestroyRenderModelAssetEXT(asset);
    m_pfn_xrDestroyRenderModelEXT(renderModel);
    ezLog::Warning("OpenXR: Failed to get render model asset size: {}", result);
    return nullptr;
  }

  ezDynamicArray<ezUInt8> glbData;
  glbData.SetCount(assetData.bufferCountOutput);
  assetData.bufferCapacityInput = glbData.GetCount();
  assetData.buffer = glbData.GetData();

  // Second call to get data
  result = m_pfn_xrGetRenderModelAssetDataEXT(asset, &dataGetInfo, &assetData);
  if (result != XR_SUCCESS)
  {
    m_pfn_xrDestroyRenderModelAssetEXT(asset);
    m_pfn_xrDestroyRenderModelEXT(renderModel);
    ezLog::Warning("OpenXR: Failed to get render model asset data: {}", result);
    return nullptr;
  }

  // Create model space
  XrRenderModelSpaceCreateInfoEXT spaceCreateInfo{XR_TYPE_RENDER_MODEL_SPACE_CREATE_INFO_EXT};
  spaceCreateInfo.renderModel = renderModel;

  XrSpace modelSpace = XR_NULL_HANDLE;
  result = m_pfn_xrCreateRenderModelSpaceEXT(m_Session, &spaceCreateInfo, &modelSpace);
  if (result != XR_SUCCESS)
  {
    m_pfn_xrDestroyRenderModelAssetEXT(asset);
    m_pfn_xrDestroyRenderModelEXT(renderModel);
    ezLog::Warning("OpenXR: Failed to create render model space: {}", result);
    return nullptr;
  }

  // Create and populate the model object
  auto pModel = EZ_DEFAULT_NEW(ezOpenXRRenderModel);
  pModel->m_pOpenXR = m_pOpenXR;
  pModel->m_RenderModelId = modelId;
  pModel->m_RenderModelHandle = renderModel;
  pModel->m_AssetHandle = asset;
  pModel->m_ModelSpace = modelSpace;
  pModel->m_uiAnimatableNodeCount = properties.animatableNodeCount;

  // Parse the GLB data to create mesh resources
  if (!ParseGLB(glbData, pModel))
  {
    ezLog::Warning("OpenXR: Failed to parse GLB data for render model {}", modelId);
    // Continue anyway - model will be valid but without meshes
  }

  ezSharedPtr<ezOpenXRRenderModel> result_ptr(pModel);
  m_CachedModels[modelId] = result_ptr;

  ezLog::Info("OpenXR: Loaded render model {} ({} animatable nodes, {} KB GLB)",
    modelId, properties.animatableNodeCount, glbData.GetCount() / 1024);

  return result_ptr;
}

ezSharedPtr<ezOpenXRRenderModel> ezOpenXRRenderModelManager::GetRenderModel(XrRenderModelIdEXT modelId)
{
  if (auto* pExisting = m_CachedModels.GetValue(modelId))
  {
    return *pExisting;
  }
  return nullptr;
}

bool ezOpenXRRenderModelManager::LoadRenderModel(ezXRRenderModelHandle handle)
{
  XrRenderModelIdEXT modelId = static_cast<XrRenderModelIdEXT>(handle);
  return GetOrLoadRenderModel(modelId) != nullptr;
}

bool ezOpenXRRenderModelManager::IsRenderModelLoaded(ezXRRenderModelHandle handle) const
{
  XrRenderModelIdEXT modelId = static_cast<XrRenderModelIdEXT>(handle);
  return m_CachedModels.Contains(modelId);
}

bool ezOpenXRRenderModelManager::GetRenderModelNodes(ezXRRenderModelHandle handle, ezDynamicArray<ezXRRenderModelNode>& out_nodes) const
{
  out_nodes.Clear();

  XrRenderModelIdEXT modelId = static_cast<XrRenderModelIdEXT>(handle);
  auto* pModel = m_CachedModels.GetValue(modelId);
  if (pModel == nullptr || !(*pModel)->IsValid())
    return false;

  // Copy nodes from internal model
  const auto& srcNodes = (*pModel)->GetNodes();
  out_nodes.Reserve(srcNodes.GetCount());

  for (const auto& srcNode : srcNodes)
  {
    ezXRRenderModelNode node;
    node.m_sName = srcNode.m_sName;
    node.m_iParentIndex = srcNode.m_iParentIndex;
    node.m_LocalTransform = srcNode.m_LocalTransform;
    node.m_bIsAnimatable = srcNode.m_bIsAnimatable;
    node.m_uiAnimatableIndex = srcNode.m_uiAnimatableNodeIndex;
    out_nodes.PushBack(node);
  }

  return true;
}

bool ezOpenXRRenderModelManager::GetRenderModelMeshes(ezXRRenderModelHandle handle, ezDynamicArray<ezXRRenderModelMesh>& out_meshes) const
{
  out_meshes.Clear();

  XrRenderModelIdEXT modelId = static_cast<XrRenderModelIdEXT>(handle);
  auto* pModel = m_CachedModels.GetValue(modelId);
  if (pModel == nullptr || !(*pModel)->IsValid())
    return false;

  const ezOpenXRRenderModel* pRenderModel = pModel->Borrow();
  const auto& parsedMeshes = pRenderModel->GetParsedMeshes();

  out_meshes.Reserve(parsedMeshes.GetCount());

  for (const auto& parsedMesh : parsedMeshes)
  {
    ezXRRenderModelMesh& mesh = out_meshes.ExpandAndGetRef();
    mesh.m_Positions = parsedMesh.m_Positions;
    mesh.m_Normals = parsedMesh.m_Normals;
    mesh.m_TexCoords = parsedMesh.m_TexCoords;
    mesh.m_Indices = parsedMesh.m_Indices;
    mesh.m_uiNodeIndex = parsedMesh.m_uiNodeIndex;
  }

  return true;
}

bool ezOpenXRRenderModelManager::GetRenderModelState(ezXRRenderModelHandle handle, ezDynamicArray<ezXRRenderModelNodeState>& out_states) const
{
  out_states.Clear();

  XrRenderModelIdEXT modelId = static_cast<XrRenderModelIdEXT>(handle);
  auto* pModel = m_CachedModels.GetValue(modelId);
  if (pModel == nullptr || !(*pModel)->IsValid())
    return false;

  return (*pModel)->GetNodeStates(out_states, GetPredictedDisplayTime());
}

bool ezOpenXRRenderModelManager::GetRenderModelTransform(ezXRRenderModelHandle handle, ezTransform& out_transform) const
{
  XrRenderModelIdEXT modelId = static_cast<XrRenderModelIdEXT>(handle);
  auto* pModel = m_CachedModels.GetValue(modelId);
  if (pModel == nullptr || !(*pModel)->IsValid())
    return false;

  XrSpace baseSpace = m_pOpenXR->GetBaseSpace();
  if (baseSpace == XR_NULL_HANDLE)
    return false;

  return (*pModel)->LocateModel(baseSpace, GetPredictedDisplayTime(), out_transform);
}

ezUInt32 ezOpenXRRenderModelManager::GetAnimatableNodeCount(ezXRRenderModelHandle handle) const
{
  XrRenderModelIdEXT modelId = static_cast<XrRenderModelIdEXT>(handle);
  auto* pModel = m_CachedModels.GetValue(modelId);
  if (pModel == nullptr || !(*pModel)->IsValid())
    return 0;

  return (*pModel)->GetAnimatableNodeCount();
}

void ezOpenXRRenderModelManager::UnloadRenderModel(ezXRRenderModelHandle handle)
{
  XrRenderModelIdEXT modelId = static_cast<XrRenderModelIdEXT>(handle);
  auto it = m_CachedModels.Find(modelId);
  if (!it.IsValid())
    return;

  auto& model = it.Value();
  if (model)
  {
    if (model->m_ModelSpace != XR_NULL_HANDLE)
    {
      xrDestroySpace(model->m_ModelSpace);
      model->m_ModelSpace = XR_NULL_HANDLE;
    }
    if (model->m_AssetHandle != XR_NULL_HANDLE && m_pfn_xrDestroyRenderModelAssetEXT)
    {
      m_pfn_xrDestroyRenderModelAssetEXT(model->m_AssetHandle);
      model->m_AssetHandle = XR_NULL_HANDLE;
    }
    if (model->m_RenderModelHandle != XR_NULL_HANDLE && m_pfn_xrDestroyRenderModelEXT)
    {
      m_pfn_xrDestroyRenderModelEXT(model->m_RenderModelHandle);
      model->m_RenderModelHandle = XR_NULL_HANDLE;
    }
  }

  m_CachedModels.Remove(it);
}

XrTime ezOpenXRRenderModelManager::GetPredictedDisplayTime() const
{
  // Access the predicted display time from the OpenXR singleton
  // This is typically set during BeginFrame
  return m_pOpenXR ? m_pOpenXR->m_FrameState.predictedDisplayTime : 0;
}

void ezOpenXRRenderModelManager::OnRenderModelsChanged()
{
  m_bEnumerationDirty = true;
  ezLog::Info("OpenXR: Interaction render models changed, re-enumeration needed");
}

void ezOpenXRRenderModelManager::InvalidateAll()
{
  // Mark cached models as invalid but don't destroy - let components re-request
  m_bEnumerationDirty = true;
}

namespace
{
  ezVec3 ConvertAssimpVec(const aiVector3D& v)
  {
    return ezVec3(v.x, v.y, v.z);
  }

  ezMat4 ConvertAssimpMatrix(const aiMatrix4x4& m)
  {
    // aiMatrix4x4 is row-major, ezMat4 is column-major
    return ezMat4::MakeFromRowMajorArray(reinterpret_cast<const float*>(&m));
  }

  void TraverseNodes(const aiNode* pNode, ezInt32 iParentIndex, ezDynamicArray<ezOpenXRRenderModelNode>& out_nodes,
                     ezDynamicArray<ezUInt32>& out_meshNodeIndices, const aiScene* pScene)
  {
    ezInt32 iCurrentIndex = static_cast<ezInt32>(out_nodes.GetCount());

    ezOpenXRRenderModelNode& node = out_nodes.ExpandAndGetRef();
    node.m_sName = pNode->mName.C_Str();
    node.m_iParentIndex = iParentIndex;

    // Extract local transform from the node's matrix
    aiVector3D scaling, position;
    aiQuaternion rotation;
    pNode->mTransformation.Decompose(scaling, rotation, position);

    node.m_LocalTransform.m_vPosition = ConvertAssimpVec(position);
    node.m_LocalTransform.m_qRotation = ezQuat(rotation.x, rotation.y, rotation.z, rotation.w);
    node.m_LocalTransform.m_vScale = ConvertAssimpVec(scaling);

    // For now, mark all nodes as not animatable - OpenXR will provide animation state
    // The actual animatable nodes will be determined by the XrRenderModelPropertiesEXT
    node.m_bIsAnimatable = false;
    node.m_uiAnimatableNodeIndex = 0;

    // Record which node each mesh belongs to
    for (ezUInt32 i = 0; i < pNode->mNumMeshes; ++i)
    {
      while (out_meshNodeIndices.GetCount() <= pNode->mMeshes[i])
        out_meshNodeIndices.PushBack(0);
      out_meshNodeIndices[pNode->mMeshes[i]] = static_cast<ezUInt32>(iCurrentIndex);
    }

    // Recurse to children
    for (ezUInt32 i = 0; i < pNode->mNumChildren; ++i)
    {
      TraverseNodes(pNode->mChildren[i], iCurrentIndex, out_nodes, out_meshNodeIndices, pScene);
    }
  }
}

bool ezOpenXRRenderModelManager::ParseGLB(const ezArrayPtr<const ezUInt8>& glbData, ezOpenXRRenderModel* pModel)
{
  // GLB structure:
  // - Header (12 bytes): magic, version, length
  // - Chunks: each has length (4), type (4), data (length)
  //   - First chunk: JSON (type 0x4E4F534A "JSON")
  //   - Second chunk: Binary buffer (type 0x004E4942 "BIN")

  if (glbData.GetCount() < 12)
  {
    ezLog::Error("OpenXR: GLB data too small");
    return false;
  }

  // Read header for basic validation
  const ezUInt32 magic = *reinterpret_cast<const ezUInt32*>(&glbData[0]);
  const ezUInt32 version = *reinterpret_cast<const ezUInt32*>(&glbData[4]);
  const ezUInt32 length = *reinterpret_cast<const ezUInt32*>(&glbData[8]);

  if (magic != 0x46546C67) // "glTF"
  {
    ezLog::Error("OpenXR: Invalid GLB magic number");
    return false;
  }

  if (version != 2)
  {
    ezLog::Error("OpenXR: Unsupported glTF version: {}", version);
    return false;
  }

  if (length > glbData.GetCount())
  {
    ezLog::Error("OpenXR: GLB length mismatch");
    return false;
  }

  // Use assimp to parse the GLB from memory
  Assimp::Importer importer;

  // Import flags for GLB:
  // - Triangulate: Ensure all faces are triangles
  // - JoinIdenticalVertices: Optimize mesh
  // - FlipUVs: Convert from OpenGL to DirectX UV coordinate system
  // - ImproveCacheLocality: Optimize for GPU vertex cache
  constexpr ezUInt32 uiImportFlags =
    aiProcess_Triangulate |
    aiProcess_JoinIdenticalVertices |
    aiProcess_FlipUVs |
    aiProcess_ImproveCacheLocality;

  const aiScene* pScene = importer.ReadFileFromMemory(
    glbData.GetPtr(),
    glbData.GetCount(),
    uiImportFlags,
    ".glb");

  if (pScene == nullptr)
  {
    ezLog::Error("OpenXR: Failed to parse GLB with assimp: {}", importer.GetErrorString());
    return false;
  }

  if (!pScene->HasMeshes())
  {
    ezLog::Warning("OpenXR: GLB contains no meshes");
    return true; // Not an error, just empty
  }

  ezLog::Debug("OpenXR: Parsing GLB - {} meshes, {} nodes", pScene->mNumMeshes, pScene->mRootNode ? 1 : 0);

  // First, traverse the node hierarchy to build our node list
  pModel->m_Nodes.Clear();
  ezDynamicArray<ezUInt32> meshNodeIndices;

  if (pScene->mRootNode != nullptr)
  {
    TraverseNodes(pScene->mRootNode, -1, pModel->m_Nodes, meshNodeIndices, pScene);
  }

  // Now process each mesh
  pModel->m_ParsedMeshes.Clear();
  pModel->m_ParsedMeshes.Reserve(pScene->mNumMeshes);

  for (ezUInt32 meshIdx = 0; meshIdx < pScene->mNumMeshes; ++meshIdx)
  {
    const aiMesh* pMesh = pScene->mMeshes[meshIdx];

    // Skip non-triangle meshes
    if ((pMesh->mPrimitiveTypes & aiPrimitiveType_TRIANGLE) == 0)
      continue;

    if (pMesh->mNumVertices == 0 || pMesh->mNumFaces == 0)
      continue;

    ezOpenXRParsedMesh& parsedMesh = pModel->m_ParsedMeshes.ExpandAndGetRef();
    parsedMesh.m_uiNodeIndex = meshNodeIndices.GetCount() > meshIdx ? meshNodeIndices[meshIdx] : 0;

    // Extract positions
    parsedMesh.m_Positions.Reserve(pMesh->mNumVertices);
    for (ezUInt32 v = 0; v < pMesh->mNumVertices; ++v)
    {
      parsedMesh.m_Positions.PushBack(ConvertAssimpVec(pMesh->mVertices[v]));
    }

    // Extract normals if available
    if (pMesh->HasNormals())
    {
      parsedMesh.m_Normals.Reserve(pMesh->mNumVertices);
      for (ezUInt32 v = 0; v < pMesh->mNumVertices; ++v)
      {
        parsedMesh.m_Normals.PushBack(ConvertAssimpVec(pMesh->mNormals[v]));
      }
    }

    // Extract texture coordinates if available
    if (pMesh->HasTextureCoords(0))
    {
      parsedMesh.m_TexCoords.Reserve(pMesh->mNumVertices);
      for (ezUInt32 v = 0; v < pMesh->mNumVertices; ++v)
      {
        const aiVector3D& tc = pMesh->mTextureCoords[0][v];
        parsedMesh.m_TexCoords.PushBack(ezVec2(tc.x, tc.y));
      }
    }

    // Extract indices
    parsedMesh.m_Indices.Reserve(pMesh->mNumFaces * 3);
    for (ezUInt32 f = 0; f < pMesh->mNumFaces; ++f)
    {
      const aiFace& face = pMesh->mFaces[f];
      if (face.mNumIndices == 3)
      {
        parsedMesh.m_Indices.PushBack(face.mIndices[0]);
        parsedMesh.m_Indices.PushBack(face.mIndices[1]);
        parsedMesh.m_Indices.PushBack(face.mIndices[2]);
      }
    }

    ezLog::Debug("OpenXR: Parsed mesh {} - {} vertices, {} triangles",
      meshIdx, parsedMesh.m_Positions.GetCount(), parsedMesh.m_Indices.GetCount() / 3);
  }

  ezLog::Info("OpenXR: Successfully parsed GLB with {} nodes and {} meshes",
    pModel->m_Nodes.GetCount(), pModel->m_ParsedMeshes.GetCount());

  return true;
}

EZ_STATICLINK_FILE(OpenXRPlugin, OpenXRPlugin_RenderModels_Implementation_OpenXRRenderModelManager);

