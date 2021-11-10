#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/CppProjectDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

ezQtCppProjectDlg::ezQtCppProjectDlg(QWidget* parent)
  : QDialog(parent)
{
  setupUi(this);
}

void ezQtCppProjectDlg::on_Result_accepted()
{
  const ezString sProjectName = "MyGame";
  ezStringBuilder sProjectNameUpper = sProjectName;
  sProjectNameUpper.ToUpper();

  const ezStringBuilder sTargetDir = ezToolsProject::GetSingleton()->GetProjectDirectory();

  ezStringBuilder sSourceDir = ezApplicationServices::GetSingleton()->GetApplicationDataFolder();
  sSourceDir.AppendPath("CppProject");

  ezDynamicArray<ezFileStats> items;
  ezOSFile::GatherAllItemsInFolder(items, sSourceDir, ezFileSystemIteratorFlags::ReportFilesRecursive);

  ezHybridArray<ezString, 32> filesCopied;

  for (const auto& item : items)
  {
    ezStringBuilder srcPath, dstPath;
    item.GetFullPath(srcPath);

    dstPath = srcPath;
    dstPath.MakeRelativeTo(sSourceDir).IgnoreResult();

    dstPath.ReplaceAll("CppProject", sProjectName);
    dstPath.Prepend(sTargetDir, "/");
    dstPath.MakeCleanPath();

    if (ezOSFile::CopyFile(srcPath, dstPath).Failed())
    {
      ezQtUiServices::GetSingleton()->MessageBoxWarning(ezFmt("Failed to copy the C++ project files.\nSource: '{}'\nDestination: '{}'", srcPath, dstPath));
      return;
    }

    filesCopied.PushBack(dstPath);
  }

  for (const auto& filePath : filesCopied)
  {
    ezStringBuilder content;

    {
      ezFileReader file;
      if (file.Open(filePath).Failed())
      {
        ezQtUiServices::GetSingleton()->MessageBoxWarning(ezFmt("Failed to open C++ project file for reading.\nSource: '{}'", filePath));
        return;
      }

      content.ReadAll(file);
    }

    content.ReplaceAll("!CppProject!", sProjectName);
    content.ReplaceAll("!CPPPROJECT!", sProjectNameUpper);

    {
      ezFileWriter file;
      if (file.Open(filePath).Failed())
      {
        ezQtUiServices::GetSingleton()->MessageBoxWarning(ezFmt("Failed to open C++ project file for writing.\nSource: '{}'", filePath));
        return;
      }

      file.WriteBytes(content.GetData(), content.GetElementCount()).IgnoreResult();
    }
  }

  // run CMake
  {
    ezStringBuilder sBuildDir;
    sBuildDir.Format("{}/CppSource/Build/Vs2019x64", sTargetDir);

    if (ezOSFile::ExistsDirectory(sBuildDir) && ezOSFile::DeleteFolder(sBuildDir).Failed())
    {
      ezQtUiServices::GetSingleton()->MessageBoxInformation(ezFmt("Couldn't delete build output directory:\n{}", sBuildDir));
    }

    ezStringBuilder tmp;

    QStringList args;
    args << "-S";

    tmp.Format("{}/CppSource", sTargetDir);
    args << tmp.GetData();

    args << "-DEZ_ENABLE_FOLDER_UNITY_FILES:BOOL=OFF";

    args << "-G";
    args << "Visual Studio 16 2019";

    args << "-B";
    args << sBuildDir.GetData();

    args << "-A";
    args << "x64";

    ezLogSystemToBuffer log;

    ezStatus res = ezQtEditorApp::GetSingleton()->ExecuteTool("cmake/bin/cmake.exe", args, 120, &log, ezLogMsgType::InfoMsg);
    ezQtUiServices::GetSingleton()->MessageBoxStatus(res, "CMake execution failed");

    if (res.Failed())
    {
      ezQtUiServices::GetSingleton()->MessageBoxInformation(log.m_sBuffer);
      return;
    }

    ezQtUiServices::OpenInExplorer(sBuildDir, false);
  }


  accept();
}

void ezQtCppProjectDlg::on_Result_rejected()
{
  reject();
}
