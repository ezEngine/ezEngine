#include <GuiFoundation/GuiFoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_LINUX)

#  include <GuiFoundation/UIServices/UIServices.moc.h>

void ezQtUiServices::OpenInExplorer(ezStringView sPath, bool bIsFile)
{
  QStringList args;
  ezStringBuilder parentDir;

  if (bIsFile)
  {
    parentDir = sPath;
    parentDir = parentDir.GetFileDirectory();
    sPath = parentDir.GetData();
  }
  args << QDir::toNativeSeparators(ezMakeQString(sPath));

  QProcess::startDetached("xdg-open", args);
}

void ezQtUiServices::OpenWith(ezStringView sPath0)
{
  ezStringBuilder sPath = sPath0;
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
