#pragma once

#include <EditorPluginAssets/EditorPluginAssetsDLL.h>
#include <Foundation/Strings/StringView.h>
#include <Foundation/Types/Enum.h>
#include <QStringList>
#include <RendererCore/RenderContext/Implementation/RenderContextStructs.h>
#include <Texture/TexConv/TexConvEnums.h>

class ezAssetFileHeader;
class ezStringBuilder;

// Conversion of texture asset enums to the strings that ezTexConv expects on its command line.
// Shared by the texture asset types (2D, 3D, cube).
const char* ToWrapMode(ezImageAddressMode::Enum mode);
const char* ToFilterMode(ezTextureFilterSetting::Enum mode);
const char* ToUsageMode(ezTexConvUsage::Enum mode);
const char* ToMipmapMode(ezTexConvMipmapMode::Enum mode);
const char* ToCompressionMode(ezTexConvCompressionMode::Enum mode);

/// ezTexConv settings shared by every texture asset type, used by AppendCommonTexConvArguments().
struct ezTexConvCommonSettings
{
  ezEnum<ezTexConvMipmapMode> m_MipmapMode;
  ezEnum<ezTexConvCompressionMode> m_CompressionMode;
  ezEnum<ezTexConvUsage> m_TextureUsage;
  float m_fHdrExposureBias = 0;
  ezUInt16 m_uiMaxResolution = 0;
  ezUInt8 m_uiDownscaleSteps = 0;
  ezEnum<ezImageAddressMode> m_AddressModeU;
  ezEnum<ezImageAddressMode> m_AddressModeV;
  ezEnum<ezImageAddressMode> m_AddressModeW;
  ezEnum<ezTextureFilterSetting> m_TextureFilter;
};

/// Appends the ezTexConv command line arguments common to every texture asset type.
///
/// Covers asset version and hash, output, asset info and thumbnail paths, mipmap, compression and usage settings,
/// HDR exposure bias, max resolution, address modes and texture filtering.
/// The caller has to append the type specific arguments (-type, input files, channel mapping) itself.
/// The thumbnail directory is created when bUpdateThumbnail is set.
void AppendCommonTexConvArguments(QStringList& inout_arguments, ezStringBuilder& ref_sTemp, const ezAssetFileHeader& assetHeader, const char* szTargetFile, ezStringView sThumbnailFile, bool bUpdateThumbnail, const ezTexConvCommonSettings& settings);
