#include <PCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

void ezQtEditorApp::GetKnownInputSlots(ezDynamicArray<ezString>& slotList) const
{
  if (slotList.IndexOf("") == ezInvalidIndex)
    slotList.PushBack("");

  ezStringBuilder sFile;
  ezDynamicArray<ezStringView> Lines;

  ezStringBuilder sSearchDir = ezApplicationServices::GetSingleton()->GetApplicationDataFolder();
  sSearchDir.AppendPath("InputSlots/*.txt");

  ezFileSystemIterator it;
  if (it.StartSearch(sSearchDir, false, false).Succeeded())
  {
    do
    {
      sFile = it.GetCurrentPath();
      sFile.AppendPath(it.GetStats().m_sFileName);

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

          if (slotList.IndexOf(sSlot) == ezInvalidIndex)
            slotList.PushBack(sSlot);
        }
      }
    } while (it.Next().Succeeded());
  }
}
