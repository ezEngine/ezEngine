#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/World/Component.h>
#include <Core/World/World.h>

using ezMarkerComponentManager = ezComponentManager<class ezMarkerComponent, ezBlockStorageType::Compact>;

class EZ_GAMEENGINE_DLL ezMarkerComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezMarkerComponent, ezComponent, ezMarkerComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // ezMarkerComponent

public:
  ezMarkerComponent();
  ~ezMarkerComponent();

  void SetMarkerType(const char* szType); // [ property ]
  const char* GetMarkerType() const;      // [ property ]

  void SetRadius(float radius); // [ property ]
  float GetRadius() const;      // [ property ]

protected:
  void OnMsgUpdateLocalBounds(ezMsgUpdateLocalBounds& msg) const; // [ msg handler ]
  void UpdateMarker();

  float m_fRadius = 0.1f;       // [ property ]
  ezHashedString m_sMarkerType; // [ property ]

  ezSpatialData::Category m_SpatialCategory = ezInvalidSpatialDataCategory;
};
