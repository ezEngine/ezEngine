#include <PCH.h>
#include <EditorPluginProceduralPlacement/ProceduralPlacementAsset/ProceduralPlacementNodes.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProceduralPlacementNodeBase, 1, ezRTTINoAllocator)
{
  flags.Add(ezTypeFlags::Abstract);
}
EZ_END_DYNAMIC_REFLECTED_TYPE

//////////////////////////////////////////////////////////////////////////

void ezProceduralPlacementLayerOutput::Save(ezStreamWriter& stream)
{
  stream << m_sName;

  stream << m_ObjectsToPlace.GetCount();
  for (auto& object : m_ObjectsToPlace)
  {
    stream << object;
  }

  stream << m_fFootprint;

  stream << m_vMinOffset;
  stream << m_vMaxOffset;

  stream << m_fAlignToNormal;

  stream << m_vMinScale;
  stream << m_vMaxScale;

  stream << m_fCullDistance;
}

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProceduralPlacementLayerOutput, 1, ezRTTIDefaultAllocator<ezProceduralPlacementLayerOutput>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Name", m_sName),
    EZ_ARRAY_MEMBER_PROPERTY("Objects", m_ObjectsToPlace)->AddAttributes(new ezAssetBrowserAttribute("Prefab")),
    EZ_MEMBER_PROPERTY("Footprint", m_fFootprint)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("MinOffset", m_vMinOffset),
    EZ_MEMBER_PROPERTY("MaxOffset", m_vMaxOffset),
    EZ_MEMBER_PROPERTY("AlignToNormal", m_fAlignToNormal)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_MEMBER_PROPERTY("MinScale", m_vMinScale)->AddAttributes(new ezDefaultValueAttribute(ezVec3(1.0f)), new ezClampValueAttribute(ezVec3(0.0f), ezVariant())),
    EZ_MEMBER_PROPERTY("MaxScale", m_vMaxScale)->AddAttributes(new ezDefaultValueAttribute(ezVec3(1.0f)), new ezClampValueAttribute(ezVec3(0.0f), ezVariant())),
    EZ_MEMBER_PROPERTY("CullDistance", m_fCullDistance)->AddAttributes(new ezDefaultValueAttribute(100.0f), new ezClampValueAttribute(0.0f, ezVariant())),

    EZ_MEMBER_PROPERTY("Density", m_DensityPin),
    EZ_MEMBER_PROPERTY("Scale", m_ScalePin),
    EZ_MEMBER_PROPERTY("ColorIndex", m_ColorIndexPin),
    EZ_MEMBER_PROPERTY("ObjectIndex", m_ObjectIndexPin)
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProceduralPlacementRandom, 1, ezRTTIDefaultAllocator<ezProceduralPlacementRandom>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Value", m_OutputValuePin)
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

