#pragma once

#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Strings/String.h>
#include <Texture/TexConv/TexConvEnums.h>

/// \brief Describes how to create a texture atlas from multiple input images.
///
/// A texture atlas packs multiple images into a single texture for improved rendering performance.
/// This structure defines the layers (different image types like diffuse, normal maps) and items
/// (individual images to pack) that will be combined.
///
/// **Workflow:**
/// 1. Create ezTextureAtlasCreationDesc with desired layers and items
/// 2. Use texture conversion system to generate the actual atlas
/// 3. Load the resulting ezTextureAtlasRuntimeDesc for runtime access
///
/// **Example:**
/// ```cpp
/// ezTextureAtlasCreationDesc desc;
///
/// // Add a diffuse layer
/// auto& diffuseLayer = desc.m_Layers.ExpandAndGetRef();
/// diffuseLayer.m_Usage = ezTexConvUsage::Color;
/// diffuseLayer.m_uiNumChannels = 4;
///
/// // Add an item (sprite/icon)
/// auto& item = desc.m_Items.ExpandAndGetRef();
/// item.m_uiUniqueID = 100;
/// item.m_sLayerInput[0] = "icon_sword.png";
/// ```
struct EZ_TEXTURE_DLL ezTextureAtlasCreationDesc
{
  /// \brief Defines a single layer in the texture atlas (e.g., diffuse, normal, roughness).
  struct Layer
  {
    ezEnum<ezTexConvUsage> m_Usage;
    ezUInt8 m_uiNumChannels = 4;
  };

  /// \brief Represents one item (image) to be packed into the atlas.
  struct Item
  {
    ezUInt32 m_uiUniqueID;
    ezUInt32 m_uiFlags;
    ezString m_sAlphaInput;
    ezString m_sLayerInput[4];
  };

  ezHybridArray<Layer, 4> m_Layers;
  ezDynamicArray<Item> m_Items;

  ezResult Serialize(ezStreamWriter& inout_stream) const;
  ezResult Deserialize(ezStreamReader& inout_stream);

  ezResult Save(ezStringView sFile) const;
  ezResult Load(ezStringView sFile);
};

/// \brief Runtime data for efficiently accessing items within a generated texture atlas.
///
/// After a texture atlas is created and processed, this structure provides the information
/// needed to find and render individual items from the packed atlas texture. It maps unique
/// item IDs to their rectangular regions within each layer of the atlas.
///
/// **Runtime Usage:**
/// ```cpp
/// ezTextureAtlasRuntimeDesc atlas;
/// atlas.Load("MyAtlas.ezAtlas");
///
/// // Find an item by ID
/// auto item = atlas.m_Items.Find(itemID);
/// if (item.IsValid())
/// {
///   // Get UV coordinates for diffuse layer (layer 0)
///   ezRectU32 rect = item.Value().m_LayerRects[0];
///   // Convert to UV coordinates based on atlas texture size
/// }
/// ```
struct EZ_TEXTURE_DLL ezTextureAtlasRuntimeDesc
{
  struct Item
  {
    ezUInt32 m_uiFlags;
    ezRectU32 m_LayerRects[4];
  };

  ezUInt32 m_uiNumLayers = 0;
  ezArrayMap<ezUInt32, Item> m_Items;

  void Clear();

  ezResult Serialize(ezStreamWriter& inout_stream) const;
  ezResult Deserialize(ezStreamReader& inout_stream);
};
