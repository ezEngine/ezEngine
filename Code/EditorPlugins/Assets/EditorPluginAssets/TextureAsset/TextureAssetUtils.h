#pragma once

#include <EditorPluginAssets/EditorPluginAssetsDLL.h>
#include <Foundation/Strings/StringView.h>
#include <Foundation/Types/Enum.h>
#include <QStringList>
#include <RendererCore/RenderContext/Implementation/RenderContextStructs.h>
#include <Texture/TexConv/TexConvEnums.h>

class ezAssetFileHeader;
class ezStringBuilder;

// Shared by every texture asset type's RunTexConv() (2D, 3D, cube, ...); see
// ezTextureAssetDocument::RunTexConv (TextureAsset.cpp) and
// ezTexture3DAssetDocument::RunTexConv (Texture3DAsset.cpp).
const char* ToWrapMode(ezImageAddressMode::Enum mode);
const char* ToFilterMode(ezTextureFilterSetting::Enum mode);
const char* ToUsageMode(ezTexConvUsage::Enum mode);
const char* ToMipmapMode(ezTexConvMipmapMode::Enum mode);
const char* ToCompressionMode(ezTexConvCompressionMode::Enum mode);

/// \brief ezTexConv settings shared by every texture asset type, used by AppendCommonTexConvArguments().
struct ezTexConvCommonSettings
{
  ezEnum<ezTexConvMipmapMode> m_MipmapMode;
  ezEnum<ezTexConvCompressionMode> m_CompressionMode;
  ezEnum<ezTexConvUsage> m_TextureUsage;
  float m_fHdrExposureBias = 0;
  ezUInt32 m_uiMaxResolution = 0;
  ezEnum<ezImageAddressMode> m_AddressModeU;
  ezEnum<ezImageAddressMode> m_AddressModeV;
  ezEnum<ezImageAddressMode> m_AddressModeW;
  ezEnum<ezTextureFilterSetting> m_TextureFilter;
};

/// \brief Appends the ezTexConv command line arguments common to every texture asset type: asset
/// version/hash, output + thumbnail paths, mipmap/compression/usage settings, HDR exposure bias,
/// max resolution, address modes and texture filtering. Callers append their own -type / input
/// file / channel-mapping arguments afterwards -- see ezTexture3DAssetDocument::RunTexConv
/// (Texture3DAsset.cpp) for an example of composing this with volume-texture-specific arguments.
void AppendCommonTexConvArguments(QStringList& inout_arguments, ezStringBuilder& ref_temp, const ezAssetFileHeader& assetHeader, const char* szTargetFile, ezStringView sThumbnailFile, bool bUpdateThumbnail, const ezTexConvCommonSettings& settings);
