#include <Foundation/PCH.h>
#include <Foundation/IO/ExtendedJSONReader.h>

// convert the binary string representation of some value to its memory
// useful for types where no text representation is available/useful
template<typename T>
ezVariant BuildTypedVariant_binary(const ezVariant& Binary, ezResult& out_Result)
{
  out_Result = EZ_FAILURE;

  T tValue = T();

  if (Binary.IsValid() && Binary.CanConvertTo<ezString>())
  {
    ezUInt8 Data[1024];
    ezConversionUtils::ConvertHexToBinary(Binary.ConvertTo<ezString>().GetData(), Data, EZ_ARRAY_SIZE(Data));

    tValue = *((T*) &Data[0]);

    out_Result = EZ_SUCCESS;
  }

  return ezVariant(tValue);
}

ezVariant BuildDataBuffer(const ezVariant& Binary, ezResult& out_Result)
{
  out_Result = EZ_FAILURE;

  ezDataBuffer data;

  if (Binary.IsValid() && Binary.IsA<ezString>())
  {
    const ezString& bin = Binary.Get<ezString>();
    data.SetCount((bin.GetElementCount() - 1) / 2);

    ezConversionUtils::ConvertHexToBinary(bin.GetData(), data.GetData(), data.GetCount());

    out_Result = EZ_SUCCESS;
  }

  return ezVariant(data);
}

// convert a number from either string or binary representation to its actual value
// choose the one that is 'more precise'
template<typename T>
T BuildTypedVariant_number(const ezVariant& Value, const ezVariant& Binary, T fEpsilon, ezResult& out_Result)
{
  T tValue = T();
  T bValue = T();
  bool bText = false;
  bool bBinary = false;

  if (Value.IsValid() && Value.CanConvertTo<T>())
  {
    ezResult Status(EZ_FAILURE);
    tValue = Value.ConvertTo<T>(&Status);

    bText = Status.Succeeded();
  }

  if (Binary.IsValid() && Binary.CanConvertTo<ezString>())
  {
    ezResult Status(EZ_FAILURE);

    ezUInt8 Data[1024];
    ezConversionUtils::ConvertHexToBinary(Binary.ConvertTo<ezString>(&Status).GetData(), Data, EZ_ARRAY_SIZE(Data));

    bValue = *((T*) &Data[0]);

    bBinary = Status.Succeeded();
  }

  out_Result = (bText || bBinary) ? EZ_SUCCESS : EZ_FAILURE;

  if (bText && bBinary)
  {
    // if they are really close, prefer the binary value
    if (ezMath::IsEqual(tValue, bValue, fEpsilon))
      return bValue;
  }

  if (bText)
  {
    return tValue;
  }

  return bValue;
}

ezAngle BuildTypedVariant_Angle(const ezVariant& Value, const ezVariant& Binary, float fEpsilon, ezResult& out_Result)
{
  float tValue = 0;
  ezAngle bValue = ezAngle();
  bool bText = false;
  bool bBinary = false;

  if (Value.IsValid() && Value.CanConvertTo<float>())
  {
    ezResult Status(EZ_FAILURE);
    tValue = Value.ConvertTo<float>(&Status);

    bText = Status.Succeeded();
  }

  if (Binary.IsValid() && Binary.CanConvertTo<ezString>())
  {
    ezResult Status(EZ_FAILURE);

    ezUInt8 Data[1024];
    ezConversionUtils::ConvertHexToBinary(Binary.ConvertTo<ezString>(&Status).GetData(), Data, EZ_ARRAY_SIZE(Data));

    bValue = *((ezAngle*)&Data[0]);

    bBinary = Status.Succeeded();
  }

  out_Result = (bText || bBinary) ? EZ_SUCCESS : EZ_FAILURE;

  if (bText && bBinary)
  {
    // if they are really close, prefer the binary value
    if (ezMath::IsEqual(tValue, bValue.GetDegree(), fEpsilon))
      return bValue;
  }

  if (bText)
  {
    return ezAngle::Degree(tValue);
  }

  return bValue;
}

template<typename TYPE, typename SUB_TYPE>
ezInt32 ExtractValues(const char* szString, ezInt32 iNumSubValues, TYPE& value)
{
  // TODO: Use double here and in ExtractFloatsFromString
  float temp[4];
  EZ_ASSERT_DEBUG(iNumSubValues <= 4, "ExtractValues can only be used for vectors that have less than 4 elements!");
  ezInt32 iRes = ezConversionUtils::ExtractFloatsFromString(szString, iNumSubValues, &temp[0]);

  SUB_TYPE* pSubValues = (SUB_TYPE*)&value;
  for (ezInt32 i = 0; i < iNumSubValues; i++)
  {
    pSubValues[i] = static_cast<SUB_TYPE>(temp[i]);
  }
  return iRes;
}

// same as BuildTypedVariant_number, but for vector types
template<typename T, typename SUB_TYPE>
T BuildTypedVariant_vector(const ezVariant& Value, const ezVariant& Binary, float fEpsilon, ezResult& out_Result)
{
  T tValue = T();
  T bValue = T();
  bool bText = false;
  bool bBinary = false;
  const ezInt32 iNumFloats = sizeof(T) / sizeof(SUB_TYPE);

  SUB_TYPE* ptValueFloats = (SUB_TYPE*) &tValue;
  SUB_TYPE* pbValueFloats = (SUB_TYPE*) &bValue;

  if (Value.IsValid() && Value.CanConvertTo<ezString>())
  {
    ezInt32 iRes = ExtractValues<T, SUB_TYPE>(Value.ConvertTo<ezString>(), iNumFloats, tValue);
    bText = (iRes == iNumFloats);
  }

  if (Binary.IsValid() && Binary.CanConvertTo<ezString>())
  {
    ezResult Status(EZ_FAILURE);

    ezUInt8 Data[1024];
    ezConversionUtils::ConvertHexToBinary(Binary.ConvertTo<ezString>(&Status).GetData(), Data, EZ_ARRAY_SIZE(Data));

    bValue = *((T*) &Data[0]);

    bBinary = Status.Succeeded();
  }

  if (bText && bBinary)
  {
    for (ezInt32 i = 0; i < iNumFloats; ++i)
    {
      // if they are really close, prefer the binary value
      // here we do this on a per-component level
      // copy it into the text representation
      if (ezMath::IsEqual((float)ptValueFloats[i], (float)pbValueFloats[i], fEpsilon))
        ptValueFloats[i] = pbValueFloats[i];
    }
  }

  out_Result = (bText || bBinary) ? EZ_SUCCESS : EZ_FAILURE;

  if (bText)
  {
    return tValue;
  }

  return bValue;
}

ezVariant BuildTypedVariant(const char* szType, const ezVariant& Value, const ezVariant& Binary, ezResult& out_Result)
{
  out_Result = EZ_FAILURE;

  if (ezStringUtils::IsEqual(szType, "int32"))
    return ezVariant(BuildTypedVariant_number<ezInt32>(Value, Binary, 0, out_Result));

  if (ezStringUtils::IsEqual(szType, "uint32"))
    return ezVariant(BuildTypedVariant_number<ezUInt32>(Value, Binary, 0, out_Result));

  if (ezStringUtils::IsEqual(szType, "int64"))
    return ezVariant(BuildTypedVariant_number<ezInt64>(Value, Binary, 0, out_Result));

  if (ezStringUtils::IsEqual(szType, "uint64"))
    return ezVariant(BuildTypedVariant_number<ezUInt64>(Value, Binary, 0, out_Result));

  if (ezStringUtils::IsEqual(szType, "float"))
    return ezVariant(BuildTypedVariant_number<float>(Value, Binary, 0.00009f, out_Result));

  if (ezStringUtils::IsEqual(szType, "double"))
    return ezVariant(BuildTypedVariant_number<double>(Value, Binary, 0.000000009f, out_Result));

  if (ezStringUtils::IsEqual(szType, "time"))
    return ezVariant(ezTime::Seconds(BuildTypedVariant_number<float>(Value, Binary, 0.00009f, out_Result)));

  if (ezStringUtils::IsEqual(szType, "color"))
    return ezVariant(BuildTypedVariant_vector<ezColor, float>(Value, Binary, 0.00009f, out_Result));

  if (ezStringUtils::IsEqual(szType, "gamma"))
    return ezVariant(BuildTypedVariant_vector<ezColorGammaUB, ezUInt8>(Value, Binary, 0, out_Result));

  if (ezStringUtils::IsEqual(szType, "vec2"))
    return ezVariant(BuildTypedVariant_vector<ezVec2, float>(Value, Binary, 0.00009f, out_Result));

  if (ezStringUtils::IsEqual(szType, "vec3"))
    return ezVariant(BuildTypedVariant_vector<ezVec3, float>(Value, Binary, 0.00009f, out_Result));

  if (ezStringUtils::IsEqual(szType, "vec4"))
    return ezVariant(BuildTypedVariant_vector<ezVec4, float>(Value, Binary, 0.00009f, out_Result));

  if (ezStringUtils::IsEqual(szType, "vec2i"))
    return ezVariant(BuildTypedVariant_vector<ezVec2I32, ezInt32>(Value, Binary, 0, out_Result));

  if (ezStringUtils::IsEqual(szType, "vec3i"))
    return ezVariant(BuildTypedVariant_vector<ezVec3I32, ezInt32>(Value, Binary, 0, out_Result));

  if (ezStringUtils::IsEqual(szType, "vec4i"))
    return ezVariant(BuildTypedVariant_vector<ezVec4I32, ezInt32>(Value, Binary, 0, out_Result));

  if (ezStringUtils::IsEqual(szType, "quat"))
    return BuildTypedVariant_binary<ezQuat>(Binary, out_Result);

  if (ezStringUtils::IsEqual(szType, "mat3"))
    return BuildTypedVariant_binary<ezMat3>(Binary, out_Result);

  if (ezStringUtils::IsEqual(szType, "mat4"))
    return BuildTypedVariant_binary<ezMat4>(Binary, out_Result);

  if (ezStringUtils::IsEqual(szType, "uuid"))
    return BuildTypedVariant_binary<ezUuid>(Binary, out_Result);

  if (ezStringUtils::IsEqual(szType, "angle"))
    return ezVariant(BuildTypedVariant_Angle(Value, Binary, 0.00009f, out_Result));

  if (ezStringUtils::IsEqual(szType, "data"))
    return BuildDataBuffer(Binary, out_Result);

  return ezVariant();
}

void ezExtendedJSONReader::OnEndObject()
{
  Element& Child = m_Stack[m_Stack.GetCount() - 1];

  ezVariant Type, Value, Binary;
  Child.m_Dictionary.TryGetValue("$v", Value);
  Child.m_Dictionary.TryGetValue("$b", Binary);

  // if the object contains a '$t' element, this is an 'extended' type
  // in this case, extract the type info from $t, the value from $v and/or $b and then actually
  // remove the object from the tree, but replace it by a single variable, with a precise type + value
  if (Child.m_Dictionary.TryGetValue("$t", Type))
  {
    ezResult result = EZ_FAILURE;
    ezVariant v = BuildTypedVariant(Type.ConvertTo<ezString>().GetData(), Value, Binary, result);

    if (result.Failed())
    {
      ezStringBuilder s;
      s.Printf("The extended type variable '%s' could not be converted. $t = '%s', $v = '%s', $b = '%s'", Child.m_sName.GetData(), Type.ConvertTo<ezString>().GetData(), Value.ConvertTo<ezString>().GetData(), Binary.ConvertTo<ezString>().GetData());
      ParsingError(s.GetData(), false);
    }

    if (m_Stack.GetCount() > 1)
    {
      Element& Parent = m_Stack[m_Stack.GetCount() - 2];

      if (Parent.m_Mode == ElementMode::Array)
      {
        Parent.m_Array.PushBack(v);
      }
      else
      {
        Parent.m_Dictionary[Child.m_sName] = v;
      }

      m_Stack.PopBack();
    }
    else
    {
      // this would be an invalid document

      ezStringBuilder s;
      s.Printf("Variable '%s' is an extended type, but at the root of the document hierarchy. This is invalid.", Child.m_sName.GetData());
      ParsingError(s.GetData(), true);
    }
  }
  else
  {
    if (m_Stack.GetCount() > 1)
    {
      Element& Parent = m_Stack[m_Stack.GetCount() - 2];

      if (Parent.m_Mode == ElementMode::Array)
      {
        Parent.m_Array.PushBack(Child.m_Dictionary);
      }
      else
      {
        Parent.m_Dictionary[Child.m_sName] = Child.m_Dictionary;
      }

      m_Stack.PopBack();
    }
    else
    {
      // do nothing, keep the top-level dictionary
    }
  }
}



EZ_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_ExtendedJSONReader);

