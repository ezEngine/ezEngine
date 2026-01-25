#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/XR/XRInputDevice.h>
#include <GameEngine/XR/XRRenderModelInterface.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>

using ezControllerRenderModelComponentManager = ezComponentManagerSimple<class ezControllerRenderModelComponent, ezComponentUpdateType::WhenSimulating>;

/// \brief Renders a 3D model of an XR controller.
///
/// This component automatically retrieves and displays the controller model
/// provided by the XR runtime (e.g., OpenXR). It creates child game objects
/// for each part of the controller and animates them based on user input.
///
/// Usage:
/// 1. Add this component to a game object
/// 2. Set DeviceType to LeftController or RightController
/// 3. Optionally set a fallback material
/// 4. The component will automatically load and display the controller model
///
/// The component creates the following hierarchy:
/// - Owner object (positioned at controller grip pose)
///   - Node objects (one per glTF node)
///     - Mesh components (one per primitive)
///
class EZ_GAMEENGINE_DLL ezControllerRenderModelComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezControllerRenderModelComponent, ezComponent, ezControllerRenderModelComponentManager);

public:
  ezControllerRenderModelComponent();
  ~ezControllerRenderModelComponent();

  /// \brief Sets which controller to render.
  void SetDeviceType(ezEnum<ezXRDeviceType> type); // [ property ]
  ezEnum<ezXRDeviceType> GetDeviceType() const;    // [ property ]

  /// \brief Sets an optional fallback material to use when the runtime doesn't provide one.
  void SetFallbackMaterial(ezMaterialResourceHandle hMaterial); // [ property ]
  ezMaterialResourceHandle GetFallbackMaterial() const;         // [ property ]

  /// \brief Whether to hide the model when the controller is not tracked.
  void SetHideWhenNotTracked(bool bHide); // [ property ]
  bool GetHideWhenNotTracked() const;     // [ property ]

protected:
  //
  // ezComponent Interface
  //

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  void Update();

private:
  void LoadModel();
  void UnloadModel();
  void CreateNodeHierarchy();
  void UpdateNodeTransforms();
  void SetVisibility(bool bVisible);

  // Properties
  ezEnum<ezXRDeviceType> m_DeviceType = ezXRDeviceType::LeftController;
  ezMaterialResourceHandle m_hFallbackMaterial;
  bool m_bHideWhenNotTracked = true;

  // State
  ezXRRenderModelHandle m_ModelHandle = ezInvalidXRRenderModelHandle;
  bool m_bModelLoaded = false;
  bool m_bHierarchyCreated = false;
  bool m_bCurrentlyVisible = true;

  // Node data
  struct NodeInfo
  {
    ezGameObjectHandle m_hGameObject;
    ezInt32 m_iParentIndex = -1;
    bool m_bIsAnimatable = false;
    ezUInt32 m_uiAnimatableIndex = 0;
  };
  ezDynamicArray<NodeInfo> m_Nodes;

  // Cached interface
  ezXRRenderModelInterface* m_pRenderModelInterface = nullptr;
};

