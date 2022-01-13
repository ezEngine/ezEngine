module ez.foundation.logging.log;

public import std.format;

struct ezLog
{
  import std.string;

  static void Error(string text)
  {
    DLangLog.Error(toStringz(text));
  }

  static void SeriousWarning(string text)
  {
    DLangLog.SeriousWarning(toStringz(text));
  }

  static void Warning(string text)
  {
    DLangLog.Warning(toStringz(text));
  }

  static void Success(string text)
  {
    DLangLog.Success(toStringz(text));
  }

  static void Info(string text)
  {
    DLangLog.Info(toStringz(text));
  }

  static void Dev(string text)
  {
    DLangLog.Dev(toStringz(text));
  }

  static void Debug(string text)  
  {
    DLangLog.Debug(toStringz(text));
  }
}

private extern(C++, struct) struct DLangLog
{
  // CODEGEN-BEGIN: Struct("DLangLog")
  static void Error(const(char)* text);
  static void SeriousWarning(const(char)* text);
  static void Warning(const(char)* text);
  static void Success(const(char)* text);
  static void Info(const(char)* text);
  static void Dev(const(char)* text);
  static void Debug(const(char)* text);
  // CODEGEN-END
}
