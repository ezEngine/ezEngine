#pragma once

#include <TerrainPlugin/Components/TerrainBrushBaseComponent.h>

/// Brush modes available on a 2D (heightfield-style) terrain brush.
struct EZ_TERRAINPLUGIN_DLL ezTerrainModifyMode2D
{
  using StorageType = ezUInt8;
  enum Enum : ezUInt8
  {
    Max = 0,         ///< Only raises terrain toward the brush height.
    Min = 1,         ///< Only lowers terrain toward the brush height.
    Set = 2,         ///< Forces terrain to the brush height.
    OnlyPaint2D = 5, ///< Does not modify height values; paints material using 2D SDF projection.
    Default = Max,
  };
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_TERRAINPLUGIN_DLL, ezTerrainModifyMode2D);

using ezTerrainBrush2DComponentManager = ezComponentManager<class ezTerrainBrush2DComponent, ezBlockStorageType::Compact>;

/// 2D terrain brush: raises, lowers, or sets the heightfield surface.
///
/// The footprint is a rounded rectangle oriented by the owner object's rotation.
/// HalfSizeX/HalfSizeY control the straight-edge lengths; OuterRadius is the corner rounding radius.
/// Setting HalfSizeX=HalfSizeY=0 produces a circle; OuterRadius=0 gives a plain rectangle.
///
/// If an ezSplineComponent exists on the same game object the brush stamps along the spline instead of
/// acting as a single point.
class EZ_TERRAINPLUGIN_DLL ezTerrainBrush2DComponent : public ezTerrainBrushBaseComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezTerrainBrush2DComponent, ezTerrainBrushBaseComponent, ezTerrainBrush2DComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezTerrainBrush2DComponent

public:
  ezTerrainBrush2DComponent();
  ~ezTerrainBrush2DComponent();

  void SetModifyMode(ezEnum<ezTerrainModifyMode2D> mode);                      //< [ property ]
  ezEnum<ezTerrainModifyMode2D> GetModifyMode() const { return m_ModifyMode; } //< [ property ]

  void SetHalfSizeY(float fSize);                                              //< [ property ]
  float GetHalfSizeY() const { return m_fHalfSizeY; }                          //< [ property ]

protected:
  virtual void FillBrushSpecificProperties(ezTerrainData_Brush& brush, float fHalfSizeX) override;

  ezEnum<ezTerrainModifyMode2D> m_ModifyMode;
  float m_fHalfSizeY = 0.0f;
};
