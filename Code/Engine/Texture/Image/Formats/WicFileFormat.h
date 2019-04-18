#pragma once

#include <Texture/Image/Formats/ImageFileFormat.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)

/// \brief File format implementation for loading TIFF files using WIC
class EZ_TEXTURE_DLL ezWicFileFormat : public ezImageFileFormat
{
public:

  ezWicFileFormat();
  virtual ~ezWicFileFormat();

  virtual ezResult ReadImage(ezStreamReader& stream, ezImage& image, ezLogInterface* pLog, const char* szFileExtension) const override;
  virtual ezResult WriteImage(ezStreamWriter& stream, const ezImageView& image, ezLogInterface* pLog, const char* szFileExtension) const override;

  virtual bool CanReadFileType(const char* szExtension) const override;
  virtual bool CanWriteFileType(const char* szExtension) const override;

private:
  mutable bool m_bTryCoInit = true;           // Helper for keeping track of whether we have tried to init COM exactly once
  mutable bool m_bCoUninitOnShutdown = false; // Helper for keeping track of whether we have to uninitialize COM (because we were the first to initialize it)
};

#endif
