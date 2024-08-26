#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/World/Component.h>
#include <Core/World/World.h>

using ezMarkerComponentManager = ezComponentManager<class ezMarkerComponent, ezBlockStorageType::Compact>;

/// \brief This component is used to markup objects and locations with gameplay relevant semantical information.
///
/// Objects with marker components can be found through the ezSpatialSystem, making it easy to find all marked objects
/// in a certain area.
///
/// Markers use different spatial categories (\see ezSpatialData::RegisterCategory()) making it efficient to search
/// only for very specific objects.
///
/// Markers can be used for all sorts of gameplay functionality, usually for AI systems to be able to detect which objects
/// they can interact with.
class EZ_GAMEENGINE_DLL ezMarkerComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezMarkerComponent, ezComponent, ezMarkerComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezMarkerComponent

public:
  ezMarkerComponent();
  ~ezMarkerComponent();

  /// \brief The marker type is passed into ezSpatialData::RegisterCategory() so that these markers can be found through the ezSpatialSystem.
  void SetMarkerType(const char* szType); // [ property ]
  const char* GetMarkerType() const;      // [ property ]

  /// \brief The size of the marker.
  ///
  /// Often this can be very small to just mark a point, but it may be larger to represent the size of the marked object.
  void SetRadius(float fRadius);                                  // [ property ]
  float GetRadius() const;                                        // [ property ]

protected:
  void OnMsgUpdateLocalBounds(ezMsgUpdateLocalBounds& msg) const; // [ msg handler ]
  void UpdateMarker();

  float m_fRadius = 0.1f;                                         // [ property ]
  ezHashedString m_sMarkerType;                                   // [ property ]

  ezSpatialData::Category m_SpatialCategory = ezInvalidSpatialDataCategory;
};
