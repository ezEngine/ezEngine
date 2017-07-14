#include <PCH.h>
#include <ToolsFoundation/Basics/RecentFilesList.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/Logging/Log.h>

void ezRecentFilesList::Insert(const char* szFile, ezInt32 iContainerWindow)
{
  ezStringBuilder sCleanPath = szFile;
  sCleanPath.MakeCleanPath();

  ezString s = sCleanPath;

  for (ezUInt32 i = 0; i < m_Files.GetCount(); i++)
  {
    if (m_Files[i].m_File == s)
    {
      m_Files.RemoveAt(i);
      break;
    }
  }
  m_Files.PushFront(RecentFile(s, iContainerWindow));

  if (m_Files.GetCount() > m_uiMaxElements)
    m_Files.SetCount(m_uiMaxElements);
}

void ezRecentFilesList::Save(const char* szFile)
{
  ezDeferredFileWriter File;
  File.SetOutput(szFile);

  for (const RecentFile& file : m_Files)
  {
    ezStringBuilder sTemp;
    sTemp.Format("{0}|{1}", file.m_File, file.m_iContainerWindow);
    File.WriteBytes(sTemp.GetData(), sTemp.GetElementCount());
    File.WriteBytes("\n", sizeof(char));
  }

  if (File.Close().Failed())
    ezLog::Error("Unable to open file '{0}' for writing!", szFile);
}

void ezRecentFilesList::Load(const char* szFile)
{
  m_Files.Clear();

  ezFileReader File;
  if (File.Open(szFile).Failed())
    return;

  ezStringBuilder sAllLines;
  sAllLines.ReadAll(File);

  ezHybridArray<ezStringView, 16> Lines;
  sAllLines.Split(false, Lines, "\n");

  for (const ezStringView& sv : Lines)
  {
    ezStringBuilder sTemp = sv;
    ezHybridArray<ezStringView, 2> Parts;
    sTemp.Split(false, Parts, "|");
    if (Parts.GetCount() == 1)
    {
      m_Files.PushBack(RecentFile(Parts[0], 0));
    }
    else if (Parts.GetCount() == 2)
    {
      ezStringBuilder sContainer = Parts[1];
      ezInt32 iContainerWindow = 0;
      ezConversionUtils::StringToInt(sContainer, iContainerWindow);
      m_Files.PushBack(RecentFile(Parts[0], iContainerWindow));
    }

  }
}
