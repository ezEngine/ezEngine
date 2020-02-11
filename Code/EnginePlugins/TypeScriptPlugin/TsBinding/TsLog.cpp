#include <TypeScriptPluginPCH.h>

#include <TypeScriptPlugin/TsBinding/TsBinding.h>

static int __CPP_Log(duk_context* pDuk);

ezResult ezTypeScriptBinding::Init_Log()
{
  m_Duk.RegisterGlobalFunction("__CPP_Log_Error", __CPP_Log, 1, ezLogMsgType::ErrorMsg);
  m_Duk.RegisterGlobalFunction("__CPP_Log_SeriousWarning", __CPP_Log, 1, ezLogMsgType::SeriousWarningMsg);
  m_Duk.RegisterGlobalFunction("__CPP_Log_Warning", __CPP_Log, 1, ezLogMsgType::WarningMsg);
  m_Duk.RegisterGlobalFunction("__CPP_Log_Success", __CPP_Log, 1, ezLogMsgType::SuccessMsg);
  m_Duk.RegisterGlobalFunction("__CPP_Log_Info", __CPP_Log, 1, ezLogMsgType::InfoMsg);
  m_Duk.RegisterGlobalFunction("__CPP_Log_Dev", __CPP_Log, 1, ezLogMsgType::DevMsg);
  m_Duk.RegisterGlobalFunction("__CPP_Log_Debug", __CPP_Log, 1, ezLogMsgType::DebugMsg);

  return EZ_SUCCESS;
}

static int __CPP_Log(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);
  const ezInt16 iMagic = duk.GetFunctionMagicValue();

  switch (iMagic)
  {
    case ezLogMsgType::ErrorMsg:
      ezLog::Error(duk.GetStringValue(0));
      break;
    case ezLogMsgType::SeriousWarningMsg:
      ezLog::SeriousWarning(duk.GetStringValue(0));
      break;
    case ezLogMsgType::WarningMsg:
      ezLog::Warning(duk.GetStringValue(0));
      break;
    case ezLogMsgType::SuccessMsg:
      ezLog::Success(duk.GetStringValue(0));
      break;
    case ezLogMsgType::InfoMsg:
      ezLog::Info(duk.GetStringValue(0));
      break;
    case ezLogMsgType::DevMsg:
      ezLog::Dev(duk.GetStringValue(0));
      break;
    case ezLogMsgType::DebugMsg:
      ezLog::Debug(duk.GetStringValue(0));
      break;
  }

  return duk.ReturnVoid();
}
