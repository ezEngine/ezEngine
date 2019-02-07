#pragma once

#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Strings/String.h>
#include <Texture/TexConv/TexConvEnums.h>

struct EZ_TEXTURE_DLL ezTextureAtlasCreationDesc
{
  struct Layer
  {
    ezEnum<ezTexConvUsage> m_Usage;
    ezUInt8 m_uiNumChannels = 4;
  };

  struct Item
  {
    ezUInt32 m_uiUniqueID;
    ezString m_sLayerInput[4];
  };

  ezHybridArray<Layer, 4> m_Layers;
  ezDynamicArray<Item> m_Items;

  ezResult Serialize(ezStreamWriter& stream) const;
  ezResult Deserialize(ezStreamReader& stream);

  ezResult Save(const char* szFile) const;
  ezResult Load(const char* szFile);
};

struct EZ_TEXTURE_DLL ezTextureAtlasRuntimeDesc
{
  struct Item
  {
    ezRectU32 m_LayerRects[4];
  };

  ezUInt32 m_uiNumLayers = 0;
  ezArrayMap<ezUInt32, Item> m_Items;

  void Clear();

  ezResult Serialize(ezStreamWriter& stream) const;
  ezResult Deserialize(ezStreamReader& stream);
};
