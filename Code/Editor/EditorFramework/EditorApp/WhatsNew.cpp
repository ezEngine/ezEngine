#include <EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/WhatsNew.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Strings/StringBuilder.h>

ezWhatsNewText::ezWhatsNewText() = default;
ezWhatsNewText::~ezWhatsNewText() = default;

void ezWhatsNewText::Load(const char* szFile)
{
  {
    ezFileReader file;
    if (file.Open(szFile).Failed())
      return;

    m_sText.ReadAll(file);

      ezStringBuilder tmp;

    if (const char* szSkipStart = m_sText.FindSubString("<!-- SKIP START -->"))
    {
      tmp = szSkipStart + ezStringUtils::GetStringElementCount("<!-- SKIP START -->");
      m_sText = tmp;
    }

    if (const char* szSkipEnd = m_sText.FindSubString("<!-- SKIP END -->"))
    {
      tmp.SetSubString_FromTo(m_sText.GetData(), szSkipEnd);
      m_sText = tmp;
    }
  }

  {
    ezFileReader file;
    if (file.Open(":appdata/LastWhatsNew.htm").Failed())
    {
      m_bHasChanged = true;
      return;
    }

    ezStringBuilder sOldWhatsNew;
    sOldWhatsNew.ReadAll(file);

    m_bHasChanged = m_sText != sOldWhatsNew;
  }
}

void ezWhatsNewText::StoreLastRead()
{
  m_bHasChanged = false;

  ezFileWriter file;
  if (file.Open(":appdata/LastWhatsNew.htm").Failed())
    return;

  file.WriteBytes(m_sText.GetData(), m_sText.GetElementCount());
}
