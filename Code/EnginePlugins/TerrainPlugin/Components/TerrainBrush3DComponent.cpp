#include <TerrainPlugin/TerrainPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <TerrainPlugin/Components/TerrainBrush3DComponent.h>
#include <TerrainPlugin/Components/TerrainBrushAttributes.h>
#include <TerrainPlugin/TerrainSystem.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezTerrainModifyMode3D, 1)
  EZ_ENUM_CONSTANT(ezTerrainModifyMode3D::OnlyPaint3D),
  EZ_ENUM_CONSTANT(ezTerrainModifyMode3D::Carve),
  EZ_ENUM_CONSTANT(ezTerrainModifyMode3D::Add),
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_COMPONENT_TYPE(ezTerrainBrush3DComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_ACCESSOR_PROPERTY("ModifyMode", ezTerrainModifyMode3D, GetModifyMode, SetModifyMode),
    EZ_ACCESSOR_PROPERTY("HalfSizeX", GetHalfSizeX, SetHalfSizeX)->AddAttributes(new ezDefaultValueAttribute(0.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("HalfSizeYBottom", GetHalfSizeYBottom, SetHalfSizeYBottom)->AddAttributes(new ezDefaultValueAttribute(0.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("HalfSizeYTop", GetHalfSizeYTop, SetHalfSizeYTop)->AddAttributes(new ezDefaultValueAttribute(0.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("HalfSizeZ", GetHalfSizeZ, SetHalfSizeZ)->AddAttributes(new ezDefaultValueAttribute(0.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("InnerRadius", GetInnerRadius, SetInnerRadius)->AddAttributes(new ezDefaultValueAttribute(0.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("OuterRadius", GetOuterRadius, SetOuterRadius)->AddAttributes(new ezDefaultValueAttribute(5.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("Falloff", GetFalloff, SetFalloff)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.1f, 10.0f)),
    EZ_ACCESSOR_PROPERTY("NoiseStrength", GetNoiseStrength, SetNoiseStrength)->AddAttributes(new ezDefaultValueAttribute(0.0f), new ezClampValueAttribute(0.0f, 10.0f)),
    EZ_ACCESSOR_PROPERTY("NoiseFrequency", GetNoiseFrequency, SetNoiseFrequency)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(1.0f, 100.0f)),
    EZ_ACCESSOR_PROPERTY("MaterialIndex", GetMaterialIndex, SetMaterialIndex)->AddAttributes(new ezClampValueAttribute(0, 15)),
    EZ_ACCESSOR_PROPERTY("MaterialStrength", GetMaterialStrength, SetMaterialStrength)->AddAttributes(new ezDefaultValueAttribute(0.0f), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_ACCESSOR_PROPERTY("AffectPatches", GetAffectPatches, SetAffectPatches)->AddAttributes(new ezDefaultValueAttribute(false)),
    EZ_ACCESSOR_PROPERTY("AffectVolumes", GetAffectVolumes, SetAffectVolumes)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_ACCESSOR_PROPERTY("Priority", GetPriority, SetPriority)->AddAttributes(new ezDefaultValueAttribute((ezInt8)0), new ezClampValueAttribute((ezInt8)-128, (ezInt8)127)),
    EZ_SET_ACCESSOR_PROPERTY("TerrainTags", GetTags, Reflection_SetTag, Reflection_RemoveTag)->AddAttributes(new ezTagSetWidgetAttribute("Terrain")),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Terrain"),
    new ezShapeIconAlwaysVisibleAttribute(),
    new ezTerrainBrush3DVisualizerAttribute("HalfSizeX", "HalfSizeYBottom", "HalfSizeYTop", "HalfSizeZ", "InnerRadius", ezColor::Yellow, "OuterRadius", ezColor::GreenYellow, ezVec3(0, 0, 0)),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE;
// clang-format on

ezTerrainBrush3DComponent::ezTerrainBrush3DComponent() = default;
ezTerrainBrush3DComponent::~ezTerrainBrush3DComponent() = default;

void ezTerrainBrush3DComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
  s << m_ModifyMode;
  s << m_fHalfSizeYBottom;
  s << m_fHalfSizeYTop;
  s << m_fHalfSizeZ;
}

void ezTerrainBrush3DComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
  s >> m_ModifyMode;
  s >> m_fHalfSizeYBottom;
  s >> m_fHalfSizeYTop;
  s >> m_fHalfSizeZ;
}

void ezTerrainBrush3DComponent::SetModifyMode(ezEnum<ezTerrainModifyMode3D> mode)
{
  if (m_ModifyMode == mode)
    return;
  m_ModifyMode = mode;
  RefreshBrushes();
}

void ezTerrainBrush3DComponent::SetHalfSizeYBottom(float fSize)
{
  if (m_fHalfSizeYBottom == fSize)
    return;
  m_fHalfSizeYBottom = fSize;
  RefreshBrushes();
}

void ezTerrainBrush3DComponent::SetHalfSizeYTop(float fSize)
{
  if (m_fHalfSizeYTop == fSize)
    return;
  m_fHalfSizeYTop = fSize;
  RefreshBrushes();
}

void ezTerrainBrush3DComponent::SetHalfSizeZ(float fSize)
{
  if (m_fHalfSizeZ == fSize)
    return;
  m_fHalfSizeZ = fSize;
  RefreshBrushes();
}

void ezTerrainBrush3DComponent::FillBrushSpecificProperties(ezTerrainData_Brush& brush, float /*fHalfSizeX*/)
{
  brush.m_ModifyMode = static_cast<ezTerrainModifyMode::Enum>(m_ModifyMode.GetValue());
  brush.m_vHalfExtents.y = m_fHalfSizeYBottom;
  brush.m_fHalfExtentYTop = m_fHalfSizeYTop;
  brush.m_fHalfExtentZ = m_fHalfSizeZ;
}
