#pragma once

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/UniquePtr.h>
#include <GameEngine/XR/XRRenderModelInterface.h>
#include <OpenXRPlugin/Basics.h>
#include <OpenXRPlugin/OpenXRIncludes.h>
#include <OpenXRPlugin/RenderModels/OpenXRRenderModel.h>

class ezOpenXR;

/// \brief Manages OpenXR render models for controllers and other tracked devices.
///
/// This singleton provides access to the XR_EXT_render_model and XR_EXT_interaction_render_model
/// extensions, allowing applications to retrieve 3D models of controllers and other
/// interaction devices for rendering.
///
/// Implements ezXRRenderModelInterface so it can be used by ezControllerRenderModelComponent.
///
/// Usage:
/// 1. Check IsSupported() to see if render models are available
/// 2. Use EnumerateInteractionRenderModels() to get available controller models
/// 3. Use CreateRenderModel() to load a model
/// 4. Each frame, use the model's GetNodeStates() to get animation state
///
class EZ_OPENXRPLUGIN_DLL ezOpenXRRenderModelManager : public ezXRRenderModelInterface
{
  EZ_DECLARE_SINGLETON_OF_INTERFACE(ezOpenXRRenderModelManager, ezXRRenderModelInterface);

public:
  ezOpenXRRenderModelManager(ezOpenXR* pOpenXR);
  ~ezOpenXRRenderModelManager();

  //
  // ezXRRenderModelInterface Implementation
  //

  virtual bool IsSupported() const override { return m_bSupported; }
  virtual bool EnumerateInteractionRenderModels(ezDynamicArray<ezXRRenderModelInfo>& out_models) override;
  virtual bool LoadRenderModel(ezXRRenderModelHandle handle) override;
  virtual bool IsRenderModelLoaded(ezXRRenderModelHandle handle) const override;
  virtual bool GetRenderModelNodes(ezXRRenderModelHandle handle, ezDynamicArray<ezXRRenderModelNode>& out_nodes) const override;
  virtual bool GetRenderModelMeshes(ezXRRenderModelHandle handle, ezDynamicArray<ezXRRenderModelMesh>& out_meshes) const override;
  virtual bool GetRenderModelState(ezXRRenderModelHandle handle, ezDynamicArray<ezXRRenderModelNodeState>& out_states) const override;
  virtual bool GetRenderModelTransform(ezXRRenderModelHandle handle, ezTransform& out_transform) const override;
  virtual ezUInt32 GetAnimatableNodeCount(ezXRRenderModelHandle handle) const override;
  virtual void UnloadRenderModel(ezXRRenderModelHandle handle) override;

  //
  // OpenXR-Specific API
  //

  /// \brief Returns whether interaction render model extension is supported.
  bool IsInteractionRenderModelSupported() const { return m_bInteractionRenderModelSupported; }

  /// \brief Initializes the render model manager after session creation.
  XrResult Initialize(XrSession session);

  /// \brief Deinitializes the manager before session destruction.
  void Deinitialize();

  /// \brief Called when XrEventDataInteractionRenderModelsChangedEXT is received.
  void OnRenderModelsChanged();

  /// \brief Invalidates all cached models, forcing reload on next access.
  void InvalidateAll();

  /// \brief Gets the predicted display time for the current frame.
  XrTime GetPredictedDisplayTime() const;

private:
  friend class ezOpenXRRenderModel;

  XrResult LoadExtensionFunctions(XrInstance instance);
  XrResult EnumerateInteractionRenderModelIds(ezDynamicArray<XrRenderModelIdEXT>& out_modelIds);
  bool ParseGLB(const ezArrayPtr<const ezUInt8>& glbData, ezOpenXRRenderModel* pModel);
  ezSharedPtr<ezOpenXRRenderModel> GetOrLoadRenderModel(XrRenderModelIdEXT modelId);
  ezSharedPtr<ezOpenXRRenderModel> GetRenderModel(XrRenderModelIdEXT modelId);

  ezOpenXR* m_pOpenXR = nullptr;
  XrSession m_Session = XR_NULL_HANDLE;

  bool m_bSupported = false;
  bool m_bInteractionRenderModelSupported = false;

  // Extension function pointers
  PFN_xrEnumerateInteractionRenderModelIdsEXT m_pfn_xrEnumerateInteractionRenderModelIdsEXT = nullptr;
  PFN_xrCreateRenderModelEXT m_pfn_xrCreateRenderModelEXT = nullptr;
  PFN_xrDestroyRenderModelEXT m_pfn_xrDestroyRenderModelEXT = nullptr;
  PFN_xrGetRenderModelPropertiesEXT m_pfn_xrGetRenderModelPropertiesEXT = nullptr;
  PFN_xrCreateRenderModelAssetEXT m_pfn_xrCreateRenderModelAssetEXT = nullptr;
  PFN_xrDestroyRenderModelAssetEXT m_pfn_xrDestroyRenderModelAssetEXT = nullptr;
  PFN_xrGetRenderModelAssetDataEXT m_pfn_xrGetRenderModelAssetDataEXT = nullptr;
  PFN_xrCreateRenderModelSpaceEXT m_pfn_xrCreateRenderModelSpaceEXT = nullptr;
  PFN_xrGetRenderModelStateEXT m_pfn_xrGetRenderModelStateEXT = nullptr;

  // Cache of loaded models
  ezMap<XrRenderModelIdEXT, ezSharedPtr<ezOpenXRRenderModel>> m_CachedModels;

  // Cached enumeration
  ezDynamicArray<ezXRRenderModelInfo> m_CachedInteractionModels;
  bool m_bEnumerationDirty = true;
};

