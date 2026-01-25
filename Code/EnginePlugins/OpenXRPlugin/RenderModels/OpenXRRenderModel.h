#pragma once

#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/UniquePtr.h>
#include <GameEngine/XR/Declarations.h>
#include <GameEngine/XR/XRRenderModelInterface.h>
#include <OpenXRPlugin/Basics.h>
#include <OpenXRPlugin/OpenXRIncludes.h>
#include <RendererCore/Meshes/MeshResource.h>

EZ_DEFINE_AS_POD_TYPE(XrRenderModelNodeStateEXT);

class ezOpenXR;
class ezWorld;

/// \brief Represents an animatable node within a render model.
///
/// Each node can have its transform and visibility updated per-frame
/// based on the current controller state.
struct ezOpenXRRenderModelNode
{
  ezString m_sName;           ///< Name of the node from glTF.
  ezInt32 m_iParentIndex;     ///< Index of parent node, -1 if root.
  ezTransform m_LocalTransform; ///< Local transform relative to parent.
  bool m_bIsAnimatable;       ///< Whether this node can be animated by OpenXR.
  ezUInt32 m_uiAnimatableNodeIndex; ///< Index for xrGetRenderModelStateEXT if animatable.
};

/// \brief Represents a single mesh primitive within a render model.
struct ezOpenXRRenderModelPrimitive
{
  ezMeshResourceHandle m_hMesh;  ///< The mesh resource for this primitive.
  ezUInt32 m_uiNodeIndex;        ///< Index of the node this primitive belongs to.
  ezMaterialResourceHandle m_hMaterial; ///< Material for this primitive.
};

/// \brief Parsed mesh data from GLB.
struct ezOpenXRParsedMesh
{
  ezDynamicArray<ezVec3> m_Positions;
  ezDynamicArray<ezVec3> m_Normals;
  ezDynamicArray<ezVec2> m_TexCoords;
  ezDynamicArray<ezUInt32> m_Indices;
  ezUInt32 m_uiNodeIndex = 0;
};

/// \brief Represents a parsed and loaded render model ready for rendering.
///
/// A render model can contain multiple nodes organized in a hierarchy,
/// with each node potentially having mesh primitives attached.
class EZ_OPENXRPLUGIN_DLL ezOpenXRRenderModel : public ezRefCounted
{
public:
  ezOpenXRRenderModel();
  ~ezOpenXRRenderModel();

  /// \brief Returns whether this render model is valid and ready to use.
  bool IsValid() const { return m_RenderModelHandle != XR_NULL_HANDLE; }

  /// \brief Returns the model's unique ID.
  XrRenderModelIdEXT GetId() const { return m_RenderModelId; }

  /// \brief Returns all nodes in the model.
  ezArrayPtr<const ezOpenXRRenderModelNode> GetNodes() const { return m_Nodes; }

  /// \brief Returns all mesh primitives in the model.
  ezArrayPtr<const ezOpenXRRenderModelPrimitive> GetPrimitives() const { return m_Primitives; }

  /// \brief Returns all parsed mesh data.
  ezArrayPtr<const ezOpenXRParsedMesh> GetParsedMeshes() const { return m_ParsedMeshes; }

  /// \brief Gets the current animation state for all animatable nodes.
  ///
  /// \param out_nodeStates Array to receive node states, sized to number of animatable nodes.
  /// \param predictedDisplayTime The predicted display time for this frame.
  /// \return True if states were retrieved successfully.
  bool GetNodeStates(ezDynamicArray<ezXRRenderModelNodeState>& out_nodeStates, XrTime predictedDisplayTime) const;

  /// \brief Returns the number of animatable nodes.
  ezUInt32 GetAnimatableNodeCount() const { return m_uiAnimatableNodeCount; }

  /// \brief Returns the model space for locating the render model in world space.
  XrSpace GetModelSpace() const { return m_ModelSpace; }

  /// \brief Locates the render model in world space.
  ///
  /// \param baseSpace The reference space to locate relative to.
  /// \param time The time for location.
  /// \param out_transform The output transform.
  /// \return True if the location is valid.
  bool LocateModel(XrSpace baseSpace, XrTime time, ezTransform& out_transform) const;

private:
  friend class ezOpenXRRenderModelManager;

  XrRenderModelIdEXT m_RenderModelId = 0;
  XrRenderModelEXT m_RenderModelHandle = XR_NULL_HANDLE;
  XrRenderModelAssetEXT m_AssetHandle = XR_NULL_HANDLE;
  XrSpace m_ModelSpace = XR_NULL_HANDLE;

  ezUInt32 m_uiAnimatableNodeCount = 0;

  ezDynamicArray<ezOpenXRRenderModelNode> m_Nodes;
  ezDynamicArray<ezOpenXRRenderModelPrimitive> m_Primitives;
  ezDynamicArray<ezOpenXRParsedMesh> m_ParsedMeshes;

  // Reference to owning singleton for API calls
  ezOpenXR* m_pOpenXR = nullptr;
};

