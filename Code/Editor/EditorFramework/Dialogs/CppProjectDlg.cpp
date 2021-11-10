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

  accept();
}

void ezQtCppProjectDlg::on_Result_rejected()
{
  reject();
}
