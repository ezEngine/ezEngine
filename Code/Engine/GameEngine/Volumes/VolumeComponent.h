#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/World.h>
#include <Foundation/Types/RangeView.h>
#include <GameEngine/GameEngineDLL.h>

struct ezMsgUpdateLocalBounds;

using ezBlackboardTemplateResourceHandle = ezTypedResourceHandle<class ezBlackboardTemplateResource>;

/// \brief A volume component can hold generic values either from a blackboard template or set directly on the component.
///
/// The values can be sampled with an ezVolumeSampler and then used for things like e.g. post-processing, reverb etc.
/// They can also be used to represent knowledge in a scene, like e.g. smell or threat, and can be detected by an ezSensorComponent and then processed by AI.
class EZ_GAMEENGINE_DLL ezVolumeComponent : public ezComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezVolumeComponent, ezComponent);

public:
  ezVolumeComponent();
  ~ezVolumeComponent();

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  void SetTemplateFile(const char* szFile); // [ property ]
  const char* GetTemplateFile() const;      // [ property ]

  void SetTemplate(const ezBlackboardTemplateResourceHandle& hResource);
  ezBlackboardTemplateResourceHandle GetTemplate() const { return m_hTemplateResource; }

  void SetSortOrder(float fOrder);                    // [ property ]
  float GetSortOrder() const { return m_fSortOrder; } // [ property ]

  void SetVolumeType(const char* szType); // [ property ]
  const char* GetVolumeType() const;      // [ property ]

  void SetValue(const ezHashedString& sName, const ezVariant& value); // [ scriptable ]
  ezVariant GetValue(ezTempHashedString sName) const                  // [ scriptable ]
  {
    ezVariant v;
    m_Values.TryGetValue(sName, v);
    return v;
  }

  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  const ezRangeView<const ezString&, ezUInt32> Reflection_GetKeys() const;
  bool Reflection_GetValue(const char* szName, ezVariant& value) const;
  void Reflection_InsertValue(const char* szName, const ezVariant& value);
  void Reflection_RemoveValue(const char* szName);

  void InitializeFromTemplate();
  void ReloadTemplate();
  void RemoveReloadFunction();

  ezBlackboardTemplateResourceHandle m_hTemplateResource;
  ezHashTable<ezHashedString, ezVariant> m_Values;
  ezSmallArray<ezHashedString, 1> m_OverwrittenValues; // only used in editor
  float m_fSortOrder = 0.0f;
  ezSpatialData::Category m_SpatialCategory = ezInvalidSpatialDataCategory;
  bool m_bReloadFunctionAdded = false;
};

//////////////////////////////////////////////////////////////////////////

using ezVolumeSphereComponentManager = ezComponentManager<class ezVolumeSphereComponent, ezBlockStorageType::Compact>;

class EZ_GAMEENGINE_DLL ezVolumeSphereComponent : public ezVolumeComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezVolumeSphereComponent, ezVolumeComponent, ezVolumeSphereComponentManager);

public:
  ezVolumeSphereComponent();
  ~ezVolumeSphereComponent();

  float GetRadius() const { return m_fRadius; }
  void SetRadius(float fRadius);

  float GetFalloff() const { return m_fFalloff; }
  void SetFalloff(float fFalloff);

  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& ref_msg) const;

protected:
  float m_fRadius = 5.0f;
  float m_fFalloff = 0.5f;
};

//////////////////////////////////////////////////////////////////////////

using ezVolumeBoxComponentManager = ezComponentManager<class ezVolumeBoxComponent, ezBlockStorageType::Compact>;

class EZ_GAMEENGINE_DLL ezVolumeBoxComponent : public ezVolumeComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezVolumeBoxComponent, ezVolumeComponent, ezVolumeBoxComponentManager);

public:
  ezVolumeBoxComponent();
  ~ezVolumeBoxComponent();

  const ezVec3& GetExtents() const { return m_vExtents; }
  void SetExtents(const ezVec3& vExtents);

  const ezVec3& GetFalloff() const { return m_vFalloff; }
  void SetFalloff(const ezVec3& vFalloff);

  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& ref_msg) const;

protected:
  ezVec3 m_vExtents = ezVec3(10.0f);
  ezVec3 m_vFalloff = ezVec3(0.5f);
};
