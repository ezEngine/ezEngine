#pragma once

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <Foundation/Types/TagSet.h>
#include <TerrainPlugin/TerrainPluginDLL.h>
#include <TerrainPlugin/TerrainSystem.h>

struct ezMsgTransformChanged;
struct ezMsgSplineChanged;

/// Abstract base for all terrain brush components.
///
/// Handles creation and cleanup of TerrainSystem brushes, including automatic spline-path support:
/// if an ezSplineComponent exists on the same game object the brush stamps along the spline instead of
/// acting as a single brush. Properties common to 2D and 3D brushes are declared here and inherited by
/// both ezTerrainBrush2DComponent and ezTerrainBrush3DComponent.
class EZ_TERRAINPLUGIN_DLL ezTerrainBrushBaseComponent : public ezComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezTerrainBrushBaseComponent, ezComponent);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezTerrainBrushBaseComponent

public:
  ezTerrainBrushBaseComponent();
  ~ezTerrainBrushBaseComponent();

  void SetHalfSizeX(float fSize);                                //< [ property ]
  float GetHalfSizeX() const { return m_fHalfSizeX; }            //< [ property ]

  void SetInnerRadius(float fRadius);                            //< [ property ]
  float GetInnerRadius() const { return m_fInnerRadius; }        //< [ property ]

  void SetOuterRadius(float fRadius);                            //< [ property ]
  float GetOuterRadius() const { return m_fOuterRadius; }        //< [ property ]

  void SetFalloff(float fFalloff);                               //< [ property ]
  float GetFalloff() const { return m_fFalloff; }                //< [ property ]

  void SetMaterialIndex(ezUInt8 uiIndex);                        //< [ property ]
  ezUInt8 GetMaterialIndex() const { return m_uiMaterialIndex; } //< [ property ]

  /// Blend strength for material painting in [0, 1]. 0 = disabled.
  void SetMaterialStrength(float fStrength);                        //< [ property ]
  float GetMaterialStrength() const { return m_fMaterialStrength; } //< [ property ]

  void SetNoiseStrength(float fNoise);                              //< [ property ]
  float GetNoiseStrength() const { return m_fNoiseStrength; }       //< [ property ]

  void SetNoiseFrequency(float fNoise);                             //< [ property ]
  float GetNoiseFrequency() const { return m_fNoiseFrequency; }     //< [ property ]

  /// Brush application order. Higher = applied later = wins. Equal priorities use mode-based ordering.
  void SetPriority(ezInt8 iPriority);                //< [ property ]
  ezInt8 GetPriority() const { return m_iPriority; } //< [ property ]

  const ezTagSet& GetTags() const { return m_Tags; } //< [ property ]
  void Reflection_SetTag(const char* szTagName);     //< [ property ]
  void Reflection_RemoveTag(const char* szTagName);  //< [ property ]

protected:
  void OnMsgTransformChanged(ezMsgTransformChanged& msg);
  void OnMsgSplineChanged(ezMsgSplineChanged& msg);

  void RefreshBrushes();
  void ClearBrushes();

  /// Fills transform, HalfSizeX, and all common properties, then calls FillBrushSpecificProperties.
  void FillBrush(ezTerrainData_Brush& brush, const ezTransform& transform, float fHalfSizeX);

  /// Subclass fills: ModifyMode, m_vHalfExtents.y, m_fHalfExtentYTop, m_fHalfExtentZ.
  virtual void FillBrushSpecificProperties(ezTerrainData_Brush& brush, float fHalfSizeX) = 0;

  ezSmallArray<ezUInt32, 1> m_BrushIndices;

  float m_fHalfSizeX = 0.0f;
  float m_fInnerRadius = 0.0f;
  float m_fOuterRadius = 5.0f;
  float m_fFalloff = 1.0f;
  float m_fNoiseStrength = 0.0f;
  float m_fNoiseFrequency = 1.0f;
  float m_fMaterialStrength = 0.0f;
  ezUInt8 m_uiMaterialIndex = 0;
  ezInt8 m_iPriority = 0;
  /// If non-empty, brush only affects terrain objects that have at least one matching tag.
  ezTagSet m_Tags;
};
