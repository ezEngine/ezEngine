#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

void ezQtEditorApp::GetKnownInputSlots(ezDynamicArray<ezString>& ref_slotList) const
{
  if (ref_slotList.IndexOf("") == ezInvalidIndex)
    ref_slotList.PushBack("");

  ezStringBuilder sFile;
  ezDynamicArray<ezStringView> Lines;

  ezStringBuilder sSearchDir = ezApplicationServices::GetSingleton()->GetApplicationDataFolder();
  sSearchDir.AppendPath("InputSlots/*.txt");

  ezFileSystemIterator it;
  for (it.StartSearch(sSearchDir, ezFileSystemIteratorFlags::ReportFiles); it.IsValid(); it.Next())
  {
    sFile = it.GetCurrentPath();
    sFile.AppendPath(it.GetStats().m_sName);

    ezFileReader reader;
    if (reader.Open(sFile).Succeeded())
    {
      sFile.ReadAll(reader);

      Lines.Clear();
      sFile.Split(false, Lines, "\n", "\r");

      ezString sSlot;
      for (ezUInt32 s = 0; s < Lines.GetCount(); ++s)
      {
        sSlot = Lines[s];

        if (ref_slotList.IndexOf(sSlot) == ezInvalidIndex)
          ref_slotList.PushBack(sSlot);
      }
    }
  }
}
