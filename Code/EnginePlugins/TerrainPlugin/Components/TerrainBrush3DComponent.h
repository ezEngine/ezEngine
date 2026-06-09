#pragma once

#include <TerrainPlugin/Components/TerrainBrushBaseComponent.h>

/// Brush modes available on a 3D (volumetric) terrain brush.
struct EZ_TERRAINPLUGIN_DLL ezTerrainModifyMode3D
{
  using StorageType = ezUInt8;
  enum Enum : ezUInt8
  {
    Carve = 3,
    Add = 4,
    OnlyPaint3D = 6,
    Default = Carve,
  };
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_TERRAINPLUGIN_DLL, ezTerrainModifyMode3D);

using ezTerrainBrush3DComponentManager = ezComponentManager<class ezTerrainBrush3DComponent, ezBlockStorageType::Compact>;

/// 3D (volumetric) terrain brush.
///
/// The footprint is a 3D rounded box oriented by the owner object's rotation.
/// HalfSizeX / HalfSizeYBottom / HalfSizeYTop / HalfSizeZ control the box extents.
/// Setting HalfSizeYTop=0 keeps it equal to HalfSizeYBottom (symmetric).
/// Setting HalfSizeYTop=0 and HalfSizeYBottom to a positive value with a non-zero InnerRadius
/// produces an arch cross-section suitable for natural tunnels with flat floors.
///
/// If an ezSplineComponent exists on the same game object the brush stamps along the spline instead of
/// acting as a single point.
class EZ_TERRAINPLUGIN_DLL ezTerrainBrush3DComponent : public ezTerrainBrushBaseComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezTerrainBrush3DComponent, ezTerrainBrushBaseComponent, ezTerrainBrush3DComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezTerrainBrush3DComponent

public:
  ezTerrainBrush3DComponent();
  ~ezTerrainBrush3DComponent();

  void SetModifyMode(ezEnum<ezTerrainModifyMode3D> mode);                      //< [ property ]
  ezEnum<ezTerrainModifyMode3D> GetModifyMode() const { return m_ModifyMode; } //< [ property ]

  void SetHalfSizeYBottom(float fSize);                                        //< [ property ]
  float GetHalfSizeYBottom() const { return m_fHalfSizeYBottom; }              //< [ property ]

  /// Upper Y half-size. 0 = same as HalfSizeYBottom (symmetric box).
  void SetHalfSizeYTop(float fSize);                        //< [ property ]
  float GetHalfSizeYTop() const { return m_fHalfSizeYTop; } //< [ property ]

  void SetHalfSizeZ(float fSize);                           //< [ property ]
  float GetHalfSizeZ() const { return m_fHalfSizeZ; }       //< [ property ]

protected:
  virtual void FillBrushSpecificProperties(ezTerrainData_Brush& brush, float fHalfSizeX) override;

  ezEnum<ezTerrainModifyMode3D> m_ModifyMode;
  float m_fHalfSizeYBottom = 0.0f;
  float m_fHalfSizeYTop = 0.0f;
  float m_fHalfSizeZ = 0.0f;
};
