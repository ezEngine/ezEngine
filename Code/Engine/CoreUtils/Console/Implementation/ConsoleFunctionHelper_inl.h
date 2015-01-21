// NO #pragma once in this file !

template <typename R EZ_COMMA_IF(ARG_COUNT) EZ_LIST(typename P, ARG_COUNT)>
class ezConsoleFunction<R (EZ_LIST(P, ARG_COUNT))> : public ezConsoleFunctionBase
{
public:
  typedef ezDelegate<R (EZ_LIST(P, ARG_COUNT))> FUNC;

  FUNC m_Func;

  ezConsoleFunction(const char* szFunctionName, const char* szDescription, FUNC f) : ezConsoleFunctionBase(szFunctionName, szDescription)
  {
    m_Func = f;
  }

  ezUInt32 GetNumParameters() const override { return ARG_COUNT; }

  virtual ezVariant::Type::Enum GetParameterType(ezUInt32 uiParam) const override
  {
    EZ_ASSERT_DEV(uiParam < GetNumParameters(), "Invalid Parameter Index %d", uiParam);

#if (ARG_COUNT > 0)

    switch (uiParam)
    {
    case 0:
      return static_cast<ezVariant::Type::Enum>(ezVariant::TypeDeduction<P0>::value);

#if (ARG_COUNT > 1)
    case 1:
      return static_cast<ezVariant::Type::Enum>(ezVariant::TypeDeduction<P1>::value);
#endif
#if (ARG_COUNT > 2)
    case 2:
      return static_cast<ezVariant::Type::Enum>(ezVariant::TypeDeduction<P2>::value);
#endif
#if (ARG_COUNT > 3)
    case 3:
      return static_cast<ezVariant::Type::Enum>(ezVariant::TypeDeduction<P3>::value);
#endif
#if (ARG_COUNT > 4)
    case 4:
      return static_cast<ezVariant::Type::Enum>(ezVariant::TypeDeduction<P4>::value);
#endif
#if (ARG_COUNT > 5)
    case 5:
      return static_cast<ezVariant::Type::Enum>(ezVariant::TypeDeduction<P5>::value);
#endif
    }

#endif
    return ezVariant::Type::Invalid;
  }

  virtual ezResult Call(ezArrayPtr<ezVariant> params) override
  {
    ezResult r = EZ_FAILURE;

#if (ARG_COUNT > 0)
    P0 param0 = params[0].ConvertTo<P0>(&r);

    if (r.Failed())
      return EZ_FAILURE;
#endif

#if (ARG_COUNT > 1)
    P1 param1 = params[1].ConvertTo<P1>(&r);

    if (r.Failed())
      return EZ_FAILURE;
#endif

#if (ARG_COUNT > 2)
    P2 param2 = params[2].ConvertTo<P2>(&r);

    if (r.Failed())
      return EZ_FAILURE;
#endif

#if (ARG_COUNT > 3)
    P3 param3 = params[3].ConvertTo<P3>(&r);

    if (r.Failed())
      return EZ_FAILURE;
#endif

#if (ARG_COUNT > 4)
    P4 param4 = params[4].ConvertTo<P4>(&r);

    if (r.Failed())
      return EZ_FAILURE;
#endif

#if (ARG_COUNT > 5)
    P5 param5 = params[5].ConvertTo<P5>(&r);

    if (r.Failed())
      return EZ_FAILURE;
#endif

    m_Func(EZ_LIST(param, ARG_COUNT));
    return EZ_SUCCESS;
  }
};



