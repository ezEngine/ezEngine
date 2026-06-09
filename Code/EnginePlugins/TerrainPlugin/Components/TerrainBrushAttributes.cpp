#include <TerrainPlugin/TerrainPluginPCH.h>

#include <TerrainPlugin/Components/TerrainBrushAttributes.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTerrainBrush2DVisualizerAttribute, 1, ezRTTIDefaultAllocator<ezTerrainBrush2DVisualizerAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("InnerColor", m_InnerColor),
    EZ_MEMBER_PROPERTY("OuterColor", m_OuterColor),
    EZ_MEMBER_PROPERTY("Offset", m_vOffset),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*, const ezColor&, const char*, const ezColor&, ezVec3),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezTerrainBrush2DVisualizerAttribute::ezTerrainBrush2DVisualizerAttribute()
  : ezVisualizerAttribute(nullptr)
{
}

ezTerrainBrush2DVisualizerAttribute::ezTerrainBrush2DVisualizerAttribute(const char* szHalfSizeXProp, const char* szHalfSizeYProp, const char* szInnerRadiusProp, const ezColor& innerColor, const char* szOuterRadiusProp, const ezColor& outerColor, ezVec3 vLocalOffset)
  : ezVisualizerAttribute(szHalfSizeXProp, szHalfSizeYProp, szInnerRadiusProp, szOuterRadiusProp)
  , m_InnerColor(innerColor)
  , m_OuterColor(outerColor)
  , m_vOffset(vLocalOffset)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTerrainBrush3DVisualizerAttribute, 1, ezRTTIDefaultAllocator<ezTerrainBrush3DVisualizerAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("InnerColor", m_InnerColor),
    EZ_MEMBER_PROPERTY("OuterColor", m_OuterColor),
    EZ_MEMBER_PROPERTY("Offset", m_vOffset),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*, const char*, const char*, const ezColor&, const char*, const ezColor&, ezVec3),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezTerrainBrush3DVisualizerAttribute::ezTerrainBrush3DVisualizerAttribute()
  : ezVisualizerAttribute(nullptr)
{
}

ezTerrainBrush3DVisualizerAttribute::ezTerrainBrush3DVisualizerAttribute(const char* szHalfSizeXProp, const char* szHalfSizeYBottomProp, const char* szHalfSizeYTopProp, const char* szHalfSizeZProp, const char* szInnerRadiusProp, const ezColor& innerColor, const char* szOuterRadiusProp, const ezColor& outerColor, ezVec3 vLocalOffset)
  : ezVisualizerAttribute(szHalfSizeXProp, szHalfSizeYBottomProp, szHalfSizeYTopProp, szHalfSizeZProp, szInnerRadiusProp, szOuterRadiusProp)
  , m_InnerColor(innerColor)
  , m_OuterColor(outerColor)
  , m_vOffset(vLocalOffset)
{
}
