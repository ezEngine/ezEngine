
class ezVariantHelper
{
  friend class ezVariant;
  friend struct ConvertFunc;

  template <typename T>
  EZ_ALWAYS_INLINE static bool CompareFloat(const ezVariant& v, const T& other, ezTraitInt<1>)
  {
    return v.ConvertNumber<double>() == static_cast<double>(other);
  }

  template <typename T>
  EZ_ALWAYS_INLINE static bool CompareFloat(const ezVariant& v, const T& other, ezTraitInt<0>)
  {
    return false;
  }

  template <typename T>
  EZ_ALWAYS_INLINE static bool CompareNumber(const ezVariant& v, const T& other, ezTraitInt<1>)
  {
    return v.ConvertNumber<ezInt64>() == static_cast<ezInt64>(other);
  }

  template <typename T>
  EZ_ALWAYS_INLINE static bool CompareNumber(const ezVariant& v, const T& other, ezTraitInt<0>)
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
    To(value, tempResult, bSuccessful);
    result = (ezInt8)tempResult;
  }

  static void To(const ezVariant& value, ezUInt8& result, bool& bSuccessful)
  {
    ezUInt32 tempResult;
    To(value, tempResult, bSuccessful);
    result = (ezUInt8)tempResult;
  }

  static void To(const ezVariant& value, ezInt16& result, bool& bSuccessful)
  {
    ezInt32 tempResult;
    To(value, tempResult, bSuccessful);
    result = (ezInt16)tempResult;
  }

  static void To(const ezVariant& value, ezUInt16& result, bool& bSuccessful)
  {
    ezUInt32 tempResult;
    To(value, tempResult, bSuccessful);
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

  static void To(const ezVariant& value, void*& result, bool& bSuccessful)
  {
    EZ_ASSERT_DEBUG(value.GetType() == ezVariant::Type::VoidPointer || value.GetType() == ezVariant::Type::ReflectedPointer,
      "Only ptr can be converted to void*!");
    result = value.GetType() == ezVariant::Type::VoidPointer ? value.Get<void*>() : value.Get<ezReflectedClass*>();
    bSuccessful = true;
  }

  static void To(const ezVariant& value, ezColor& result, bool& bSuccessful)
  {
    if (value.GetType() == ezVariant::Type::ColorGamma)
      result = value.Get<ezColorGammaUB>();
    else
      EZ_REPORT_FAILURE("Conversion to ezColor failed");
  }

  static void To(const ezVariant& value, ezColorGammaUB& result, bool& bSuccessful)
  {
    if (value.GetType() == ezVariant::Type::Color)
      result = value.Get<ezColor>();
    else
      EZ_REPORT_FAILURE("Conversion to ezColorGammaUB failed");
  }

  template <typename T, typename V1, typename V2>
  static void ToVec2X(const ezVariant& value, T& result, bool& bSuccessful)
  {
    if (value.IsA<V1>())
    {
      const V1& v = value.Get<V1>();
      result = T(v.x, v.y);
    }
    else if (value.IsA<V2>())
    {
      const V2& v = value.Get<V2>();
      result = T(v.x, v.y);
    }
    else
    {
      EZ_REPORT_FAILURE("Conversion to ezVec2X failed");
      bSuccessful = false;
    }
  }

  static void To(const ezVariant& value, ezVec2& result, bool& bSuccessful)
  {
    ToVec2X<ezVec2, ezVec2I32, ezVec2U32>(value, result, bSuccessful);
  }

  static void To(const ezVariant& value, ezVec2I32& result, bool& bSuccessful)
  {
    ToVec2X<ezVec2I32, ezVec2, ezVec2U32>(value, result, bSuccessful);
  }

  static void To(const ezVariant& value, ezVec2U32& result, bool& bSuccessful)
  {
    ToVec2X<ezVec2U32, ezVec2I32, ezVec2>(value, result, bSuccessful);
  }

  template <typename T, typename V1, typename V2>
  static void ToVec3X(const ezVariant& value, T& result, bool& bSuccessful)
  {
    if (value.IsA<V1>())
    {
      const V1& v = value.Get<V1>();
      result = T(v.x, v.y, v.z);
    }
    else if (value.IsA<V2>())
    {
      const V2& v = value.Get<V2>();
      result = T(v.x, v.y, v.z);
    }
    else
    {
      EZ_REPORT_FAILURE("Conversion to ezVec3X failed");
      bSuccessful = false;
    }
  }

  static void To(const ezVariant& value, ezVec3& result, bool& bSuccessful)
  {
    ToVec3X<ezVec3, ezVec3I32, ezVec3U32>(value, result, bSuccessful);
  }

  static void To(const ezVariant& value, ezVec3I32& result, bool& bSuccessful)
  {
    ToVec3X<ezVec3I32, ezVec3, ezVec3U32>(value, result, bSuccessful);
  }

  static void To(const ezVariant& value, ezVec3U32& result, bool& bSuccessful)
  {
    ToVec3X<ezVec3U32, ezVec3I32, ezVec3>(value, result, bSuccessful);
  }

  template <typename T, typename V1, typename V2>
  static void ToVec4X(const ezVariant& value, T& result, bool& bSuccessful)
  {
    if (value.IsA<V1>())
    {
      const V1& v = value.Get<V1>();
      result = T(v.x, v.y, v.z, v.w);
    }
    else if (value.IsA<V2>())
    {
      const V2& v = value.Get<V2>();
      result = T(v.x, v.y, v.z, v.w);
    }
    else
    {
      EZ_REPORT_FAILURE("Conversion to ezVec4X failed");
      bSuccessful = false;
    }
  }

  static void To(const ezVariant& value, ezVec4& result, bool& bSuccessful)
  {
    ToVec4X<ezVec4, ezVec4I32, ezVec4U32>(value, result, bSuccessful);
  }

  static void To(const ezVariant& value, ezVec4I32& result, bool& bSuccessful)
  {
    ToVec4X<ezVec4I32, ezVec4, ezVec4U32>(value, result, bSuccessful);
  }

  static void To(const ezVariant& value, ezVec4U32& result, bool& bSuccessful)
  {
    ToVec4X<ezVec4U32, ezVec4I32, ezVec4>(value, result, bSuccessful);
  }

  template <typename T>
  static void To(const ezVariant& value, T& result, bool& bSuccessful)
  {
    EZ_REPORT_FAILURE("Conversion function not implemented for target type '{0}'", ezVariant::TypeDeduction<T>::value);
    bSuccessful = false;
  }

  struct ToStringFunc
  {
    template <typename T>
    EZ_ALWAYS_INLINE void operator()()
    {
      ezStringBuilder tmp;
      *m_pResult = ezConversionUtils::ToString(m_pThis->Cast<T>(), tmp);
    }

    const ezVariant* m_pThis;
    ezString* m_pResult;
  };
};
