#pragma once

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Strings/PathUtils.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Logging/HTMLWriter.h>
#include <Core/Application/Application.h>
#include <TexConv/DirectXTex/DirectXTex.h>
#include <Foundation/Image/Image.h>
#include <Foundation/Image/ImageConversion.h>
#include <Foundation/Image/Formats/ImageFormatMappings.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/Image/Formats/DdsFileFormat.h>
#include <Foundation/Math/Rect.h>
#include <memory>

using namespace DirectX;
using namespace std;

struct ID3D11Device;
class ezTextureGroupDesc;

enum TexConvReturnCodes
{
  OK,
  UNKNOWN_FAILURE,
  VALIDATION_FAILED,
  BAD_SINGLE_CUBEMAP_FILE,
  FAILED_CONVERT_INPUT_TO_RGBA,
  FAILED_WRITE_OUTPUT,
  FAILED_LOAD_INPUTS,
  BAD_INPUT_RESOLUTIONS,
  FAILED_PASS_THROUGH,
  FAILED_MIPMAP_GENERATION,
  FAILED_BC_COMPRESSION,
  FAILED_CONVERT_TO_OUTPUT_FORMAT,
  FAILED_SAVE_AS_DDS,
  FAILED_INITIALIZE_CUBEMAP,
  FAILED_COMBINE_CUBEMAP,
  FAILED_PREMULTIPLY_ALPHA,
};

class ezTexConv : public ezApplication
{
public:

  enum Channel
  {
    None = 0,
    Red = EZ_BIT(0),
    Green = EZ_BIT(1),
    Blue = EZ_BIT(2),
    Alpha = EZ_BIT(3),
    All = 0xFF
  };

  enum class TextureType
  {
    Texture2D,
    Cubemap,
    DecalAtlas,
    //Volume
  };

  struct ChannelMapping
  {
    ezInt8 m_iInput = 0;
    ezUInt8 m_uiChannelMask = 0;
  };

  enum DecalLayer
  {
    BaseColor,
    Normal,
    ENUM_COUNT,
  };

  struct DecalDesc
  {
    ezString m_sIdentifier;
    ezString m_sFile[DecalLayer::ENUM_COUNT];
    ezImage m_Image[DecalLayer::ENUM_COUNT];
    ezRectU32 m_Rect[DecalLayer::ENUM_COUNT];
  };

  TextureType m_TextureType = TextureType::Texture2D;
  ezHybridArray<ezString, 4> m_InputFileNames;
  ezDynamicArray<ezImage> m_InputImages;
  ezString m_sOutputFile;
  bool m_bGeneratedMipmaps = false;
  bool m_bCompress = false;
  bool m_bSRGBOutput = false;
  bool m_bHDROutput = false;
  bool m_bPremultiplyAlpha = false;
  ezUInt8 m_uiFilterSetting;
  ezUInt8 m_uiAddressU = 0;
  ezUInt8 m_uiAddressV = 0;
  ezUInt8 m_uiAddressW = 0;
  ezUInt8 m_uiOutputChannels = 4;
  ezHybridArray<ezImage*, 6> m_CleanupImages;
  ezUInt64 m_uiAssetHash = 0;
  ezUInt16 m_uiAssetVersion = 0;

  ezString m_sThumbnailFile;
  bool m_bHasOutput = false; ///< Whether m_FileOut is set, can be false if only a thumbnail need to be generated.
  ezDeferredFileWriter m_FileOut;
  bool m_bAlphaIsMaskOnly = false;
  shared_ptr<ScratchImage> m_pCurrentImage;
  ID3D11Device* m_pD3dDevice = nullptr;

  ChannelMapping m_2dSource[4];

  ezTexConv();

  virtual void BeforeCoreStartup() override;
  virtual void AfterCoreStartup() override;
  virtual void BeforeCoreShutdown() override;
  virtual const char* TranslateReturnCode() const override;

  ChannelMapping ParseInputCfg(const char* cfg, ezInt8 iChannelIndex, bool bSingleChannel);
  void ParseCommandLine();
  ezResult ValidateConfiguration();
  void PrintConfig();

  ezString ChannelMaskToString(ezUInt8 mask);

  ezResult LoadSingleInputFile(const char* szFile);
  ezResult LoadInputs();
  void CheckCompression();
  ezResult ConvertInputsToRGBA();

  float GetChannelValue(const ChannelMapping& ds, const ezColor& rgba);
  bool IsImageAlphaBinaryMask(const ezImage& img);

  ezImage* CreateCombined2DImage(const ChannelMapping* dataSources);
  ezImageFormat::Enum ChooseOutputFormat(bool bSRGB, bool bAlphaIsMask) const;
  bool CanPassThroughInput() const;
  void WriteTexHeader(ezStreamWriter& stream);

  virtual ezApplication::ApplicationExecution Run() override;
  ezApplication::ApplicationExecution RunInternal();

  ezResult CreateTexture2D(ezImage* pImage, bool bCheckAlphaIsMask);
  ezResult CreateTextureCube();
  ezResult CreateTextureCubeFromSingleFile();
  ezResult CreateTextureCubeFrom6Files();

  ezResult PassImageThrough();
  ezResult GenerateMipmaps();
  ezResult ApplyPremultiplyAlpha();
  ezResult ConvertToOutputFormat();
  ezResult SaveResultToDDS(ezStreamWriter& stream);
  ezResult SaveThumbnail();

  ezResult CreateDecalAtlas();

  ezResult LoadDecalInputs(ezTextureGroupDesc &decalAtlasDesc, ezDynamicArray<DecalDesc> &decals);
  ezResult SortDecalsIntoAtlas(ezDynamicArray<DecalDesc> &decals, ezUInt32& out_ResX, ezUInt32& out_ResY, ezInt32 layer);
  ezResult CreateDecalAtlasTexture(const ezDynamicArray<DecalDesc>& decals, ezUInt32 uiResX, ezUInt32 uiResY, ezImage& atlas, ezInt32 layer);
  ezResult TrySortDecalsIntoAtlas(ezDynamicArray<DecalDesc> &decals, ezUInt32 uiWidth, ezUInt32 uiHeight, ezInt32 layer);
  ezResult ToFloatImage(const ezImage& src, ezImage& dst);
  ezResult CreateDecalLayerTexture(ezDynamicArray<DecalDesc>& decals, ezInt32 layer, ezStreamWriter& stream);
  void WriteDecalAtlasInfo(ezDynamicArray<DecalDesc> decals);

};
