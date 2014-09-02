#include <Foundation/PCH.h>
#include <Foundation/Logging/HTMLWriter.h>

ezLogWriter::HTML::~HTML()
{
  EndLog ();
}

void ezLogWriter::HTML::BeginLog (const char* szFile, const char* szAppTitle)
{
  const ezUInt32 uiLogCache = 1024 * 10;

  if (m_File.Open(szFile, uiLogCache) == EZ_FAILURE)
  {
    for (ezUInt32 i = 1; i < 32; ++i)
    {
      const ezStringBuilder sName = ezPathUtils::GetFileName(szFile);

      ezStringBuilder sNewName;
      sNewName.Format("%s_%i", sName.GetData(), i);

      ezStringBuilder sPath = szFile;
      sPath.ChangeFileName(sNewName.GetData());

      if (m_File.Open(sPath.GetData(), uiLogCache) == EZ_SUCCESS)
        break;
    }
  }

  if (!m_File.IsOpen())
  {
    ezLog::Error("Could not open Log-File \"%s\".", szFile);
    return;
  }

  ezStringBuilder sText;
  sText.Format("<HTML><HEAD><META HTTP-EQUIV=\"Content-Type\" content=\"text/html; charset=utf-8\"><TITLE>Log - %s</TITLE></HEAD><BODY>", szAppTitle);

  m_File.WriteBytes(sText.GetData(), sizeof (char) * sText.GetElementCount());
}

void ezLogWriter::HTML::EndLog()
{
  if (!m_File.IsOpen())
    return;

  WriteString("", 0);
  WriteString("", 0);
  WriteString(" <<< HTML-Log End >>> ", 0);
  WriteString("", 0);
  WriteString("", 0);

  ezStringBuilder sText;
  sText.Format("</BODY></HTML>");

  m_File.WriteBytes(sText.GetData(), sizeof (char) * sText.GetElementCount());

  m_File.Close ();
}

const ezFileWriter& ezLogWriter::HTML::GetOpenedLogFile() const
{
  return m_File;
}

void ezLogWriter::HTML::LogMessageHandler(const ezLoggingEventData& eventData)
{
  if (!m_File.IsOpen())
    return;

  ezStringBuilder sText;
  ezStringBuilder sOriginalText = eventData.m_szText;

  ezStringBuilder sTag = eventData.m_szTag;

  // Cannot write <, > or & to HTML, must be escaped
  sOriginalText.ReplaceAll ("&", "&amp;");
  sOriginalText.ReplaceAll ("<", "&lt;");
  sOriginalText.ReplaceAll (">", "&gt;");
  sOriginalText.ReplaceAll ("\n", "<br>\n");

  sTag.ReplaceAll ("&", "&amp;");
  sTag.ReplaceAll ("<", "&lt;");
  sTag.ReplaceAll (">", "&gt;");

  bool bFlushWriteCache = false;

  switch (eventData.m_EventType)
  {
  case ezLogMsgType::BeginGroup:
    sText.Format("<br><font color=\"#8080FF\"><b> <<< <u>%s</u> >>> </b> (%s) </font><br><table width=100%% border=0><tr width=100%%><td width=10></td><td width=*>\n", sOriginalText.GetData(), sTag.GetData());
    break;
  case ezLogMsgType::EndGroup:
    sText.Format("</td></tr></table><font color=\"#8080FF\"><b> <<< %s >>> </b></font><br><br>\n", sOriginalText.GetData());
    break;
  case ezLogMsgType::ErrorMsg:
    bFlushWriteCache = true;
    sText.Format("<font color=\"#FF0000\"><b><u>Error:</u> %s</b></font><br>\n", sOriginalText.GetData());
    break;
  case ezLogMsgType::SeriousWarningMsg:
    bFlushWriteCache = true;
    sText.Format("<font color=\"#FF4000\"><b><u>Seriously:</u> %s</b></font><br>\n", sOriginalText.GetData());
    break;
  case ezLogMsgType::WarningMsg:
    sText.Format("<font color=\"#FF8000\"><u>Warning:</u> %s</font><br>\n", sOriginalText.GetData());
    break;
  case ezLogMsgType::SuccessMsg:
    sText.Format("<font color=\"#009000\">%s</font><br>\n", sOriginalText.GetData());
    break;
  case ezLogMsgType::InfoMsg:
    sText.Format("<font color=\"#000000\">%s</font><br>\n", sOriginalText.GetData());
    break;
  case ezLogMsgType::DevMsg:
    sText.Format("<font color=\"#3030F0\">%s</font><br>\n", sOriginalText.GetData());
    break;
  case ezLogMsgType::DebugMsg:
    sText.Format("<font color=\"#A000FF\">%s</font><br>\n", sOriginalText.GetData());
    break;
  default:
    sText.Format("<font color=\"#A0A0A0\">%s</font><br>\n", sOriginalText.GetData());

    ezLog::Warning ("Unknown Message Type %d", eventData.m_EventType);
    break;
  }

  if (!sText.IsEmpty())
    m_File.WriteBytes(sText.GetData(), sizeof (char) * sText.GetElementCount());

  if (bFlushWriteCache)
    m_File.Flush();
}

void ezLogWriter::HTML::WriteString(const char* szString, ezUInt32 uiColor)
{
  ezStringBuilder sTemp;
  sTemp.Format("<font color=\"#%X\">%s</font>", uiColor, szString);

  m_File.WriteBytes(sTemp.GetData(), sizeof (char) * sTemp.GetElementCount());
}


EZ_STATICLINK_FILE(Foundation, Foundation_Logging_Implementation_HTMLWriter);

