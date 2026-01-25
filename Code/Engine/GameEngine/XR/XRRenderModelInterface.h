#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Foundation/Types/UniquePtr.h>

class ezWorld;

/// \brief Handle to a render model instance.
using ezXRRenderModelHandle = ezUInt64;

constexpr ezXRRenderModelHandle ezInvalidXRRenderModelHandle = 0;

/// \brief Information about an available controller/interaction device render model.
struct ezXRRenderModelInfo
{
  ezXRRenderModelHandle m_Handle = ezInvalidXRRenderModelHandle;
  ezString m_sName;
  bool m_bIsInteractionDevice = false; ///< True if this is a controller model.
};

/// \brief State of an animatable node within a render model.
struct ezXRRenderModelNodeState
{
  ezTransform m_Transform; ///< Current transform (replaces node's local transform).
  bool m_bIsVisible = true; ///< Whether this node should be rendered.
};

/// \brief Mesh data for a render model primitive.
struct ezXRRenderModelMesh
{
  ezDynamicArray<ezVec3> m_Positions;
  ezDynamicArray<ezVec3> m_Normals;
  ezDynamicArray<ezVec2> m_TexCoords;
  ezDynamicArray<ezUInt32> m_Indices;
  ezUInt32 m_uiNodeIndex = 0;
};

/// \brief Node in the render model hierarchy.
struct ezXRRenderModelNode
{
  ezString m_sName;
  ezInt32 m_iParentIndex = -1;
  ezTransform m_LocalTransform;
  bool m_bIsAnimatable = false;
  ezUInt32 m_uiAnimatableIndex = 0; ///< Index for animation state if animatable.
};

/// \brief Interface for accessing XR controller render models.
///
/// This interface is implemented by XR backends (e.g., OpenXR plugin) to provide
/// 3D models of controllers that can be displayed in the virtual environment.
///
/// Usage:
/// 1. Call EnumerateInteractionRenderModels() to get available models
/// 2. Call LoadRenderModel() to load a specific model
/// 3. Call GetRenderModelState() each frame to get animation states
/// 4. Call GetRenderModelTransform() to get the model's world position
class EZ_GAMEENGINE_DLL ezXRRenderModelInterface
{
public:
  virtual ~ezXRRenderModelInterface() = default;

  /// \brief Returns whether render models are supported by the XR runtime.
  virtual bool IsSupported() const = 0;

  /// \brief Enumerates available interaction device (controller) render models.
  ///
  /// \param out_models Array to receive available model information.
  /// \return True on success.
  virtual bool EnumerateInteractionRenderModels(ezDynamicArray<ezXRRenderModelInfo>& out_models) = 0;

  /// \brief Loads a render model.
  ///
  /// \param handle The model handle from enumeration.
  /// \return True on success.
  virtual bool LoadRenderModel(ezXRRenderModelHandle handle) = 0;

  /// \brief Checks if a render model is loaded and ready.
  virtual bool IsRenderModelLoaded(ezXRRenderModelHandle handle) const = 0;

  /// \brief Gets the nodes (scene graph) of a loaded render model.
  virtual bool GetRenderModelNodes(ezXRRenderModelHandle handle, ezDynamicArray<ezXRRenderModelNode>& out_nodes) const = 0;

  /// \brief Gets the meshes of a loaded render model.
  virtual bool GetRenderModelMeshes(ezXRRenderModelHandle handle, ezDynamicArray<ezXRRenderModelMesh>& out_meshes) const = 0;

  /// \brief Gets the current animation state for all animatable nodes.
  ///
  /// \param handle The model handle.
  /// \param out_states Array to receive node states.
  /// \return True on success.
  virtual bool GetRenderModelState(ezXRRenderModelHandle handle, ezDynamicArray<ezXRRenderModelNodeState>& out_states) const = 0;

  /// \brief Gets the world transform of a render model.
  ///
  /// \param handle The model handle.
  /// \param out_transform The output transform.
  /// \return True if the transform is valid.
  virtual bool GetRenderModelTransform(ezXRRenderModelHandle handle, ezTransform& out_transform) const = 0;

  /// \brief Returns the number of animatable nodes in the model.
  virtual ezUInt32 GetAnimatableNodeCount(ezXRRenderModelHandle handle) const = 0;

  /// \brief Unloads a render model, freeing resources.
  virtual void UnloadRenderModel(ezXRRenderModelHandle handle) = 0;
};

