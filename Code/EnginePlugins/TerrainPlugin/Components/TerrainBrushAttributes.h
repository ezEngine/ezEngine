#pragma once

#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <TerrainPlugin/TerrainPluginDLL.h>

/// Visualizer attribute for a 2D rounded rectangle brush shape in the XY plane.
///
/// Used on brush components to render an inner and outer rounded-rectangle outline around the brush area.
class EZ_TERRAINPLUGIN_DLL ezTerrainBrush2DVisualizerAttribute : public ezVisualizerAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTerrainBrush2DVisualizerAttribute, ezVisualizerAttribute);

public:
  ezTerrainBrush2DVisualizerAttribute();
  ezTerrainBrush2DVisualizerAttribute(const char* szHalfSizeXProp, const char* szHalfSizeYProp, const char* szInnerRadiusProp, const ezColor& innerColor, const char* szOuterRadiusProp, const ezColor& outerColor, ezVec3 vLocalOffset);

  const ezUntrackedString& GetPropHalfSizeX() const { return m_sProperty1; }
  const ezUntrackedString& GetPropHalfSizeY() const { return m_sProperty2; }
  const ezUntrackedString& GetPropInnerRadius() const { return m_sProperty3; }
  const ezUntrackedString& GetPropOuterRadius() const { return m_sProperty4; }

  ezColor m_InnerColor = ezColor::White;
  ezColor m_OuterColor = ezColor::White;
  ezVec3 m_vOffset = ezVec3::MakeZero();
};

//////////////////////////////////////////////////////////////////////////

/// Visualizer attribute for a 3D rounded box brush shape.
///
/// The box spans [-HalfSizeX, +HalfSizeX] in X, [-HalfSizeYBottom, +HalfSizeYTop] in Y, and [-HalfSizeZ, +HalfSizeZ] in Z.
/// Top and bottom faces are drawn as rounded rectangles; four vertical edges connect them.
/// When OuterRadius > 0, a second outline is drawn using (InnerRadius + OuterRadius) as the corner radius.
class EZ_TERRAINPLUGIN_DLL ezTerrainBrush3DVisualizerAttribute : public ezVisualizerAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTerrainBrush3DVisualizerAttribute, ezVisualizerAttribute);

public:
  ezTerrainBrush3DVisualizerAttribute();
  ezTerrainBrush3DVisualizerAttribute(const char* szHalfSizeXProp, const char* szHalfSizeYBottomProp, const char* szHalfSizeYTopProp, const char* szHalfSizeZProp, const char* szInnerRadiusProp, const ezColor& innerColor, const char* szOuterRadiusProp, const ezColor& outerColor, ezVec3 vLocalOffset);

  const ezUntrackedString& GetPropHalfSizeX() const { return m_sProperty1; }
  const ezUntrackedString& GetPropHalfSizeYBottom() const { return m_sProperty2; }
  const ezUntrackedString& GetPropHalfSizeYTop() const { return m_sProperty3; }
  const ezUntrackedString& GetPropHalfSizeZ() const { return m_sProperty4; }
  const ezUntrackedString& GetPropInnerRadius() const { return m_sProperty5; }
  const ezUntrackedString& GetPropOuterRadius() const { return m_sProperty6; }

  ezColor m_InnerColor = ezColor::White;
  ezColor m_OuterColor = ezColor::White;
  ezVec3 m_vOffset = ezVec3::MakeZero();
};
