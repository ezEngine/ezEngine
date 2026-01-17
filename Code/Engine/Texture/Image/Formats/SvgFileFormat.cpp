#include <Texture/TexturePCH.h>

#ifdef BUILDSYSTEM_ENABLE_LUNASVG_SUPPORT

#  include <Texture/Image/Formats/SvgFileFormat.h>
#  include <Texture/Image/Image.h>

#  include <Foundation/IO/StreamUtils.h>
#  include <Foundation/Profiling/Profiling.h>

#  include <lunasvg.h>

EZ_STATICLINK_FORCE static ezImageFileFormatRegistrator<ezSvgFileFormat> g_SvgFileFormat;

ezResult ezSvgFileFormat::ReadImageHeader(ezStreamReader& inout_stream, ezImageHeader& ref_header, ezStringView sFileExtension) const
{
  EZ_IGNORE_UNUSED(inout_stream);
  EZ_IGNORE_UNUSED(sFileExtension);

  ref_header.SetWidth(m_uiResolutionX);
  ref_header.SetHeight(m_uiResolutionY);
  ref_header.SetImageFormat(ezImageFormat::R8G8B8A8_UNORM);
  ref_header.SetNumMipLevels(1);
  ref_header.SetNumArrayIndices(1);
  ref_header.SetNumFaces(1);
  ref_header.SetDepth(1);

  return EZ_SUCCESS;
}

ezResult ezSvgFileFormat::ReadImage(ezStreamReader& inout_stream, ezImage& ref_image, ezStringView sFileExtension) const
{
  EZ_IGNORE_UNUSED(sFileExtension);
  EZ_PROFILE_SCOPE("ezSvgFileFormat::ReadImage");

  // Read entire SVG file into memory
  ezDynamicArray<ezUInt8> fileBuffer;
  ezStreamUtils::ReadAllAndAppend(inout_stream, fileBuffer);

  if (fileBuffer.IsEmpty())
  {
    ezLog::Error("SVG file is empty");
    return EZ_FAILURE;
  }

  // Parse SVG document
  std::unique_ptr<lunasvg::Document> document = lunasvg::Document::loadFromData(
    reinterpret_cast<const char*>(fileBuffer.GetData()),
    fileBuffer.GetCount());

  if (!document)
  {
    ezLog::Error("Failed to parse SVG file");
    return EZ_FAILURE;
  }

  // Render SVG to bitmap at default resolution
  lunasvg::Bitmap bitmap = document->renderToBitmap(
    static_cast<int>(m_uiResolutionX),
    static_cast<int>(m_uiResolutionY),
    m_Background.ToRGBA8());

  if (bitmap.isNull() || bitmap.data() == nullptr)
  {
    ezLog::Error("Failed to render SVG to bitmap");
    return EZ_FAILURE;
  }

  // Convert from ARGB32 premultiplied to RGBA
  bitmap.convertToRGBA();

  // Set up image header
  ezImageHeader header;
  header.SetWidth(m_uiResolutionX);
  header.SetHeight(m_uiResolutionY);
  header.SetImageFormat(ezImageFormat::R8G8B8A8_UNORM);
  header.SetNumMipLevels(1);
  header.SetNumArrayIndices(1);
  header.SetNumFaces(1);
  header.SetDepth(1);

  ref_image.ResetAndAlloc(header);

  // Copy pixel data
  const ezUInt32 pixelCount = m_uiResolutionX * m_uiResolutionY;
  ezMemoryUtils::Copy(ref_image.GetBlobPtr<ezUInt8>().GetPtr(), bitmap.data(), pixelCount * 4);

  return EZ_SUCCESS;
}

ezResult ezSvgFileFormat::WriteImage(ezStreamWriter& inout_stream, const ezImageView& image, ezStringView sFileExtension) const
{
  EZ_IGNORE_UNUSED(inout_stream);
  EZ_IGNORE_UNUSED(image);
  EZ_IGNORE_UNUSED(sFileExtension);

  ezLog::Error("Writing SVG files is not supported");
  return EZ_FAILURE;
}

bool ezSvgFileFormat::CanReadFileType(ezStringView sExtension) const
{
  return sExtension.IsEqual_NoCase("svg");
}

bool ezSvgFileFormat::CanWriteFileType(ezStringView sExtension) const
{
  EZ_IGNORE_UNUSED(sExtension);
  return false;
}

#endif

EZ_STATICLINK_FILE(Texture, Texture_Image_Formats_SvgFileFormat);
