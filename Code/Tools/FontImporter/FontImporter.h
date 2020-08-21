#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Strings/String.h>
#include <FontImportOptions.h>
#include <ft2build.h>
#include FT_FREETYPE_H

class ezFontImporter
{
public:
  void Startup();
  ezResult Import(const ezString& inputFile, const ezFontImportOptions& importOptions, ezRawFont& outFont, bool saveFontAtlas = false);
  void Shutdown();

private:
  FT_Library m_Library;
  const static int sMaximumTextureSize = 8192;
};
