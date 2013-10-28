
class ezVariantHelper
{
public:
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

  static void To(bool& result, const ezVariant& value)
  {
    if (value.GetType() <= ezVariant::Type::Double)
      result = value.ConvertNumber<ezInt32>() != 0;
    else if (value.GetType() == ezVariant::Type::String)
      ezConversionUtils::StringToBool(value.Cast<ezString>().GetData(), result);
    else
      EZ_REPORT_FAILURE("Conversion to bool failed");
  }

  static void To(ezInt32& result, const ezVariant& value)
  {
    if (value.GetType() <= ezVariant::Type::Double)
      result = value.ConvertNumber<ezInt32>() != 0;
    else if (value.GetType() == ezVariant::Type::String)
      ezConversionUtils::StringToInt(value.Cast<ezString>().GetData(), result);
    else
      EZ_REPORT_FAILURE("Conversion to int failed");
  }

  /// \todo add more conversion function

  static void To(float& result, const ezVariant& value)
  {
    if (value.GetType() <= ezVariant::Type::Double)
      result = value.ConvertNumber<float>() != 0;
    else if (value.GetType() == ezVariant::Type::String)
    {
      double tmp = result;
      ezConversionUtils::StringToFloat(value.Cast<ezString>().GetData(), tmp);
      result = static_cast<float>(tmp);
    }
    else
      EZ_REPORT_FAILURE("Conversion to float failed");
  }

  static void To(double& result, const ezVariant& value)
  {
    if (value.GetType() <= ezVariant::Type::Double)
      result = value.ConvertNumber<double>() != 0;
    else if (value.GetType() == ezVariant::Type::String)
      ezConversionUtils::StringToFloat(value.Cast<ezString>().GetData(), result);
    else
      EZ_REPORT_FAILURE("Conversion to double failed");
  }

  static void To(ezString& result, const ezVariant& value)
  {
    /*ToStringFunc toStringFunc;
    toStringFunc.m_pThis = &value;
    toStringFunc.m_pResult = &result;

    ezVariant::DispatchTo(toStringFunc, value.GetType());*/
  }

  template <typename T>
  static void To(T& result, const ezVariant& value)
  {
    EZ_REPORT_FAILURE("Conversion function not implemented for target type '%d'", ezVariant::TypeDeduction<T>::value);
  }

private:
  struct ToStringFunc
  {
    template <typename T>
    EZ_FORCE_INLINE void operator()()
    {
      ezConversionUtils::ToString(m_pThis->Cast<T>());
    }

    const ezVariant* m_pThis;
    ezString* m_pResult;
  };
};
