
class ezVariantHelper
{
  friend class ezVariant;
  friend struct ConvertFunc;

  template <typename T>
  EZ_FORCE_INLINE static bool CompareFloat(const ezVariant& v, const T& other, ezTraitInt<1>)
  {
    return v.ConvertNumber<double>() == static_cast<double>(other);
  }
  
  template <typename T>
  EZ_FORCE_INLINE static bool CompareFloat(const ezVariant& v, const T& other, ezTraitInt<0>)
  {
    return false;
  }

  template <typename T>
  EZ_FORCE_INLINE static bool CompareNumber(const ezVariant& v, const T& other, ezTraitInt<1>)
  {
    return v.ConvertNumber<ezInt64>() == static_cast<ezInt64>(other);
  }
  
  template <typename T>
  EZ_FORCE_INLINE static bool CompareNumber(const ezVariant& v, const T& other, ezTraitInt<0>)
  {
    return false;
  }

  static void To(const ezVariant& value, bool& result, bool& bSuccessful)
  {
    if (value.GetType() <= ezVariant::Type::Double)
      result = value.ConvertNumber<ezInt32>() != 0;
    else if (value.GetType() == ezVariant::Type::String)
    {
      if (ezConversionUtils::StringToBool(value.Cast<ezString>().GetData(), result) == EZ_FAILURE)
      {
        result = false;
        bSuccessful = false;
      }
    }
    else
      EZ_REPORT_FAILURE("Conversion to bool failed");
  }

  static void To(const ezVariant& value, ezInt8& result, bool& bSuccessful)
  {
    ezInt32 tempResult;
    To (value, tempResult, bSuccessful);
    result = (ezInt8)tempResult;
  }

  static void To(const ezVariant& value, ezUInt8& result, bool& bSuccessful)
  {
    ezUInt32 tempResult;
    To (value, tempResult, bSuccessful);
    result = (ezUInt8)tempResult;
  }

  static void To(const ezVariant& value, ezInt16& result, bool& bSuccessful)
  {
    ezInt32 tempResult;
    To (value, tempResult, bSuccessful);
    result = (ezInt16)tempResult;
  }

  static void To(const ezVariant& value, ezUInt16& result, bool& bSuccessful)
  {
    ezUInt32 tempResult;
    To (value, tempResult, bSuccessful);
    result = (ezUInt16)tempResult;
  }

  static void To(const ezVariant& value, ezInt32& result, bool& bSuccessful)
  {
    if (value.GetType() <= ezVariant::Type::Double)
      result = value.ConvertNumber<ezInt32>();
    else if (value.GetType() == ezVariant::Type::String)
    {
      if (ezConversionUtils::StringToInt(value.Cast<ezString>().GetData(), result) == EZ_FAILURE)
      {
        result = 0;
        bSuccessful = false;
      }
    }
    else
      EZ_REPORT_FAILURE("Conversion to int failed");
  }

  static void To(const ezVariant& value, ezUInt32& result, bool& bSuccessful)
  {
    if (value.GetType() <= ezVariant::Type::Double)
      result = value.ConvertNumber<ezUInt32>();
    else if (value.GetType() == ezVariant::Type::String)
    {
      ezInt64 tmp = result;
      if (ezConversionUtils::StringToInt64(value.Cast<ezString>().GetData(), tmp) == EZ_FAILURE)
      {
        result = 0;
        bSuccessful = false;
      }
      else
        result = (ezUInt32)tmp;
    }
    else
      EZ_REPORT_FAILURE("Conversion to uint failed");
  }

  static void To(const ezVariant& value, ezInt64& result, bool& bSuccessful)
  {
    if (value.GetType() <= ezVariant::Type::Double)
      result = value.ConvertNumber<ezInt64>();
    else if (value.GetType() == ezVariant::Type::String)
    {
      if (ezConversionUtils::StringToInt64(value.Cast<ezString>().GetData(), result) == EZ_FAILURE)
      {
        result = 0;
        bSuccessful = false;
      }
    }
    else
      EZ_REPORT_FAILURE("Conversion to int64 failed");
  }

  static void To(const ezVariant& value, ezUInt64& result, bool& bSuccessful)
  {
    if (value.GetType() <= ezVariant::Type::Double)
      result = value.ConvertNumber<ezUInt64>();
    else if (value.GetType() == ezVariant::Type::String)
    {
      ezInt64 tmp = result;
      if (ezConversionUtils::StringToInt64(value.Cast<ezString>().GetData(), tmp) == EZ_FAILURE)
      {
        result = 0;
        bSuccessful = false;
      }
      else
        result = (ezUInt64)tmp;
    }
    else
      EZ_REPORT_FAILURE("Conversion to uint64 failed");
  }

  static void To(const ezVariant& value, float& result, bool& bSuccessful)
  {
    if (value.GetType() <= ezVariant::Type::Double)
      result = value.ConvertNumber<float>();
    else if (value.GetType() == ezVariant::Type::String)
    {
      double tmp = result;
      if (ezConversionUtils::StringToFloat(value.Cast<ezString>().GetData(), tmp) == EZ_FAILURE)
      {
        result = 0.0f;
        bSuccessful = false;
      }
      else
        result = (float)tmp;
    }
    else
      EZ_REPORT_FAILURE("Conversion to float failed");
  }

  static void To(const ezVariant& value, double& result, bool& bSuccessful)
  {
    if (value.GetType() <= ezVariant::Type::Double)
      result = value.ConvertNumber<double>();
    else if (value.GetType() == ezVariant::Type::String)
    {
      if (ezConversionUtils::StringToFloat(value.Cast<ezString>().GetData(), result) == EZ_FAILURE)
      {
        result = 0.0;
        bSuccessful = false;
      }
    }
    else
      EZ_REPORT_FAILURE("Conversion to double failed");
  }

  static void To(const ezVariant& value, ezString& result, bool& bSuccessful)
  {
    ToStringFunc toStringFunc;
    toStringFunc.m_pThis = &value;
    toStringFunc.m_pResult = &result;

    ezVariant::DispatchTo(toStringFunc, value.GetType());
    bSuccessful = true;
  }

  template <typename T>
  static void To(const ezVariant& value, T& result, bool& bSuccessful)
  {
    EZ_REPORT_FAILURE("Conversion function not implemented for target type '%d'", ezVariant::TypeDeduction<T>::value);
    bSuccessful = false;
  }

  struct ToStringFunc
  {
    template <typename T>
    EZ_FORCE_INLINE void operator()()
    {
      *m_pResult = ezConversionUtils::ToString(m_pThis->Cast<T>());
    }

    const ezVariant* m_pThis;
    ezString* m_pResult;
  };
};

