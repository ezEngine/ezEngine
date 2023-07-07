#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Utils/BlackboardTemplateResource.h>
#include <GameEngine/Volumes/VolumeComponent.h>

namespace
{
  ezSpatialData::Category s_VolumeCategory = ezSpatialData::RegisterCategory("GenericVolume", ezSpatialData::Flags::None);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezVolumeComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Template", GetTemplateFile, SetTemplateFile)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_BlackboardTemplate")),
    EZ_ACCESSOR_PROPERTY("SortOrder", GetSortOrder, SetSortOrder)->AddAttributes(new ezClampValueAttribute(-64.0f, 64.0f)),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Gameplay"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_ABSTRACT_COMPONENT_TYPE
// clang-format on
