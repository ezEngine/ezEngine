#include <GuiFoundation/GuiFoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_LINUX)

#  include <GuiFoundation/UIServices/UIServices.moc.h>
#  include <ToolsFoundation/Application/ApplicationServices.h>
#  include <ToolsFoundation/Project/ToolsProject.h>

#  include <Foundation/IO/OSFile.h>

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

  QProcess::startDetached("xdg-open", {ezMakeQString(sPath)});
}

ezStatus ezQtUiServices::OpenInVsCode(const QStringList& arguments)
{
  {
    ezStringBuilder sDstDir = ezToolsProject::GetSingleton()->GetProjectDirectory();
    sDstDir.AppendPath(".vscode");

    ezStringBuilder sSrcDir = ezApplicationServices::GetSingleton()->GetApplicationDataFolder();
    sSrcDir.AppendPath("VSC");
    ezOSFile::CopyFolder(sSrcDir, sDstDir).IgnoreResult();
  }

  const QString sVsCodeExe = QStandardPaths::findExecutable("code");
  if (sVsCodeExe.isEmpty())
  {
    return ezStatus("Installation of Visual Studio Code could not be located.\n"
                    "Please visit 'https://code.visualstudio.com/download' to download Visual Studio Code.");
  }

  if (!QProcess::startDetached(sVsCodeExe, arguments))
  {
    return ezStatus("Failed to launch Visual Studio Code.");
  }

  return ezStatus(EZ_SUCCESS);
}

#endif
