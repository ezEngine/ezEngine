#include <TypeScriptPluginPCH.h>

#include <Duktape/duktape.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

//static int __CPP_World_DeleteObjectDelayed(duk_context* pDuk);

ezResult ezTypeScriptBinding::Init_Physics()
{
  //m_Duk.RegisterGlobalFunction("__CPP_World_DeleteObjectDelayed", __CPP_World_DeleteObjectDelayed, 1);

  return EZ_SUCCESS;
}


//static int __CPP_World_DeleteObjectDelayed(duk_context* pDuk)
//{
//}
