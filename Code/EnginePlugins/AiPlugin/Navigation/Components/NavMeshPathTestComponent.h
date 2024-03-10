#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <AiPlugin/Navigation/Navigation.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>

using ezNavMeshPathTestComponentManager = ezComponentManagerSimple<class ezAiNavMeshPathTestComponent, ezComponentUpdateType::WhenSimulating>;

/// \brief Used to test path-finding through a navmesh.
///
/// The component takes a reference to another game object as the destination
/// and then requests a path from the navmesh.
/// Various aspects of the path can be visualized for inspection.
///
/// This component should be used in the editor, to test whether the scene navmesh behaves as desired.
class EZ_AIPLUGIN_DLL ezAiNavMeshPathTestComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezAiNavMeshPathTestComponent, ezComponent, ezNavMeshPathTestComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  //  ezAiNavMeshPathTestComponent

public:
  ezAiNavMeshPathTestComponent();
  ~ezAiNavMeshPathTestComponent();

  void SetPathEndReference(const char* szReference); // [ property ]
  void SetPathEnd(ezGameObjectHandle hObject);

  /// \brief Render the navmesh polygons, through which the path goes.
  bool m_bVisualizePathCorridor = true; // [ property ]

  /// \brief Render a line for the shortest path through the corridor.
  bool m_bVisualizePathLine = true; // [ property ]

  /// \brief Render text describing what went wrong during path search.
  bool m_bVisualizePathState = true; // [ property ]

  /// \brief Name of the ezAiNavmeshConfig to use. See ezAiNavigationConfig.
  ezHashedString m_sNavmeshConfig; // [ property ]

  /// \brief Name of the ezAiPathSearchConfig to use. See ezAiNavigationConfig.
  ezHashedString m_sPathSearchConfig; // [ property ]

protected:
  void Update();

  ezGameObjectHandle m_hPathEnd;
  ezAiNavigation m_Navigation;

private:
  const char* DummyGetter() const { return nullptr; }
};
