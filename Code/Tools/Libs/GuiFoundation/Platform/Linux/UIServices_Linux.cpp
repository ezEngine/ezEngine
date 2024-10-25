#include <GuiFoundation/GuiFoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_LINUX)

#  include <GuiFoundation/UIServices/UIServices.moc.h>

void ezQtUiServices::OpenInExplorer(const char* szPath, bool bIsFile)
{
  QStringList args;
  ezStringBuilder parentDir;

  if (bIsFile)
  {
    parentDir = szPath;
    parentDir = parentDir.GetFileDirectory();
    szPath = parentDir.GetData();
  }
  args << QDir::toNativeSeparators(szPath);

  QProcess::startDetached("xdg-open", args);
}

void ezQtUiServices::OpenWith(const char* szPath)
{
  ezStringBuilder sPath = szPath;
  sPath.MakeCleanPath();
  sPath.MakePathSeparatorsNative();

  ezLog::Error("ezQtUiServices::OpenWith() not implemented on Linux");
}

ezStatus ezQtUiServices::OpenInVsCode(const QStringList& arguments)
{
  ezLog::Error("ezQtUiServices::OpenInVsCode() not implemented on Linux");
  return ezStatus(EZ_FAILURE);
}

#endif
