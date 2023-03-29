#pragma once

#include <Texture/Image/Formats/ImageFileFormat.h>

class EZ_TEXTURE_DLL ezDdsFileFormat : public ezImageFileFormat
{
public:
  virtual ezResult ReadImageHeader(ezStreamReader& inout_stream, ezImageHeader& ref_header, const char* szFileExtension) const override;
  virtual ezResult ReadImage(ezStreamReader& inout_stream, ezImage& ref_image, const char* szFileExtension) const override;
  virtual ezResult WriteImage(ezStreamWriter& inout_stream, const ezImageView& image, const char* szFileExtension) const override;

  virtual bool CanReadFileType(const char* szExtension) const override;
  virtual bool CanWriteFileType(const char* szExtension) const override;
};
