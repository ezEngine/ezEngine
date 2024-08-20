#include <GameEngine/GameEnginePCH.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <GameEngine/Utils/ImageDataResource.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/Formats/ImageFileFormat.h>
#include <Texture/Image/Formats/StbImageFileFormats.h>
#include <Texture/ezTexFormat/ezTexFormat.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezImageDataResource, 1, ezRTTIDefaultAllocator<ezImageDataResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezImageDataResource);
// clang-format on

ezImageDataResource::ezImageDataResource()
  : ezResource(DoUpdate::OnAnyThread, 1)
{
}

ezImageDataResource::~ezImageDataResource() = default;

ezResourceLoadDesc ezImageDataResource::UnloadData(Unload WhatToUnload)
{
  m_pDescriptor.Clear();

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;

  return res;
}

ezResourceLoadDesc ezImageDataResource::UpdateContent(ezStreamReader* Stream)
{
  EZ_LOG_BLOCK("ezImageDataResource::UpdateContent", GetResourceIdOrDescription());

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  ezStringBuilder sAbsFilePath;
  (*Stream) >> sAbsFilePath;

  ezImageDataResourceDescriptor desc;

  if (sAbsFilePath.HasExtension("ezBinImageData"))
  {
    ezAssetFileHeader AssetHash;
    if (AssetHash.Read(*Stream).Failed())
    {
      res.m_State = ezResourceState::LoadedResourceMissing;
      return res;
    }

    ezUInt8 uiVersion = 0;
    ezUInt8 uiDataFormat = 0;

    *Stream >> uiVersion;
    *Stream >> uiDataFormat;

    if (uiVersion != 1 || uiDataFormat != 1)
    {
      ezLog::Error("Unsupported ezImageData file format or version");

      res.m_State = ezResourceState::LoadedResourceMissing;
      return res;
    }

    ezStbImageFileFormats fmt;
    if (fmt.ReadImage(*Stream, desc.m_Image, "png").Failed())
    {
      res.m_State = ezResourceState::LoadedResourceMissing;
      return res;
    }
  }
  else
  {
    ezStringBuilder ext;
    ext = sAbsFilePath.GetFileExtension();

    if (ezImageFileFormat::GetReaderFormat(ext)->ReadImage(*Stream, desc.m_Image, ext).Failed())
    {
      res.m_State = ezResourceState::LoadedResourceMissing;
      return res;
    }
  }


  CreateResource(std::move(desc));

  res.m_State = ezResourceState::Loaded;
  return res;
}

void ezImageDataResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezImageDataResource);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;

  if (m_pDescriptor)
  {
    out_NewMemoryUsage.m_uiMemoryCPU += m_pDescriptor->m_Image.GetByteBlobPtr().GetCount();
  }
}

EZ_RESOURCE_IMPLEMENT_CREATEABLE(ezImageDataResource, ezImageDataResourceDescriptor)
{
  m_pDescriptor = EZ_DEFAULT_NEW(ezImageDataResourceDescriptor);

  *m_pDescriptor = std::move(descriptor);

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  if (m_pDescriptor->m_Image.Convert(ezImageFormat::R32G32B32A32_FLOAT).Failed())
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
  }

  return res;
}

// ezResult ezImageDataResourceDescriptor::Serialize(ezStreamWriter& stream) const
//{
//  EZ_SUCCEED_OR_RETURN(ezImageFileFormat::GetWriterFormat("png")->WriteImage(stream, m_Image, "png"));
//
//  return EZ_SUCCESS;
//}
//
// ezResult ezImageDataResourceDescriptor::Deserialize(ezStreamReader& stream)
//{
//  EZ_SUCCEED_OR_RETURN(ezImageFileFormat::GetReaderFormat("png")->ReadImage(stream, m_Image, "png"));
//
//  return EZ_SUCCESS;
//}


EZ_STATICLINK_FILE(GameEngine, GameEngine_Utils_Implementation_ImageDataResource);
