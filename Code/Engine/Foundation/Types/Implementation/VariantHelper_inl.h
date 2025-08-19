

// for some reason MSVC does not accept the template keyword here
#if EZ_ENABLED(EZ_COMPILER_MSVC_PURE)
#  define CALL_FUNCTOR(functor, type) return functor.operator()<type>(std::forward<Args>(args)...)
#else
#  define CALL_FUNCTOR(functor, type) return functor.template operator()<type>(std::forward<Args>(args)...)
#endif

template <typename Functor, class... Args>
auto ezVariant::DispatchTo(Functor& ref_functor, Type::Enum type, Args&&... args)
{
  switch (type)
  {
    case Type::Bool:
      CALL_FUNCTOR(ref_functor, bool);
      break;

    case Type::Int8:
      CALL_FUNCTOR(ref_functor, ezInt8);
      break;

    case Type::UInt8:
      CALL_FUNCTOR(ref_functor, ezUInt8);
      break;

    case Type::Int16:
      CALL_FUNCTOR(ref_functor, ezInt16);
      break;

    case Type::UInt16:
      CALL_FUNCTOR(ref_functor, ezUInt16);
      break;

    case Type::Int32:
      CALL_FUNCTOR(ref_functor, ezInt32);
      break;

    case Type::UInt32:
      CALL_FUNCTOR(ref_functor, ezUInt32);
      break;

    case Type::Int64:
      CALL_FUNCTOR(ref_functor, ezInt64);
      break;

    case Type::UInt64:
      CALL_FUNCTOR(ref_functor, ezUInt64);
      break;

    case Type::Float:
      CALL_FUNCTOR(ref_functor, float);
      break;

    case Type::Double:
      CALL_FUNCTOR(ref_functor, double);
      break;

    case Type::Color:
      CALL_FUNCTOR(ref_functor, ezColor);
      break;

    case Type::ColorGamma:
      CALL_FUNCTOR(ref_functor, ezColorGammaUB);
      break;

    case Type::Vector2:
      CALL_FUNCTOR(ref_functor, ezVec2);
      break;

    case Type::Vector3:
      CALL_FUNCTOR(ref_functor, ezVec3);
      break;

    case Type::Vector4:
      CALL_FUNCTOR(ref_functor, ezVec4);
      break;

    case Type::Vector2I:
      CALL_FUNCTOR(ref_functor, ezVec2I32);
      break;

    case Type::Vector3I:
      CALL_FUNCTOR(ref_functor, ezVec3I32);
      break;

    case Type::Vector4I:
      CALL_FUNCTOR(ref_functor, ezVec4I32);
      break;

    case Type::Vector2U:
      CALL_FUNCTOR(ref_functor, ezVec2U32);
      break;

    case Type::Vector3U:
      CALL_FUNCTOR(ref_functor, ezVec3U32);
      break;

    case Type::Vector4U:
      CALL_FUNCTOR(ref_functor, ezVec4U32);
      break;

    case Type::Quaternion:
      CALL_FUNCTOR(ref_functor, ezQuat);
      break;

    case Type::Matrix3:
      CALL_FUNCTOR(ref_functor, ezMat3);
      break;

    case Type::Matrix4:
      CALL_FUNCTOR(ref_functor, ezMat4);
      break;

    case Type::Transform:
      CALL_FUNCTOR(ref_functor, ezTransform);
      break;

    case Type::String:
      CALL_FUNCTOR(ref_functor, ezString);
      break;

    case Type::StringView:
      CALL_FUNCTOR(ref_functor, ezStringView);
      break;

    case Type::DataBuffer:
      CALL_FUNCTOR(ref_functor, ezDataBuffer);
      break;

    case Type::Time:
      CALL_FUNCTOR(ref_functor, ezTime);
      break;

    case Type::Uuid:
      CALL_FUNCTOR(ref_functor, ezUuid);
      break;

    case Type::Angle:
      CALL_FUNCTOR(ref_functor, ezAngle);
      break;

    case Type::HashedString:
      CALL_FUNCTOR(ref_functor, ezHashedString);
      break;

    case Type::TempHashedString:
      CALL_FUNCTOR(ref_functor, ezTempHashedString);
      break;

    case Type::VariantArray:
      CALL_FUNCTOR(ref_functor, ezVariantArray);
      break;

    case Type::VariantDictionary:
      CALL_FUNCTOR(ref_functor, ezVariantDictionary);
      break;

    case Type::TypedObject:
      CALL_FUNCTOR(ref_functor, ezTypedObject);
      break;

    default:
      EZ_REPORT_FAILURE("Could not dispatch type '{0}'", type);
      // Intended fall through to disable warning.
    case Type::TypedPointer:
      CALL_FUNCTOR(ref_functor, ezTypedPointer);
      break;
  }
}

#undef CALL_FUNCTOR

class ezVariantHelper
{
  friend class ezVariant;
  friend struct ConvertFunc;

  static void To(const ezVariant& value, bool& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.GetType() <= ezVariant::Type::Double)
      result = value.ConvertNumber<ezInt32>() != 0;
    else if (value.GetType() == ezVariant::Type::String || value.GetType() == ezVariant::Type::HashedString)
    {
      ezStringView s = value.IsA<ezString>() ? value.Cast<ezString>().GetView() : value.Cast<ezHashedString>().GetView();
      if (ezConversionUtils::StringToBool(s, result) == EZ_FAILURE)
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
    ezInt32 tempResult = 0;
    To(value, tempResult, bSuccessful);
    result = (ezInt8)tempResult;
  }

  static void To(const ezVariant& value, ezUInt8& result, bool& bSuccessful)
  {
    ezUInt32 tempResult = 0;
    To(value, tempResult, bSuccessful);
    result = (ezUInt8)tempResult;
  }

  static void To(const ezVariant& value, ezInt16& result, bool& bSuccessful)
  {
    ezInt32 tempResult = 0;
    To(value, tempResult, bSuccessful);
    result = (ezInt16)tempResult;
  }

  static void To(const ezVariant& value, ezUInt16& result, bool& bSuccessful)
  {
    ezUInt32 tempResult = 0;
    To(value, tempResult, bSuccessful);
    result = (ezUInt16)tempResult;
  }

  static void To(const ezVariant& value, ezInt32& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.GetType() <= ezVariant::Type::Double)
      result = value.ConvertNumber<ezInt32>();
    else if (value.GetType() == ezVariant::Type::String || value.GetType() == ezVariant::Type::HashedString)
    {
      ezStringView s = value.IsA<ezString>() ? value.Cast<ezString>().GetView() : value.Cast<ezHashedString>().GetView();
      if (ezConversionUtils::StringToInt(s, result) == EZ_FAILURE)
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
    bSuccessful = true;

    if (value.GetType() <= ezVariant::Type::Double)
      result = value.ConvertNumber<ezUInt32>();
    else if (value.GetType() == ezVariant::Type::String || value.GetType() == ezVariant::Type::HashedString)
    {
      ezStringView s = value.IsA<ezString>() ? value.Cast<ezString>().GetView() : value.Cast<ezHashedString>().GetView();
      ezInt64 tmp = result;
      if (ezConversionUtils::StringToInt64(s, tmp) == EZ_FAILURE)
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
    bSuccessful = true;

    if (value.GetType() <= ezVariant::Type::Double)
      result = value.ConvertNumber<ezInt64>();
    else if (value.GetType() == ezVariant::Type::String || value.GetType() == ezVariant::Type::HashedString)
    {
      ezStringView s = value.IsA<ezString>() ? value.Cast<ezString>().GetView() : value.Cast<ezHashedString>().GetView();
      if (ezConversionUtils::StringToInt64(s, result) == EZ_FAILURE)
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
    bSuccessful = true;

    if (value.GetType() <= ezVariant::Type::Double)
      result = value.ConvertNumber<ezUInt64>();
    else if (value.GetType() == ezVariant::Type::String || value.GetType() == ezVariant::Type::HashedString)
    {
      ezStringView s = value.IsA<ezString>() ? value.Cast<ezString>().GetView() : value.Cast<ezHashedString>().GetView();
      ezInt64 tmp = result;
      if (ezConversionUtils::StringToInt64(s, tmp) == EZ_FAILURE)
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
    bSuccessful = true;

    if (value.GetType() <= ezVariant::Type::Double)
      result = value.ConvertNumber<float>();
    else if (value.GetType() == ezVariant::Type::String || value.GetType() == ezVariant::Type::HashedString)
    {
      ezStringView s = value.IsA<ezString>() ? value.Cast<ezString>().GetView() : value.Cast<ezHashedString>().GetView();
      double tmp = result;
      if (ezConversionUtils::StringToFloat(s, tmp) == EZ_FAILURE)
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
    bSuccessful = true;

    if (value.GetType() <= ezVariant::Type::Double)
      result = value.ConvertNumber<double>();
    else if (value.GetType() == ezVariant::Type::String || value.GetType() == ezVariant::Type::HashedString)
    {
      ezStringView s = value.IsA<ezString>() ? value.Cast<ezString>().GetView() : value.Cast<ezHashedString>().GetView();
      if (ezConversionUtils::StringToFloat(s, result) == EZ_FAILURE)
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
    bSuccessful = true;

    if (value.IsValid() == false)
    {
      result = "<Invalid>";
      return;
    }

    ToStringFunc toStringFunc;
    toStringFunc.m_pThis = &value;
    toStringFunc.m_pResult = &result;

    ezVariant::DispatchTo(toStringFunc, value.GetType());
  }

  static void To(const ezVariant& value, ezStringView& result, bool& bSuccessful)
  {
    bSuccessful = true;

    result = value.IsA<ezString>() ? value.Get<ezString>().GetView() : value.Get<ezHashedString>().GetView();
  }

  static void To(const ezVariant& value, ezTypedPointer& result, bool& bSuccessful)
  {
    bSuccessful = true;
    EZ_ASSERT_DEBUG(value.GetType() == ezVariant::Type::TypedPointer, "Only ptr can be converted to void*!");
    result = value.Cast<ezTypedPointer>();
  }

  static void To(const ezVariant& value, ezColor& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.GetType() == ezVariant::Type::ColorGamma)
      result = value.Cast<ezColorGammaUB>();
    else
      EZ_REPORT_FAILURE("Conversion to ezColor failed");
  }

  static void To(const ezVariant& value, ezColorGammaUB& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.GetType() == ezVariant::Type::Color)
      result = value.Cast<ezColor>();
    else
      EZ_REPORT_FAILURE("Conversion to ezColorGammaUB failed");
  }

  template <typename T, typename V1, typename V2>
  static void ToVec2X(const ezVariant& value, T& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.IsNumber())
    {
      auto x = value.ConvertNumber<typename T::ComponentType>();
      result = T(x);
    }
    else if (value.IsA<V1>())
    {
      const V1& v = value.Cast<V1>();
      result = T(static_cast<typename T::ComponentType>(v.x), static_cast<typename T::ComponentType>(v.y));
    }
    else if (value.IsA<V2>())
    {
      const V2& v = value.Cast<V2>();
      result = T(static_cast<typename T::ComponentType>(v.x), static_cast<typename T::ComponentType>(v.y));
    }
    else
    {
      EZ_REPORT_FAILURE("Conversion to ezVec2X failed");
      bSuccessful = false;
    }
  }

  static void To(const ezVariant& value, ezVec2& result, bool& bSuccessful) { ToVec2X<ezVec2, ezVec2I32, ezVec2U32>(value, result, bSuccessful); }

  static void To(const ezVariant& value, ezVec2I32& result, bool& bSuccessful) { ToVec2X<ezVec2I32, ezVec2, ezVec2U32>(value, result, bSuccessful); }

  static void To(const ezVariant& value, ezVec2U32& result, bool& bSuccessful) { ToVec2X<ezVec2U32, ezVec2I32, ezVec2>(value, result, bSuccessful); }

  template <typename T, typename V1, typename V2>
  static void ToVec3X(const ezVariant& value, T& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.IsNumber())
    {
      auto x = value.ConvertNumber<typename T::ComponentType>();
      result = T(x);
    }
    else if (value.IsA<V1>())
    {
      const V1& v = value.Cast<V1>();
      result = T(static_cast<typename T::ComponentType>(v.x), static_cast<typename T::ComponentType>(v.y), static_cast<typename T::ComponentType>(v.z));
    }
    else if (value.IsA<V2>())
    {
      const V2& v = value.Cast<V2>();
      result = T(static_cast<typename T::ComponentType>(v.x), static_cast<typename T::ComponentType>(v.y), static_cast<typename T::ComponentType>(v.z));
    }
    else
    {
      EZ_REPORT_FAILURE("Conversion to ezVec3X failed");
      bSuccessful = false;
    }
  }

  static void To(const ezVariant& value, ezVec3& result, bool& bSuccessful) { ToVec3X<ezVec3, ezVec3I32, ezVec3U32>(value, result, bSuccessful); }

  static void To(const ezVariant& value, ezVec3I32& result, bool& bSuccessful) { ToVec3X<ezVec3I32, ezVec3, ezVec3U32>(value, result, bSuccessful); }

  static void To(const ezVariant& value, ezVec3U32& result, bool& bSuccessful) { ToVec3X<ezVec3U32, ezVec3I32, ezVec3>(value, result, bSuccessful); }

  template <typename T, typename V1, typename V2>
  static void ToVec4X(const ezVariant& value, T& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.IsNumber())
    {
      auto x = value.ConvertNumber<typename T::ComponentType>();
      result = T(x);
    }
    else if (value.IsA<V1>())
    {
      const V1& v = value.Cast<V1>();
      result = T(static_cast<typename T::ComponentType>(v.x), static_cast<typename T::ComponentType>(v.y), static_cast<typename T::ComponentType>(v.z), static_cast<typename T::ComponentType>(v.w));
    }
    else if (value.IsA<V2>())
    {
      const V2& v = value.Cast<V2>();
      result = T(static_cast<typename T::ComponentType>(v.x), static_cast<typename T::ComponentType>(v.y), static_cast<typename T::ComponentType>(v.z), static_cast<typename T::ComponentType>(v.w));
    }
    else
    {
      EZ_REPORT_FAILURE("Conversion to ezVec4X failed");
      bSuccessful = false;
    }
  }

  static void To(const ezVariant& value, ezVec4& result, bool& bSuccessful) { ToVec4X<ezVec4, ezVec4I32, ezVec4U32>(value, result, bSuccessful); }

  static void To(const ezVariant& value, ezVec4I32& result, bool& bSuccessful) { ToVec4X<ezVec4I32, ezVec4, ezVec4U32>(value, result, bSuccessful); }

  static void To(const ezVariant& value, ezVec4U32& result, bool& bSuccessful) { ToVec4X<ezVec4U32, ezVec4I32, ezVec4>(value, result, bSuccessful); }

  static void To(const ezVariant& value, ezHashedString& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.GetType() == ezVariantType::String)
      result.Assign(value.Cast<ezString>());
    else if (value.GetType() == ezVariantType::StringView)
      result.Assign(value.Cast<ezStringView>());
    else
    {
      ezString s;
      To(value, s, bSuccessful);
      result.Assign(s.GetView());
    }
  }

  static void To(const ezVariant& value, ezTempHashedString& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.GetType() == ezVariantType::String)
      result = value.Cast<ezString>();
    else if (value.GetType() == ezVariantType::StringView)
      result = value.Cast<ezStringView>();
    else if (value.GetType() == ezVariant::Type::HashedString)
      result = value.Cast<ezHashedString>();
    else
    {
      ezString s;
      To(value, s, bSuccessful);
      result = s.GetView();
    }
  }

  template <typename T>
  static void To(const ezVariant& value, T& result, bool& bSuccessful)
  {
    EZ_IGNORE_UNUSED(value);
    EZ_IGNORE_UNUSED(result);
    EZ_REPORT_FAILURE("Conversion function not implemented for target type '{0}'", ezVariant::TypeDeduction<T>::value);
    bSuccessful = false;
  }

  struct ToStringFunc
  {
    template <typename T>
    EZ_ALWAYS_INLINE void operator()()
    {
      ezStringBuilder tmp;
      *m_pResult = ezConversionUtils::ToString(m_pThis->Cast<T>(), tmp); // NOLINT (clang-analyzer-core.CallAndMessage)
    }

    const ezVariant* m_pThis;
    ezString* m_pResult;
  };
};
