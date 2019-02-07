#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Strings/String.h>
#include <Texture/TextureDLL.h>
#include <Texture/TexConv/TexConvEnums.h>

/// \brief Describes a list of texture groups
///
/// Used to pass information about what textures are supposed to make up a texture atlas.
/// E.g. each group describes a set of diffuse map, normal map, etc.
/// All groups together define which textures should be put into an atlas.
/// Interpretation of the group data and how to put them into the atlas is up to other code.
class EZ_TEXTURE_DLL ezTextureGroupDesc
{
public:
  ezTextureGroupDesc();
  ~ezTextureGroupDesc();

  struct TextureGroup
  {
    ezString m_sFilepaths[8];
  };

  ezResult Save(const char* szFile) const;
  ezResult Load(const char* szFile);

  ezDynamicArray<TextureGroup> m_Groups;
};


struct EZ_TEXTURE_DLL ezTextureAtlasLayerDesc
{
  ezEnum<ezTexConvUsage> m_Usage;
  ezDynamicArray<ezString> m_Filenames;

  ezResult Serialize(ezStreamWriter& stream) const;
  ezResult Deserialize(ezStreamReader& stream);
};

struct EZ_TEXTURE_DLL ezTextureAtlasDesc
{
  ezHybridArray<ezTextureAtlasLayerDesc, 4> m_Layers;

  ezResult Save(const char* szFile) const;
  ezResult Load(const char* szFile);
};
