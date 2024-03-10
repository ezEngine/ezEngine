#pragma once

#include <Texture/Image/Formats/ImageFileFormat.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)

/// \brief File format implementation for loading TIFF files using WIC
class EZ_TEXTURE_DLL ezWicFileFormat : public ezImageFileFormat
{
public:
  ezWicFileFormat();
  virtual ~ezWicFileFormat();

  virtual ezResult ReadImageHeader(ezStreamReader& inout_stream, ezImageHeader& ref_header, ezStringView sFileExtension) const override;
  virtual ezResult ReadImage(ezStreamReader& inout_stream, ezImage& ref_image, ezStringView sFileExtension) const override;
  virtual ezResult WriteImage(ezStreamWriter& inout_stream, const ezImageView& image, ezStringView sFileExtension) const override;

  virtual bool CanReadFileType(ezStringView sExtension) const override;
  virtual bool CanWriteFileType(ezStringView sExtension) const override;

private:
  mutable bool m_bTryCoInit = true; // Helper for keeping track of whether we have tried to init COM exactly once
  mutable bool m_bCoUninitOnShutdown =
    false;                          // Helper for keeping track of whether we have to uninitialize COM (because we were the first to initialize it)

  ezResult ReadFileData(ezStreamReader& stream, ezDynamicArray<ezUInt8>& storage) const;
};

#endif
