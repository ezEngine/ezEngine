#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

#include <Duktape/duktape.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

static int __CPP_RNG_Get0(duk_context* pDuk);
static int __CPP_RNG_Get1(duk_context* pDuk);
static int __CPP_RNG_Get2(duk_context* pDuk);

ezResult ezTypeScriptBinding::Init_Random()
{
  m_Duk.RegisterGlobalFunction("__CPP_RNG_Bool", __CPP_RNG_Get0, 0, 0);
  m_Duk.RegisterGlobalFunction("__CPP_RNG_DoubleZeroToOneExclusive", __CPP_RNG_Get0, 0, 1);
  m_Duk.RegisterGlobalFunction("__CPP_RNG_DoubleZeroToOneInclusive", __CPP_RNG_Get0, 0, 2);

  m_Duk.RegisterGlobalFunction("__CPP_RNG_UIntInRange", __CPP_RNG_Get1, 1, 0);
  m_Duk.RegisterGlobalFunction("__CPP_RNG_DoubleVarianceAroundZero", __CPP_RNG_Get1, 1, 1);

  m_Duk.RegisterGlobalFunction("__CPP_RNG_IntInRange", __CPP_RNG_Get2, 2, 0);
  m_Duk.RegisterGlobalFunction("__CPP_RNG_IntMinMax", __CPP_RNG_Get2, 2, 1);
  m_Duk.RegisterGlobalFunction("__CPP_RNG_DoubleInRange", __CPP_RNG_Get2, 2, 2);
  m_Duk.RegisterGlobalFunction("__CPP_RNG_DoubleMinMax", __CPP_RNG_Get2, 2, 3);
  m_Duk.RegisterGlobalFunction("__CPP_RNG_DoubleVariance", __CPP_RNG_Get2, 2, 4);

  return EZ_SUCCESS;
}

static int __CPP_RNG_Get0(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);
  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(duk);

  switch (duk.GetFunctionMagicValue())
  {
    case 0: // Bool()
      duk.ReturnBool(pWorld->GetRandomNumberGenerator().Bool());
      break;

    case 1: // DoubleZeroToOneExclusive()
      duk.ReturnNumber(pWorld->GetRandomNumberGenerator().DoubleZeroToOneExclusive());
      break;

    case 2: // DoubleZeroToOneInclusive()
      duk.ReturnNumber(pWorld->GetRandomNumberGenerator().DoubleZeroToOneInclusive());
      break;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }

  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, 1, +1);
}

static int __CPP_RNG_Get1(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);
  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(duk);

  switch (duk.GetFunctionMagicValue())
  {
    case 0: // UIntInRange()
      duk.ReturnUInt(pWorld->GetRandomNumberGenerator().UIntInRange(duk.GetUIntValue(0, 1)));
      break;

    case 1: // DoubleVarianceAroundZero()
      duk.ReturnNumber(pWorld->GetRandomNumberGenerator().DoubleVarianceAroundZero(duk.GetIntValue(0, 0)));
      break;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }

  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, 1, +1);
}

static int __CPP_RNG_Get2(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);
  ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(duk);

  switch (duk.GetFunctionMagicValue())
  {
    case 0: // IntInRange()
      duk.ReturnInt(pWorld->GetRandomNumberGenerator().IntMinMax(duk.GetIntValue(0, 0), duk.GetIntValue(0, 0) + duk.GetUIntValue(1, 1)));
      break;

    case 1: // IntMinMax()
      duk.ReturnNumber(pWorld->GetRandomNumberGenerator().IntMinMax(duk.GetIntValue(0, 0), duk.GetIntValue(1, 0)));
      break;

    case 2: // DoubleInRange()
      duk.ReturnNumber(pWorld->GetRandomNumberGenerator().DoubleMinMax(duk.GetNumberValue(0, 0), duk.GetNumberValue(0, 0) + duk.GetNumberValue(1, 0)));
      break;

    case 3: // DoubleMinMax()
      duk.ReturnNumber(pWorld->GetRandomNumberGenerator().DoubleMinMax(duk.GetNumberValue(0, 0), duk.GetNumberValue(1, 0)));
      break;

    case 4: // DoubleVariance()
      duk.ReturnNumber(pWorld->GetRandomNumberGenerator().DoubleVariance(duk.GetNumberValue(0, 0), duk.GetNumberValue(1, 1)));
      break;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }

  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, 1, +1);
}
