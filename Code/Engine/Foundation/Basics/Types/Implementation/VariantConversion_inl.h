
struct ezVariantConversion
{
  static void To(bool& result, const ezVariant& value)
  {
    if (value.GetType() <= ezVariant::Type::Double)
      result = value.ConvertNumber<ezInt32>() != 0;
    else if (value.GetType() == ezVariant::Type::String)
      result = value.Cast<ezString>() == "true";
  }

  /// \todo add more conversion function


  template <typename T>
  static void To(T& result, const ezVariant& value)
  {
    EZ_REPORT_FAILURE("Conversion function not implemented for target type '%d'", ezVariant::TypeDeduction<T>::value);
  }
};
