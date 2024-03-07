#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/StackTraceLogParser.h>

#include <EditorFramework/CodeGen/CppProject.h>
#include <GuiFoundation/Widgets/LogWidget.moc.h>

namespace ezStackTraceLogParser
{
  static void StackTraceLogCallback(const ezStringView& sLogText)
  {
    ezStringView sFileName;
    ezInt32 lineNumber;

    if (!ParseStackTraceFileNameAndLineNumber(sLogText, sFileName, lineNumber))
      return;

    ezCppSettings cpp;
    cpp.Load().IgnoreResult();
    const ezStatus res = ezCppProject::OpenInCodeEditor(sFileName, lineNumber);
  }

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
  bool ParseStackTraceFileNameAndLineNumber(const ezStringView& sLine, ezStringView& ref_sFileName, ezInt32& ref_iLineNumber)
  {
    const char* szEndFileNameMarker = "(";
    const char* szEndLineNumberMarker = "):";

    const char* szEndLineNumber = sLine.FindSubString(szEndLineNumberMarker);
    if (szEndLineNumber == nullptr)
    {
      return false;
    }

    const char* szEndFileName = sLine.FindLastSubString(szEndFileNameMarker, szEndLineNumber);
    if (szEndFileName == nullptr)
    {
      return false;
    }

    ref_sFileName = ezStringView(sLine.GetStartPointer(), szEndFileName);
    ref_sFileName.Trim(" ");

    if (!ref_sFileName.IsAbsolutePath())
    {
      return false;
    }

    const ezResult res = ezConversionUtils::StringToInt(ezStringView(szEndFileName + 1, szEndLineNumber), ref_iLineNumber);
    if (res != EZ_SUCCESS)
    {
      return false;
    }

    return true;
  }
#else
  static bool ParseStackTraceFileNameAndLineNumber(const ezStringView& sLine, ezStringView& ref_sFileName, ezInt32& ref_iLineNumber)
  {
    return false;
  }
#endif

  void Register()
  {
    ezQtLogWidget::AddLogItemContextActionCallback("StackTraceLog", &StackTraceLogCallback);
  }

  void Unregister()
  {
    ezQtLogWidget::RemoveLogItemContextActionCallback("StackTraceLog");
  }
} // namespace ezStackTraceLogParser