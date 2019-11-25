#include <TypeScriptPluginPCH.h>

#include <TypeScriptPlugin/TsBinding/TsBinding.h>

static int __CPP_Utils_StringToHash(duk_context* pDuk);

ezResult ezTypeScriptBinding::Init_Utils()
{
  m_Duk.RegisterGlobalFunction("__CPP_Utils_StringToHash", __CPP_Utils_StringToHash, 1);

  return EZ_SUCCESS;
}

static int __CPP_Utils_StringToHash(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  return duk.ReturnUInt(ezTempHashedString::ComputeHash(duk.GetStringValue(0)));
}
