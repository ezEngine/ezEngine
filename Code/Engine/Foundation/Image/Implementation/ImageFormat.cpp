#include <PCH.h>

#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Image/ImageFormat.h>

namespace
{
  struct ezImageFormatMetaData
  {
    ezImageFormatMetaData()
    {
      ezMemoryUtils::ZeroFill(m_uiBitsPerChannel);
      ezMemoryUtils::ZeroFill(m_uiChannelMasks);
    }

    const char* m_szName{nullptr};

    ezUInt16 m_uiBitsPerBlock{0}; ///< Bits per block for compressed formats; for uncompressed formats (which always have a block size of
                                  ///< 1x1x1), this is equal to bits per pixel.
    ezUInt8 m_uiBlockWidth{1};
    ezUInt8 m_uiBlockHeight{1};
    ezUInt8 m_uiBlockDepth{1};

    ezUInt8 m_uiNumChannels{0};

    ezUInt8 m_uiBitsPerChannel[ezImageFormatChannel::COUNT];
    ezUInt32 m_uiChannelMasks[ezImageFormatChannel::COUNT];


    bool m_requireFirstLevelBlockAligned{
        false}; ///< Only for compressed formats: If true, the first level's dimensions must be a multiple of the
                ///< block size; if false, padding can be applied for compressing the first mip level, too.
    bool m_isDepth{false};
    bool m_isStencil{false};

    ezImageFormatDataType::Enum m_dataType{ezImageFormatDataType::NONE};
    ezImageFormatType::Enum m_formatType{ezImageFormatType::UNKNOWN};

    ezImageFormat::Enum m_asLinear{ezImageFormat::UNKNOWN};
    ezImageFormat::Enum m_asSrgb{ezImageFormat::UNKNOWN};

    ezUInt32 getNumBlocksX(ezUInt32 width) const { return (width - 1) / m_uiBlockWidth + 1; }

    ezUInt32 getNumBlocksY(ezUInt32 height) const { return (height - 1) / m_uiBlockHeight + 1; }

    ezUInt32 getNumBlocksZ(ezUInt32 depth) const { return (depth - 1) / m_uiBlockDepth + 1; }

    ezUInt32 getRowPitch(ezUInt32 width) const { return getNumBlocksX(width) * m_uiBitsPerBlock / 8; }
  };

  ezStaticArray<ezImageFormatMetaData, ezImageFormat::NUM_FORMATS> s_formatMetaData;

  void InitFormatLinear(ezImageFormat::Enum format, const char* szName, ezImageFormatDataType::Enum dataType, ezUInt8 uiBitsPerPixel,
                        ezUInt8 uiBitsR, ezUInt8 uiBitsG, ezUInt8 uiBitsB, ezUInt8 uiBitsA, ezUInt8 uiNumChannels)
  {
    s_formatMetaData[format].m_szName = szName;

    s_formatMetaData[format].m_uiBitsPerBlock = uiBitsPerPixel;
    s_formatMetaData[format].m_dataType = dataType;
    s_formatMetaData[format].m_formatType = ezImageFormatType::LINEAR;

    s_formatMetaData[format].m_uiNumChannels = uiNumChannels;

    s_formatMetaData[format].m_uiBitsPerChannel[ezImageFormatChannel::R] = uiBitsR;
    s_formatMetaData[format].m_uiBitsPerChannel[ezImageFormatChannel::G] = uiBitsG;
    s_formatMetaData[format].m_uiBitsPerChannel[ezImageFormatChannel::B] = uiBitsB;
    s_formatMetaData[format].m_uiBitsPerChannel[ezImageFormatChannel::A] = uiBitsA;

    s_formatMetaData[format].m_asLinear = format;
    s_formatMetaData[format].m_asSrgb = format;
  }

#define INIT_FORMAT_LINEAR(format, dataType, uiBitsPerPixel, uiBitsR, uiBitsG, uiBitsB, uiBitsA, uiNumChannels)                            \
  InitFormatLinear(ezImageFormat::format, #format, ezImageFormatDataType::dataType, uiBitsPerPixel, uiBitsR, uiBitsG, uiBitsB, uiBitsA,    \
                   uiNumChannels)

  void InitFormatCompressed(ezImageFormat::Enum format, const char* szName, ezImageFormatDataType::Enum dataType, ezUInt16 uiBitsPerBlock,
                            ezUInt8 uiBlockWidth, ezUInt8 uiBlockHeight, ezUInt8 uiBlockDepth, bool requireFirstLevelBlockAligned,
                            ezUInt8 uiNumChannels)
  {
    s_formatMetaData[format].m_szName = szName;

    s_formatMetaData[format].m_uiBitsPerBlock = uiBitsPerBlock;
    s_formatMetaData[format].m_uiBlockWidth = uiBlockWidth;
    s_formatMetaData[format].m_uiBlockHeight = uiBlockHeight;
    s_formatMetaData[format].m_uiBlockDepth = uiBlockDepth;
    s_formatMetaData[format].m_dataType = dataType;
    s_formatMetaData[format].m_formatType = ezImageFormatType::BLOCK_COMPRESSED;

    s_formatMetaData[format].m_uiNumChannels = uiNumChannels;

    s_formatMetaData[format].m_requireFirstLevelBlockAligned = requireFirstLevelBlockAligned;

    s_formatMetaData[format].m_asLinear = format;
    s_formatMetaData[format].m_asSrgb = format;
  }

#define INIT_FORMAT_COMPRESSED(format, dataType, uiBitsPerBlock, uiBlockWidth, uiBlockHeight, uiBlockDepth, requireFirstLevelBlockAligned, \
                               uiNumChannels)                                                                                              \
  InitFormatCompressed(ezImageFormat::format, #format, ezImageFormatDataType::dataType, uiBitsPerBlock, uiBlockWidth, uiBlockHeight,       \
                       uiBlockDepth, requireFirstLevelBlockAligned, uiNumChannels)

  void InitFormatDepth(ezImageFormat::Enum format, const char* szName, ezImageFormatDataType::Enum dataType, ezUInt8 uiBitsPerPixel,
                       bool isStencil, ezUInt8 uiBitsD, ezUInt8 uiBitsS)
  {
    s_formatMetaData[format].m_szName = szName;

    s_formatMetaData[format].m_uiBitsPerBlock = uiBitsPerPixel;
    s_formatMetaData[format].m_dataType = dataType;
    s_formatMetaData[format].m_formatType = ezImageFormatType::LINEAR;

    s_formatMetaData[format].m_isDepth = true;
    s_formatMetaData[format].m_isStencil = isStencil;

    s_formatMetaData[format].m_uiNumChannels = isStencil ? 2 : 1;

    s_formatMetaData[format].m_uiBitsPerChannel[ezImageFormatChannel::D] = uiBitsD;
    s_formatMetaData[format].m_uiBitsPerChannel[ezImageFormatChannel::S] = uiBitsS;

    s_formatMetaData[format].m_asLinear = format;
    s_formatMetaData[format].m_asSrgb = format;
  }

#define INIT_FORMAT_DEPTH(format, dataType, uiBitsPerPixel, isStencil, uiBitsD, uiBitsS)                                                   \
  InitFormatDepth(ezImageFormat::format, #format, ezImageFormatDataType::dataType, uiBitsPerPixel, isStencil, uiBitsD, uiBitsS);

  void SetupSrgbPair(ezImageFormat::Enum linearFormat, ezImageFormat::Enum srgbFormat)
  {
    s_formatMetaData[linearFormat].m_asLinear = linearFormat;
    s_formatMetaData[linearFormat].m_asSrgb = srgbFormat;

    s_formatMetaData[srgbFormat].m_asLinear = linearFormat;
    s_formatMetaData[srgbFormat].m_asSrgb = srgbFormat;
  }

} // namespace

static void SetupImageFormatTable()
{
  if (!s_formatMetaData.IsEmpty())
    return;

  s_formatMetaData.SetCount(ezImageFormat::NUM_FORMATS);

  s_formatMetaData[ezImageFormat::UNKNOWN].m_szName = "UNKNOWN";

  INIT_FORMAT_LINEAR(R32G32B32A32_FLOAT, FLOAT, 128, 32, 32, 32, 32, 4);
  INIT_FORMAT_LINEAR(R32G32B32A32_UINT, UINT, 128, 32, 32, 32, 32, 4);
  INIT_FORMAT_LINEAR(R32G32B32A32_SINT, SINT, 128, 32, 32, 32, 32, 4);

  INIT_FORMAT_LINEAR(R32G32B32_FLOAT, FLOAT, 96, 32, 32, 32, 0, 3);
  INIT_FORMAT_LINEAR(R32G32B32_UINT, UINT, 96, 32, 32, 32, 0, 3);
  INIT_FORMAT_LINEAR(R32G32B32_SINT, SINT, 96, 32, 32, 32, 0, 3);

  INIT_FORMAT_LINEAR(R32G32_FLOAT, FLOAT, 64, 32, 32, 0, 0, 2);
  INIT_FORMAT_LINEAR(R32G32_UINT, UINT, 64, 32, 32, 0, 0, 2);
  INIT_FORMAT_LINEAR(R32G32_SINT, SINT, 64, 32, 32, 0, 0, 2);

  INIT_FORMAT_LINEAR(R32_FLOAT, FLOAT, 32, 32, 0, 0, 0, 1);
  INIT_FORMAT_LINEAR(R32_UINT, UINT, 32, 32, 0, 0, 0, 1);
  INIT_FORMAT_LINEAR(R32_SINT, SINT, 32, 32, 0, 0, 0, 1);

  INIT_FORMAT_LINEAR(R16G16B16A16_FLOAT, FLOAT, 64, 16, 16, 16, 16, 4);
  INIT_FORMAT_LINEAR(R16G16B16A16_UINT, UINT, 64, 16, 16, 16, 16, 4);
  INIT_FORMAT_LINEAR(R16G16B16A16_SINT, SINT, 64, 16, 16, 16, 16, 4);
  INIT_FORMAT_LINEAR(R16G16B16A16_UNORM, UNORM, 64, 16, 16, 16, 16, 4);
  INIT_FORMAT_LINEAR(R16G16B16A16_SNORM, SNORM, 64, 16, 16, 16, 16, 4);

  INIT_FORMAT_LINEAR(R16G16B16_UNORM, UNORM, 48, 16, 16, 16, 0, 3);

  INIT_FORMAT_LINEAR(R16G16_FLOAT, FLOAT, 32, 16, 16, 0, 0, 2);
  INIT_FORMAT_LINEAR(R16G16_UINT, UINT, 32, 16, 16, 0, 0, 2);
  INIT_FORMAT_LINEAR(R16G16_SINT, SINT, 32, 16, 16, 0, 0, 2);
  INIT_FORMAT_LINEAR(R16G16_UNORM, UNORM, 32, 16, 16, 0, 0, 2);
  INIT_FORMAT_LINEAR(R16G16_SNORM, SNORM, 32, 16, 16, 0, 0, 2);

  INIT_FORMAT_LINEAR(R16_FLOAT, FLOAT, 16, 16, 0, 0, 0, 1);
  INIT_FORMAT_LINEAR(R16_UINT, UINT, 16, 16, 0, 0, 0, 1);
  INIT_FORMAT_LINEAR(R16_SINT, SINT, 16, 16, 0, 0, 0, 1);
  INIT_FORMAT_LINEAR(R16_UNORM, UNORM, 16, 16, 0, 0, 0, 1);
  INIT_FORMAT_LINEAR(R16_SNORM, SNORM, 16, 16, 0, 0, 0, 1);

  INIT_FORMAT_LINEAR(R8G8B8A8_UINT, UINT, 32, 8, 8, 8, 8, 4);
  INIT_FORMAT_LINEAR(R8G8B8A8_SINT, SINT, 32, 8, 8, 8, 8, 4);
  INIT_FORMAT_LINEAR(R8G8B8A8_SNORM, SNORM, 32, 8, 8, 8, 8, 4);

  INIT_FORMAT_LINEAR(R8G8B8A8_UNORM, UNORM, 32, 8, 8, 8, 8, 4);
  INIT_FORMAT_LINEAR(R8G8B8A8_UNORM_SRGB, UNORM, 32, 8, 8, 8, 8, 4);
  SetupSrgbPair(ezImageFormat::R8G8B8A8_UNORM, ezImageFormat::R8G8B8A8_UNORM_SRGB);

  s_formatMetaData[ezImageFormat::R8G8B8A8_UNORM].m_uiChannelMasks[ezImageFormatChannel::R] = 0x000000FF;
  s_formatMetaData[ezImageFormat::R8G8B8A8_UNORM].m_uiChannelMasks[ezImageFormatChannel::G] = 0x0000FF00;
  s_formatMetaData[ezImageFormat::R8G8B8A8_UNORM].m_uiChannelMasks[ezImageFormatChannel::B] = 0x00FF0000;
  s_formatMetaData[ezImageFormat::R8G8B8A8_UNORM].m_uiChannelMasks[ezImageFormatChannel::A] = 0xFF000000;

  INIT_FORMAT_LINEAR(R8G8B8_UNORM, UNORM, 24, 8, 8, 8, 0, 3);
  INIT_FORMAT_LINEAR(R8G8B8_UNORM_SRGB, UNORM, 24, 8, 8, 8, 0, 3);
  SetupSrgbPair(ezImageFormat::R8G8B8_UNORM, ezImageFormat::R8G8B8_UNORM_SRGB);

  INIT_FORMAT_LINEAR(B8G8R8A8_UNORM, UNORM, 32, 8, 8, 8, 8, 4);
  INIT_FORMAT_LINEAR(B8G8R8A8_UNORM_SRGB, UNORM, 32, 8, 8, 8, 8, 4);
  SetupSrgbPair(ezImageFormat::B8G8R8A8_UNORM, ezImageFormat::B8G8R8A8_UNORM_SRGB);

  s_formatMetaData[ezImageFormat::B8G8R8A8_UNORM].m_uiChannelMasks[ezImageFormatChannel::R] = 0x00FF0000;
  s_formatMetaData[ezImageFormat::B8G8R8A8_UNORM].m_uiChannelMasks[ezImageFormatChannel::G] = 0x0000FF00;
  s_formatMetaData[ezImageFormat::B8G8R8A8_UNORM].m_uiChannelMasks[ezImageFormatChannel::B] = 0x000000FF;
  s_formatMetaData[ezImageFormat::B8G8R8A8_UNORM].m_uiChannelMasks[ezImageFormatChannel::A] = 0xFF000000;

  INIT_FORMAT_LINEAR(B8G8R8X8_UNORM, UNORM, 32, 8, 8, 8, 0, 3);
  INIT_FORMAT_LINEAR(B8G8R8X8_UNORM_SRGB, UNORM, 32, 8, 8, 8, 0, 3);
  SetupSrgbPair(ezImageFormat::B8G8R8X8_UNORM, ezImageFormat::B8G8R8X8_UNORM_SRGB);

  s_formatMetaData[ezImageFormat::B8G8R8X8_UNORM].m_uiChannelMasks[ezImageFormatChannel::R] = 0x00FF0000;
  s_formatMetaData[ezImageFormat::B8G8R8X8_UNORM].m_uiChannelMasks[ezImageFormatChannel::G] = 0x0000FF00;
  s_formatMetaData[ezImageFormat::B8G8R8X8_UNORM].m_uiChannelMasks[ezImageFormatChannel::B] = 0x000000FF;
  s_formatMetaData[ezImageFormat::B8G8R8X8_UNORM].m_uiChannelMasks[ezImageFormatChannel::A] = 0x00000000;

  INIT_FORMAT_LINEAR(B8G8R8_UNORM, UNORM, 24, 8, 8, 8, 0, 3);
  INIT_FORMAT_LINEAR(B8G8R8_UNORM_SRGB, UNORM, 24, 8, 8, 8, 0, 3);
  SetupSrgbPair(ezImageFormat::B8G8R8_UNORM, ezImageFormat::B8G8R8_UNORM_SRGB);

  s_formatMetaData[ezImageFormat::B8G8R8_UNORM].m_uiChannelMasks[ezImageFormatChannel::R] = 0x00FF0000;
  s_formatMetaData[ezImageFormat::B8G8R8_UNORM].m_uiChannelMasks[ezImageFormatChannel::G] = 0x0000FF00;
  s_formatMetaData[ezImageFormat::B8G8R8_UNORM].m_uiChannelMasks[ezImageFormatChannel::B] = 0x000000FF;
  s_formatMetaData[ezImageFormat::B8G8R8_UNORM].m_uiChannelMasks[ezImageFormatChannel::A] = 0x00000000;

  INIT_FORMAT_LINEAR(R8G8_UINT, UINT, 16, 8, 8, 0, 0, 2);
  INIT_FORMAT_LINEAR(R8G8_SINT, SINT, 16, 8, 8, 0, 0, 2);
  INIT_FORMAT_LINEAR(R8G8_UNORM, UNORM, 16, 8, 8, 0, 0, 2);
  INIT_FORMAT_LINEAR(R8G8_SNORM, SNORM, 16, 8, 8, 0, 0, 2);

  INIT_FORMAT_LINEAR(R8_UINT, UINT, 8, 8, 0, 0, 0, 1);
  INIT_FORMAT_LINEAR(R8_SINT, SINT, 8, 8, 0, 0, 0, 1);
  INIT_FORMAT_LINEAR(R8_SNORM, SNORM, 8, 8, 0, 0, 0, 1);

  INIT_FORMAT_LINEAR(R8_UNORM, UNORM, 8, 8, 0, 0, 0, 1);
  s_formatMetaData[ezImageFormat::R8_UNORM].m_uiChannelMasks[ezImageFormatChannel::R] = 0xFF;
  s_formatMetaData[ezImageFormat::R8_UNORM].m_uiChannelMasks[ezImageFormatChannel::G] = 0x00;
  s_formatMetaData[ezImageFormat::R8_UNORM].m_uiChannelMasks[ezImageFormatChannel::B] = 0x00;
  s_formatMetaData[ezImageFormat::R8_UNORM].m_uiChannelMasks[ezImageFormatChannel::A] = 0x00;

  INIT_FORMAT_COMPRESSED(BC1_UNORM, UNORM, 64, 4, 4, 1, true, 4);
  INIT_FORMAT_COMPRESSED(BC1_UNORM_SRGB, UNORM, 64, 4, 4, 1, true, 4);
  SetupSrgbPair(ezImageFormat::BC1_UNORM, ezImageFormat::BC1_UNORM_SRGB);

  INIT_FORMAT_COMPRESSED(BC2_UNORM, UNORM, 128, 4, 4, 1, true, 4);
  INIT_FORMAT_COMPRESSED(BC2_UNORM_SRGB, UNORM, 128, 4, 4, 1, true, 4);
  SetupSrgbPair(ezImageFormat::BC2_UNORM, ezImageFormat::BC2_UNORM_SRGB);

  INIT_FORMAT_COMPRESSED(BC3_UNORM, UNORM, 128, 4, 4, 1, true, 4);
  INIT_FORMAT_COMPRESSED(BC3_UNORM_SRGB, UNORM, 128, 4, 4, 1, true, 4);
  SetupSrgbPair(ezImageFormat::BC3_UNORM, ezImageFormat::BC3_UNORM_SRGB);

  INIT_FORMAT_COMPRESSED(BC4_UNORM, UNORM, 64, 4, 4, 1, true, 1);
  INIT_FORMAT_COMPRESSED(BC4_SNORM, SNORM, 64, 4, 4, 1, true, 1);

  INIT_FORMAT_COMPRESSED(BC5_UNORM, UNORM, 128, 4, 4, 1, true, 2);
  INIT_FORMAT_COMPRESSED(BC5_SNORM, SNORM, 128, 4, 4, 1, true, 2);

  INIT_FORMAT_COMPRESSED(BC6H_UF16, FLOAT, 128, 4, 4, 1, true, 3);
  INIT_FORMAT_COMPRESSED(BC6H_SF16, FLOAT, 128, 4, 4, 1, true, 3);

  INIT_FORMAT_COMPRESSED(BC7_UNORM, UNORM, 128, 4, 4, 1, true, 4);
  INIT_FORMAT_COMPRESSED(BC7_UNORM_SRGB, UNORM, 128, 4, 4, 1, true, 4);
  SetupSrgbPair(ezImageFormat::BC7_UNORM, ezImageFormat::BC7_UNORM_SRGB);



  INIT_FORMAT_LINEAR(B5G5R5A1_UNORM, UNORM, 16, 5, 5, 5, 1, 4);
  INIT_FORMAT_LINEAR(B5G5R5A1_UNORM_SRGB, UNORM, 16, 5, 5, 5, 1, 4);
  SetupSrgbPair(ezImageFormat::B5G5R5A1_UNORM, ezImageFormat::B5G5R5A1_UNORM_SRGB);

  INIT_FORMAT_LINEAR(B4G4R4A4_UNORM, UNORM, 16, 4, 4, 4, 4, 4);
  s_formatMetaData[ezImageFormat::B4G4R4A4_UNORM].m_uiChannelMasks[ezImageFormatChannel::R] = 0x0F00;
  s_formatMetaData[ezImageFormat::B4G4R4A4_UNORM].m_uiChannelMasks[ezImageFormatChannel::G] = 0x00F0;
  s_formatMetaData[ezImageFormat::B4G4R4A4_UNORM].m_uiChannelMasks[ezImageFormatChannel::B] = 0x000F;
  s_formatMetaData[ezImageFormat::B4G4R4A4_UNORM].m_uiChannelMasks[ezImageFormatChannel::A] = 0xF000;
  INIT_FORMAT_LINEAR(B4G4R4A4_UNORM_SRGB, UNORM, 16, 4, 4, 4, 4, 4);
  SetupSrgbPair(ezImageFormat::B4G4R4A4_UNORM, ezImageFormat::B4G4R4A4_UNORM_SRGB);

  INIT_FORMAT_LINEAR(A4B4G4R4_UNORM, UNORM, 16, 4, 4, 4, 4, 4);
  s_formatMetaData[ezImageFormat::A4B4G4R4_UNORM].m_uiChannelMasks[ezImageFormatChannel::R] = 0xF000;
  s_formatMetaData[ezImageFormat::A4B4G4R4_UNORM].m_uiChannelMasks[ezImageFormatChannel::G] = 0x0F00;
  s_formatMetaData[ezImageFormat::A4B4G4R4_UNORM].m_uiChannelMasks[ezImageFormatChannel::B] = 0x00F0;
  s_formatMetaData[ezImageFormat::A4B4G4R4_UNORM].m_uiChannelMasks[ezImageFormatChannel::A] = 0x000F;
  INIT_FORMAT_LINEAR(A4B4G4R4_UNORM_SRGB, UNORM, 16, 4, 4, 4, 4, 4);
  SetupSrgbPair(ezImageFormat::A4B4G4R4_UNORM, ezImageFormat::A4B4G4R4_UNORM_SRGB);

  INIT_FORMAT_LINEAR(B5G6R5_UNORM, UNORM, 16, 5, 6, 5, 0, 3);
  s_formatMetaData[ezImageFormat::B5G6R5_UNORM].m_uiChannelMasks[ezImageFormatChannel::R] = 0xF800;
  s_formatMetaData[ezImageFormat::B5G6R5_UNORM].m_uiChannelMasks[ezImageFormatChannel::G] = 0x07E0;
  s_formatMetaData[ezImageFormat::B5G6R5_UNORM].m_uiChannelMasks[ezImageFormatChannel::B] = 0x001F;
  s_formatMetaData[ezImageFormat::B5G6R5_UNORM].m_uiChannelMasks[ezImageFormatChannel::A] = 0x0000;
  INIT_FORMAT_LINEAR(B5G6R5_UNORM_SRGB, UNORM, 16, 5, 6, 5, 0, 3);
  SetupSrgbPair(ezImageFormat::B5G6R5_UNORM, ezImageFormat::B5G6R5_UNORM_SRGB);

  INIT_FORMAT_LINEAR(B5G5R5A1_UNORM, UNORM, 16, 5, 5, 5, 1, 3);
  s_formatMetaData[ezImageFormat::B5G5R5A1_UNORM].m_uiChannelMasks[ezImageFormatChannel::R] = 0x7C00;
  s_formatMetaData[ezImageFormat::B5G5R5A1_UNORM].m_uiChannelMasks[ezImageFormatChannel::G] = 0x03E0;
  s_formatMetaData[ezImageFormat::B5G5R5A1_UNORM].m_uiChannelMasks[ezImageFormatChannel::B] = 0x001F;
  s_formatMetaData[ezImageFormat::B5G5R5A1_UNORM].m_uiChannelMasks[ezImageFormatChannel::A] = 0x8000;
  INIT_FORMAT_LINEAR(B5G5R5A1_UNORM_SRGB, UNORM, 16, 5, 5, 5, 1, 3);
  SetupSrgbPair(ezImageFormat::B5G5R5A1_UNORM, ezImageFormat::B5G5R5A1_UNORM_SRGB);

  INIT_FORMAT_LINEAR(B5G5R5X1_UNORM, UNORM, 16, 5, 5, 5, 0, 3);
  s_formatMetaData[ezImageFormat::B5G5R5X1_UNORM].m_uiChannelMasks[ezImageFormatChannel::R] = 0x7C00;
  s_formatMetaData[ezImageFormat::B5G5R5X1_UNORM].m_uiChannelMasks[ezImageFormatChannel::G] = 0x03E0;
  s_formatMetaData[ezImageFormat::B5G5R5X1_UNORM].m_uiChannelMasks[ezImageFormatChannel::B] = 0x001F;
  s_formatMetaData[ezImageFormat::B5G5R5X1_UNORM].m_uiChannelMasks[ezImageFormatChannel::A] = 0x0000;
  INIT_FORMAT_LINEAR(B5G5R5X1_UNORM_SRGB, UNORM, 16, 5, 5, 5, 0, 3);
  SetupSrgbPair(ezImageFormat::B5G5R5X1_UNORM, ezImageFormat::B5G5R5X1_UNORM_SRGB);

  INIT_FORMAT_LINEAR(A1B5G5R5_UNORM, UNORM, 16, 5, 5, 5, 1, 3);
  s_formatMetaData[ezImageFormat::A1B5G5R5_UNORM].m_uiChannelMasks[ezImageFormatChannel::R] = 0x7C00;
  s_formatMetaData[ezImageFormat::A1B5G5R5_UNORM].m_uiChannelMasks[ezImageFormatChannel::G] = 0x03E0;
  s_formatMetaData[ezImageFormat::A1B5G5R5_UNORM].m_uiChannelMasks[ezImageFormatChannel::B] = 0x001F;
  s_formatMetaData[ezImageFormat::A1B5G5R5_UNORM].m_uiChannelMasks[ezImageFormatChannel::A] = 0x8000;
  INIT_FORMAT_LINEAR(A1B5G5R5_UNORM_SRGB, UNORM, 16, 5, 5, 5, 1, 3);
  SetupSrgbPair(ezImageFormat::A1B5G5R5_UNORM, ezImageFormat::A1B5G5R5_UNORM_SRGB);

  INIT_FORMAT_LINEAR(X1B5G5R5_UNORM, UNORM, 16, 5, 5, 5, 1, 3);
  s_formatMetaData[ezImageFormat::X1B5G5R5_UNORM].m_uiChannelMasks[ezImageFormatChannel::R] = 0x7C00;
  s_formatMetaData[ezImageFormat::X1B5G5R5_UNORM].m_uiChannelMasks[ezImageFormatChannel::G] = 0x03E0;
  s_formatMetaData[ezImageFormat::X1B5G5R5_UNORM].m_uiChannelMasks[ezImageFormatChannel::B] = 0x001F;
  s_formatMetaData[ezImageFormat::X1B5G5R5_UNORM].m_uiChannelMasks[ezImageFormatChannel::A] = 0x0000;
  INIT_FORMAT_LINEAR(X1B5G5R5_UNORM_SRGB, UNORM, 16, 5, 5, 5, 1, 3);
  SetupSrgbPair(ezImageFormat::X1B5G5R5_UNORM, ezImageFormat::X1B5G5R5_UNORM_SRGB);

  INIT_FORMAT_LINEAR(R11G11B10_FLOAT, FLOAT, 32, 11, 11, 10, 0, 3);
  INIT_FORMAT_LINEAR(R10G10B10A2_UINT, UINT, 32, 10, 10, 10, 2, 4);
  INIT_FORMAT_LINEAR(R10G10B10A2_UNORM, UNORM, 32, 10, 10, 10, 2, 4);

  // msdn.microsoft.com/en-us/library/windows/desktop/bb943991(v=vs.85).aspx documents R10G10B10A2 as having an alpha mask of 0
  s_formatMetaData[ezImageFormat::R10G10B10A2_UNORM].m_uiChannelMasks[ezImageFormatChannel::R] = 0x000003FF;
  s_formatMetaData[ezImageFormat::R10G10B10A2_UNORM].m_uiChannelMasks[ezImageFormatChannel::G] = 0x000FFC00;
  s_formatMetaData[ezImageFormat::R10G10B10A2_UNORM].m_uiChannelMasks[ezImageFormatChannel::B] = 0x3FF00000;
  s_formatMetaData[ezImageFormat::R10G10B10A2_UNORM].m_uiChannelMasks[ezImageFormatChannel::A] = 0;

  INIT_FORMAT_DEPTH(D32_FLOAT, DEPTH_STENCIL, 32, false, 32, 0);
  INIT_FORMAT_DEPTH(D32_FLOAT_S8X24_UINT, DEPTH_STENCIL, 64, true, 32, 8);
  INIT_FORMAT_DEPTH(D24_UNORM_S8_UINT, DEPTH_STENCIL, 32, true, 24, 8);
  INIT_FORMAT_DEPTH(D16_UNORM, DEPTH_STENCIL, 16, false, 16, 0);

  INIT_FORMAT_COMPRESSED(ASTC_4x4_UNORM, UNORM, 128, 4, 4, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_5x4_UNORM, UNORM, 128, 5, 4, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_5x5_UNORM, UNORM, 128, 5, 5, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_6x5_UNORM, UNORM, 128, 6, 5, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_6x6_UNORM, UNORM, 128, 6, 6, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_8x5_UNORM, UNORM, 128, 8, 5, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_8x6_UNORM, UNORM, 128, 8, 6, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_10x5_UNORM, UNORM, 128, 10, 5, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_10x6_UNORM, UNORM, 128, 10, 6, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_8x8_UNORM, UNORM, 128, 8, 8, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_10x8_UNORM, UNORM, 128, 10, 8, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_10x10_UNORM, UNORM, 128, 10, 10, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_12x10_UNORM, UNORM, 128, 12, 10, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_12x12_UNORM, UNORM, 128, 12, 12, 1, false, 4);

  INIT_FORMAT_COMPRESSED(ASTC_4x4_UNORM_SRGB, UNORM, 128, 4, 4, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_5x4_UNORM_SRGB, UNORM, 128, 5, 4, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_5x5_UNORM_SRGB, UNORM, 128, 5, 5, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_6x5_UNORM_SRGB, UNORM, 128, 6, 5, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_6x6_UNORM_SRGB, UNORM, 128, 6, 6, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_8x5_UNORM_SRGB, UNORM, 128, 8, 5, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_8x6_UNORM_SRGB, UNORM, 128, 8, 6, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_10x5_UNORM_SRGB, UNORM, 128, 10, 5, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_10x6_UNORM_SRGB, UNORM, 128, 10, 6, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_8x8_UNORM_SRGB, UNORM, 128, 8, 8, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_10x8_UNORM_SRGB, UNORM, 128, 10, 8, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_10x10_UNORM_SRGB, UNORM, 128, 10, 10, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_12x10_UNORM_SRGB, UNORM, 128, 12, 10, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_12x12_UNORM_SRGB, UNORM, 128, 12, 12, 1, false, 4);

  SetupSrgbPair(ezImageFormat::ASTC_4x4_UNORM, ezImageFormat::ASTC_4x4_UNORM_SRGB);
  SetupSrgbPair(ezImageFormat::ASTC_5x4_UNORM, ezImageFormat::ASTC_5x4_UNORM_SRGB);
  SetupSrgbPair(ezImageFormat::ASTC_5x5_UNORM, ezImageFormat::ASTC_5x5_UNORM_SRGB);
  SetupSrgbPair(ezImageFormat::ASTC_6x5_UNORM, ezImageFormat::ASTC_6x5_UNORM_SRGB);
  SetupSrgbPair(ezImageFormat::ASTC_6x6_UNORM, ezImageFormat::ASTC_6x6_UNORM_SRGB);
  SetupSrgbPair(ezImageFormat::ASTC_8x5_UNORM, ezImageFormat::ASTC_8x5_UNORM_SRGB);
  SetupSrgbPair(ezImageFormat::ASTC_8x6_UNORM, ezImageFormat::ASTC_8x6_UNORM_SRGB);
  SetupSrgbPair(ezImageFormat::ASTC_10x5_UNORM, ezImageFormat::ASTC_10x5_UNORM_SRGB);
  SetupSrgbPair(ezImageFormat::ASTC_10x6_UNORM, ezImageFormat::ASTC_10x6_UNORM_SRGB);
  SetupSrgbPair(ezImageFormat::ASTC_8x8_UNORM, ezImageFormat::ASTC_8x8_UNORM_SRGB);
  SetupSrgbPair(ezImageFormat::ASTC_10x8_UNORM, ezImageFormat::ASTC_10x8_UNORM_SRGB);
  SetupSrgbPair(ezImageFormat::ASTC_10x10_UNORM, ezImageFormat::ASTC_10x10_UNORM_SRGB);
  SetupSrgbPair(ezImageFormat::ASTC_12x10_UNORM, ezImageFormat::ASTC_12x10_UNORM_SRGB);
  SetupSrgbPair(ezImageFormat::ASTC_12x12_UNORM, ezImageFormat::ASTC_12x12_UNORM_SRGB);
}

static const EZ_ALWAYS_INLINE ezImageFormatMetaData& GetImageFormatMetaData(ezImageFormat::Enum format)
{
  if (s_formatMetaData.IsEmpty())
  {
    SetupImageFormatTable();
  }

  return s_formatMetaData[format];
}

ezUInt32 ezImageFormat::GetBitsPerPixel(Enum format)
{
  const ezImageFormatMetaData& metaData = GetImageFormatMetaData(format);
  if (metaData.m_formatType == ezImageFormatType::BLOCK_COMPRESSED)
  {
    auto pixelsPerBlock = metaData.m_uiBlockWidth * metaData.m_uiBlockHeight * metaData.m_uiBlockDepth;
    return (metaData.m_uiBitsPerBlock + pixelsPerBlock - 1) / pixelsPerBlock; // Return rounded-up value
  }
  else
  {
    return metaData.m_uiBitsPerBlock;
  }
}


float ezImageFormat::GetExactBitsPerPixel(Enum format)
{
  const ezImageFormatMetaData& metaData = GetImageFormatMetaData(format);
  if (metaData.m_formatType == ezImageFormatType::BLOCK_COMPRESSED)
  {
    auto pixelsPerBlock = metaData.m_uiBlockWidth * metaData.m_uiBlockHeight * metaData.m_uiBlockDepth;
    return static_cast<float>(metaData.m_uiBitsPerBlock) / pixelsPerBlock;
  }
  else
  {
    return metaData.m_uiBitsPerBlock;
  }
}


ezUInt32 ezImageFormat::GetBitsPerBlock(Enum format)
{
  return GetImageFormatMetaData(format).m_uiBitsPerBlock;
}


ezUInt32 ezImageFormat::GetNumChannels(Enum format)
{
  return GetImageFormatMetaData(format).m_uiNumChannels;
}

ezImageFormat::Enum ezImageFormat::FromPixelMask(ezUInt32 uiRedMask, ezUInt32 uiGreenMask, ezUInt32 uiBlueMask, ezUInt32 uiAlphaMask,
                                                 ezUInt32 uiBitsPerPixel)
{
  // Some DDS files in the wild are encoded as this
  if (uiBitsPerPixel == 8 && uiRedMask == 0xff && uiGreenMask == 0xff && uiBlueMask == 0xff)
  {
    return R8_UNORM;
  }

  for (ezUInt32 index = 0; index < NUM_FORMATS; index++)
  {
    Enum format = static_cast<Enum>(index);
    if (GetChannelMask(format, ezImageFormatChannel::R) == uiRedMask && GetChannelMask(format, ezImageFormatChannel::G) == uiGreenMask &&
        GetChannelMask(format, ezImageFormatChannel::B) == uiBlueMask && GetChannelMask(format, ezImageFormatChannel::A) == uiAlphaMask &&
        GetBitsPerPixel(format) == uiBitsPerPixel && GetDataType(format) == ezImageFormatDataType::UNORM && !IsCompressed(format))
    {
      return format;
    }
  }

  return UNKNOWN;
}

const char* ezImageFormat::GetName(Enum format)
{
  return GetImageFormatMetaData(format).m_szName;
}


ezUInt32 ezImageFormat::GetChannelMask(Enum format, ezImageFormatChannel::Enum c)
{
  return GetImageFormatMetaData(format).m_uiChannelMasks[c];
}

ezUInt32 ezImageFormat::GetBitsPerChannel(Enum format, ezImageFormatChannel::Enum c)
{
  return GetImageFormatMetaData(format).m_uiBitsPerChannel[c];
}

ezUInt32 ezImageFormat::GetRedMask(Enum format)
{
  return GetImageFormatMetaData(format).m_uiChannelMasks[ezImageFormatChannel::R];
}

ezUInt32 ezImageFormat::GetGreenMask(Enum format)
{
  return GetImageFormatMetaData(format).m_uiChannelMasks[ezImageFormatChannel::G];
}

ezUInt32 ezImageFormat::GetBlueMask(Enum format)
{
  return GetImageFormatMetaData(format).m_uiChannelMasks[ezImageFormatChannel::B];
}

ezUInt32 ezImageFormat::GetAlphaMask(Enum format)
{
  return GetImageFormatMetaData(format).m_uiChannelMasks[ezImageFormatChannel::A];
}

ezUInt32 ezImageFormat::GetBlockWidth(Enum format)
{
  return GetImageFormatMetaData(format).m_uiBlockWidth;
}

ezUInt32 ezImageFormat::GetBlockHeight(Enum format)
{
  return GetImageFormatMetaData(format).m_uiBlockHeight;
}

ezUInt32 ezImageFormat::GetBlockDepth(Enum format)
{
  return GetImageFormatMetaData(format).m_uiBlockDepth;
}

ezImageFormatDataType::Enum ezImageFormat::GetDataType(Enum format)
{
  return GetImageFormatMetaData(format).m_dataType;
}

bool ezImageFormat::IsCompressed(Enum format)
{
  return GetImageFormatMetaData(format).m_formatType == ezImageFormatType::BLOCK_COMPRESSED;
}

ezImageFormat::Enum ezImageFormat::AsSrgb(Enum format)
{
  return GetImageFormatMetaData(format).m_asSrgb;
}

ezImageFormat::Enum ezImageFormat::AsLinear(Enum format)
{
  return GetImageFormatMetaData(format).m_asLinear;
}

ezUInt32 ezImageFormat::GetNumBlocksX(Enum format, ezUInt32 width)
{
  return (width - 1) / GetBlockWidth(format) + 1;
}

ezUInt32 ezImageFormat::GetNumBlocksY(Enum format, ezUInt32 height)
{
  return (height - 1) / GetBlockHeight(format) + 1;
}

ezUInt32 ezImageFormat::GetNumBlocksZ(Enum format, ezUInt32 depth)
{
  return (depth - 1) / GetBlockDepth(format) + 1;
}

ezUInt32 ezImageFormat::GetRowPitch(Enum format, ezUInt32 width)
{
  return GetNumBlocksX(format, width) * GetBitsPerBlock(format) / 8;
}

ezUInt32 ezImageFormat::GetDepthPitch(Enum format, ezUInt32 width, ezUInt32 height)
{
  return GetNumBlocksY(format, height) * GetRowPitch(format, width);
}

ezImageFormatType::Enum ezImageFormat::GetType(Enum format)
{
  return GetImageFormatMetaData(format).m_formatType;
}

EZ_STATICLINK_FILE(Foundation, Foundation_Image_Implementation_ImageFormat);
