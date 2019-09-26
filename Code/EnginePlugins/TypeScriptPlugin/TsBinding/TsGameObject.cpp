#include <TypeScriptPluginPCH.h>

#include <TypeScriptPlugin/TsBinding/TsBinding.h>

static int __CPP_GameObject_SetLocalPosition(duk_context* pContext);

ezResult ezTypeScriptBinding::Init_GameObject()
{
  m_Duk.RegisterFunction("__CPP_GameObject_SetLocalPosition", __CPP_GameObject_SetLocalPosition, 4);

  return EZ_SUCCESS;
}

static int __CPP_GameObject_SetLocalPosition(duk_context* pContext)
{
  ezDuktapeFunction duk(pContext);
  EZ_VERIFY(duk.IsParameterObject(0), "");

  ezWorld* pWorld = nullptr;

  // retrieve ezWorld* in global stash
  {
    // TODO: look up ezWorld* externally instead of through the stash

    duk.OpenGlobalStashObject();

    duk_get_prop_index(duk, -1, 0 /* index for ezWorld* */);
    pWorld = *reinterpret_cast<ezWorld**>(duk_get_buffer(duk, -1, nullptr));
    duk_pop(duk);

    duk.CloseObject();
  }

  ezGameObjectHandle hObject;
  ezGameObject* pGameObject = nullptr;

  {
    duk_get_prop_string(duk, 0, "ezGameObjectHandle");
    hObject = *reinterpret_cast<ezGameObjectHandle*>(duk_get_buffer(duk, -1, nullptr));
    duk_pop(duk);

    EZ_VERIFY(pWorld->TryGetObject(hObject, pGameObject), "");
  }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  {
    duk_get_prop_string(duk, 0, "ezGameObjectPtr");
    ezGameObject* pGo = (ezGameObject*)duk_get_pointer_default(duk, -1, nullptr);
    duk_pop(duk);

    EZ_VERIFY(pGo == pGameObject, "outdated pointer");
  }
#endif

  if (pGameObject)
  {
    EZ_VERIFY(pWorld == pGameObject->GetWorld(), "");

    ezVec3 pos(duk.GetFloatParameter(1), duk.GetFloatParameter(2), duk.GetFloatParameter(3));
    pGameObject->SetLocalPosition(pos);
  }

  return duk.ReturnVoid();
}
