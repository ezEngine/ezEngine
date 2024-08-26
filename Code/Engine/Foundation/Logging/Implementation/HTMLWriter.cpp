#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/HTMLWriter.h>
#include <Foundation/Time/Timestamp.h>

ezLogWriter::HTML::~HTML()
{
  EndLog();
}

void ezLogWriter::HTML::BeginLog(ezStringView sFile, ezStringView sAppTitle)
{
  const ezUInt32 uiLogCache = 1024 * 10;

  ezStringBuilder sNewName;
  if (m_File.Open(sFile.GetData(sNewName), uiLogCache, ezFileShareMode::SharedReads) == EZ_FAILURE)
  {
    for (ezUInt32 i = 1; i < 32; ++i)
    {
      const ezStringBuilder sName = ezPathUtils::GetFileName(sFile);

      sNewName.SetFormat("{0}_{1}", sName, i);

      ezStringBuilder sPath = sFile;
      sPath.ChangeFileName(sNewName);

      if (m_File.Open(sPath.GetData(), uiLogCache) == EZ_SUCCESS)
        break;
    }
  }

  if (!m_File.IsOpen())
  {
    ezLog::Error("Could not open Log-File \"{0}\".", sFile);
    return;
  }

  ezStringBuilder sText;
  sText.SetFormat("<HTML><HEAD><META HTTP-EQUIV=\"Content-Type\" content=\"text/html; charset=utf-8\"><TITLE>Log - {0}</TITLE></HEAD><BODY>", sAppTitle);

  m_File.WriteBytes(sText.GetData(), sizeof(char) * sText.GetElementCount()).IgnoreResult();
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
  sText.SetFormat("</BODY></HTML>");

  m_File.WriteBytes(sText.GetData(), sizeof(char) * sText.GetElementCount()).IgnoreResult();

  m_File.Close();
}

const ezFileWriter& ezLogWriter::HTML::GetOpenedLogFile() const
{
  return m_File;
}

void ezLogWriter::HTML::SetTimestampMode(ezLog::TimestampMode mode)
{
  m_TimestampMode = mode;
}

void ezLogWriter::HTML::LogMessageHandler(const ezLoggingEventData& eventData)
{
  if (!m_File.IsOpen())
    return;

  ezStringBuilder sOriginalText = eventData.m_sText;

  ezStringBuilder sTag = eventData.m_sTag;

  // Cannot write <, > or & to HTML, must be escaped
  sOriginalText.ReplaceAll("&", "&amp;");
  sOriginalText.ReplaceAll("<", "&lt;");
  sOriginalText.ReplaceAll(">", "&gt;");
  sOriginalText.ReplaceAll("\n", "<br>\n");

  sTag.ReplaceAll("&", "&amp;");
  sTag.ReplaceAll("<", "&lt;");
  sTag.ReplaceAll(">", "&gt;");

  ezStringBuilder sTimestamp;
  ezLog::GenerateFormattedTimestamp(m_TimestampMode, sTimestamp);

  bool bFlushWriteCache = false;

  ezStringBuilder sText;

  switch (eventData.m_EventType)
  {
    case ezLogMsgType::Flush:
      bFlushWriteCache = true;
      break;

    case ezLogMsgType::BeginGroup:
      sText.SetFormat("<br><font color=\"#8080FF\"><b> <<< <u>{0}</u> >>> </b> ({1}) </font><br><table width=100%% border=0><tr width=100%%><td "
                      "width=10></td><td width=*>\n",
        sOriginalText, sTag);
      break;

    case ezLogMsgType::EndGroup:
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
      sText.SetFormat("</td></tr></table><font color=\"#8080FF\"><b> <<< {0} ({1} sec)>>> </b></font><br><br>\n", sOriginalText, ezArgF(eventData.m_fSeconds, 4));
#else
      sText.SetFormat("</td></tr></table><font color=\"#8080FF\"><b> <<< {0} ({1})>>> </b></font><br><br>\n", sOriginalText, "timing info not available");
#endif
      break;

    case ezLogMsgType::ErrorMsg:
      bFlushWriteCache = true;
      sText.SetFormat("{0}<font color=\"#FF0000\"><b><u>Error:</u> {1}</b></font><br>\n", sTimestamp, sOriginalText);
      break;

    case ezLogMsgType::SeriousWarningMsg:
      bFlushWriteCache = true;
      sText.SetFormat("{0}<font color=\"#FF4000\"><b><u>Seriously:</u> {1}</b></font><br>\n", sTimestamp, sOriginalText);
      break;

    case ezLogMsgType::WarningMsg:
      sText.SetFormat("{0}<font color=\"#FF8000\"><u>Warning:</u> {1}</font><br>\n", sTimestamp, sOriginalText);
      break;

    case ezLogMsgType::SuccessMsg:
      sText.SetFormat("{0}<font color=\"#009000\">{1}</font><br>\n", sTimestamp, sOriginalText);
      break;

    case ezLogMsgType::InfoMsg:
      sText.SetFormat("{0}<font color=\"#000000\">{1}</font><br>\n", sTimestamp, sOriginalText);
      break;

    case ezLogMsgType::DevMsg:
      sText.SetFormat("{0}<font color=\"#3030F0\">{1}</font><br>\n", sTimestamp, sOriginalText);
      break;

    case ezLogMsgType::DebugMsg:
      sText.SetFormat("{0}<font color=\"#A000FF\">{1}</font><br>\n", sTimestamp, sOriginalText);
      break;

    default:
      sText.SetFormat("{0}<font color=\"#A0A0A0\">{1}</font><br>\n", sTimestamp, sOriginalText);

      ezLog::Warning("Unknown Message Type {1}", eventData.m_EventType);
      break;
  }

  if (!sText.IsEmpty())
  {
    m_File.WriteBytes(sText.GetData(), sizeof(char) * sText.GetElementCount()).IgnoreResult();
  }

  if (bFlushWriteCache)
  {
    m_File.Flush().IgnoreResult();
  }
}

void ezLogWriter::HTML::WriteString(ezStringView sText, ezUInt32 uiColor)
{
  ezStringBuilder sTemp;
  sTemp.SetFormat("<font color=\"#{0}\">{1}</font>", ezArgU(uiColor, 1, false, 16, true), sText);

  m_File.WriteBytes(sTemp.GetData(), sizeof(char) * sTemp.GetElementCount()).IgnoreResult();
}
