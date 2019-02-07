#include <TexturePCH.h>

#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/TexConv/TexConvProcessor.h>
#include <Texture/Utils/TextureGroupDesc.h>

ezResult ezTexConvProcessor::GenerateDecalAtlas(ezMemoryStreamWriter& stream)
{
  if (m_Descriptor.m_OutputType != ezTexConvOutputType::DecalAtlas)
    return EZ_SUCCESS;


  if (m_Descriptor.m_sDecalAtlasDescFile.IsEmpty())
  {
    ezLog::Error("Decal atlas description file is not specified.");
    return EZ_FAILURE;
  }

  ezTextureGroupDesc atlasDesc;
  ezDynamicArray<TextureAtlasItem> atlasItems;

  if (atlasDesc.Load(m_Descriptor.m_sDecalAtlasDescFile).Failed())
  {
    ezLog::Error("Failed to load decal atlas description '{0}'", m_Descriptor.m_sDecalAtlasDescFile);
    return EZ_FAILURE;
  }

  atlasItems.Reserve(atlasDesc.m_Groups.GetCount());

  EZ_SUCCEED_OR_RETURN(LoadAtlasInputs(atlasDesc, atlasItems));

  const ezUInt8 uiVersion = 1;
  stream << uiVersion;

  ezDdsFileFormat ddsWriter;
  ezImage atlasImg;

  bool bDebugOutput = true;

  EZ_SUCCEED_OR_RETURN(CreateAtlasLayerTexture(atlasItems, TextureAtlasLayer::BaseColor, atlasImg, 4));

  if (ddsWriter.WriteImage(stream, atlasImg, ezLog::GetThreadLocalLogSystem(), "dds").Failed())
  {
    ezLog::Error("Failed to write DDS image to decal atlas file.");
    return EZ_FAILURE;
  }

  if (bDebugOutput)
  {
    atlasImg.SaveTo("D:/DecalAtlas_diff.dds");
  }

  EZ_SUCCEED_OR_RETURN(CreateAtlasLayerTexture(atlasItems, TextureAtlasLayer::Normal, atlasImg, 4));

  if (ddsWriter.WriteImage(stream, atlasImg, ezLog::GetThreadLocalLogSystem(), "dds").Failed())
  {
    ezLog::Error("Failed to write DDS image to decal atlas file.");
    return EZ_FAILURE;
  }

  if (bDebugOutput)
  {
    atlasImg.SaveTo("D:/DecalAtlas_norm.dds");
  }

  EZ_SUCCEED_OR_RETURN(WriteTextureAtlasInfo(atlasItems, stream));

  return EZ_SUCCESS;
}
