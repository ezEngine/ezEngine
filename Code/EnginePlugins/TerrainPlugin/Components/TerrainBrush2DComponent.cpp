#include <TerrainPlugin/TerrainPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <TerrainPlugin/Components/TerrainBrush2DComponent.h>
#include <TerrainPlugin/Components/TerrainBrushAttributes.h>
#include <TerrainPlugin/TerrainSystem.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezTerrainModifyMode2D, 1)
  EZ_ENUM_CONSTANT(ezTerrainModifyMode2D::OnlyPaint2D),
  EZ_ENUM_CONSTANT(ezTerrainModifyMode2D::Max),
  EZ_ENUM_CONSTANT(ezTerrainModifyMode2D::Min),
  EZ_ENUM_CONSTANT(ezTerrainModifyMode2D::Set),
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_COMPONENT_TYPE(ezTerrainBrush2DComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_ACCESSOR_PROPERTY("ModifyMode", ezTerrainModifyMode2D, GetModifyMode, SetModifyMode),
    EZ_ACCESSOR_PROPERTY("HalfSizeX", GetHalfSizeX, SetHalfSizeX)->AddAttributes(new ezDefaultValueAttribute(0.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("HalfSizeY", GetHalfSizeY, SetHalfSizeY)->AddAttributes(new ezDefaultValueAttribute(0.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("InnerRadius", GetInnerRadius, SetInnerRadius)->AddAttributes(new ezDefaultValueAttribute(0.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("OuterRadius", GetOuterRadius, SetOuterRadius)->AddAttributes(new ezDefaultValueAttribute(5.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("Falloff", GetFalloff, SetFalloff)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.1f, 10.0f)),
    EZ_ACCESSOR_PROPERTY("NoiseStrength", GetNoiseStrength, SetNoiseStrength)->AddAttributes(new ezDefaultValueAttribute(0.0f), new ezClampValueAttribute(0.0f, 10.0f)),
    EZ_ACCESSOR_PROPERTY("NoiseFrequency", GetNoiseFrequency, SetNoiseFrequency)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(1.0f, 100.0f)),
    EZ_ACCESSOR_PROPERTY("MaterialIndex", GetMaterialIndex, SetMaterialIndex)->AddAttributes(new ezClampValueAttribute(0, 15)),
    EZ_ACCESSOR_PROPERTY("MaterialStrength", GetMaterialStrength, SetMaterialStrength)->AddAttributes(new ezDefaultValueAttribute(0.0f), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_ACCESSOR_PROPERTY("Priority", GetPriority, SetPriority)->AddAttributes(new ezDefaultValueAttribute((ezInt8)0), new ezClampValueAttribute((ezInt8)-128, (ezInt8)127)),
    EZ_SET_ACCESSOR_PROPERTY("TerrainTags", GetTags, Reflection_SetTag, Reflection_RemoveTag)->AddAttributes(new ezTagSetWidgetAttribute("Terrain")),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Terrain"),
    new ezShapeIconAlwaysVisibleAttribute(),
    new ezTerrainBrush2DVisualizerAttribute("HalfSizeX", "HalfSizeY", "InnerRadius", ezColor::Yellow, "OuterRadius", ezColor::GreenYellow, ezVec3(0, 0, 0)),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE;
// clang-format on

ezTerrainBrush2DComponent::ezTerrainBrush2DComponent() = default;
ezTerrainBrush2DComponent::~ezTerrainBrush2DComponent() = default;

void ezTerrainBrush2DComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
  s << m_ModifyMode;
  s << m_fHalfSizeY;
}

void ezTerrainBrush2DComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
  s >> m_ModifyMode;
  s >> m_fHalfSizeY;
}

void ezTerrainBrush2DComponent::SetModifyMode(ezEnum<ezTerrainModifyMode2D> mode)
{
  if (m_ModifyMode == mode)
    return;
  m_ModifyMode = mode;
  RefreshBrushes();
}

void ezTerrainBrush2DComponent::SetHalfSizeY(float fSize)
{
  if (m_fHalfSizeY == fSize)
    return;
  m_fHalfSizeY = fSize;
  RefreshBrushes();
}

void ezTerrainBrush2DComponent::FillBrushSpecificProperties(ezTerrainData_Brush& brush, float /*fHalfSizeX*/)
{
  brush.m_ModifyMode = static_cast<ezTerrainModifyMode::Enum>(m_ModifyMode.GetValue());
  brush.m_vHalfExtents.y = m_fHalfSizeY;
  brush.m_fHalfExtentYTop = 0.0f;
  brush.m_fHalfExtentZ = 0.0f;
}
