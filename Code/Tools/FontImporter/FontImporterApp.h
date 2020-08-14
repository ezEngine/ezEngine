#pragma once

#include <Foundation/Application/Application.h>
#include <RendererCore/Font/FontResource.h>
#include <FontImporter.h>

class ezStreamWriter;

class ezFontImporterApp : public ezApplication
{
public:
  typedef ezApplication SUPER;

  ezFontImporterApp();

  virtual ezResult BeforeCoreSystemsStartup() override;
  virtual void AfterCoreSystemsStartup() override;
  virtual void BeforeCoreSystemsShutdown() override;
  virtual ApplicationExecution Run() override;

  ezResult ParseCommandLine();
  ezResult ParseInputFile();
  ezResult ParseOutputFile();

  bool ParseFile(const char* szOption, ezString& result) const;

  ezResult WriteFontFile(ezStreamWriter& stream, const ezRawFont& font);
  ezResult WriteOutputFile(const char* szFile, const ezRawFont& font);

private:
  ezString m_sInputFile;
  ezString m_sOutputFile;
  ezString m_sOutputThumbnailFile;
  ezFontImporter m_FontImporter;
};
