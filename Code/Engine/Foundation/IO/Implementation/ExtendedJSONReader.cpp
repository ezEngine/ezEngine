#include <Foundation/PCH.h>
#include <Foundation/IO/ExtendedJSONReader.h>

static void ParseFloats(const char* szText, float* pFloats, ezInt32 iNumFloats)
{
  // just try to extract n floats from the given text
  // if n floats were extracted, or the text end is reached, stop

  while (*szText != '\0' && iNumFloats > 0)
  {
    double res;
    const char* szPos;

    // if successful, store the float, otherwise advance the string by one, to skip invalid characters
    if (ezConversionUtils::StringToFloat(szText, res, &szPos) == EZ_SUCCESS)
    {
      *pFloats = (float) res;
      ++pFloats;
      --iNumFloats;

      szText = szPos;
    }
    else
      ++szText;
  }
}

// converts hex characters to their integer value
static ezUInt8 StringHexToInt(char c)
{
  if (c >= '0' && c <= '9')
    return c - '0';

  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;

  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;

  return 0;
}

// converts a string of hex values to a binary data blob
static void ConvertHexToBinary(const char* szHEX, ezUInt8* pBinary, ezUInt32 uiBinaryBuffer)
{
  // skip 0x
  if (szHEX[0] == '0' && szHEX[1] == 'x')
    szHEX += 2;

  // convert two characters to one byte, at a time
  // try not to run out of buffer space
  while (*szHEX != '\0' && uiBinaryBuffer >= 1)
  {
    ezUInt8 uiValue1 = StringHexToInt(szHEX[0]);
    ezUInt8 uiValue2 = StringHexToInt(szHEX[1]);
    ezUInt8 uiValue = 16 * uiValue1 + uiValue2;
    *pBinary = uiValue;

    pBinary += 1;
    szHEX += 2;

    uiBinaryBuffer -= 1;
  }
}

// convert the binary string representation of some value to its memory
// useful for types were no text representation is available/useful
template<typename T>
ezVariant BuildTypedVariant_binary(const ezVariant& Binary)
{
  T tValue = T();

  if (Binary.IsValid() && Binary.CanConvertTo<ezString>())
  {
    ezUInt8 Data[1024];
    ConvertHexToBinary(Binary.ConvertTo<ezString>().GetData(), Data, EZ_ARRAY_SIZE(Data));

    tValue = *((T*) &Data[0]);
  }

  return ezVariant(tValue);
}


bool IsCloseEnough(double d1, double d2, double fEpsilon)
{
  return ezMath::IsEqual(d1, d2, fEpsilon);
}

bool IsCloseEnough(float d1, float d2, float fEpsilon)
{
  return ezMath::IsEqual(d1, d2, fEpsilon);
}

template<typename T>
bool IsCloseEnough(T d1, T d2, T fEpsilon)
{
  return d1 == d2;
}

// convert a number from either string or binary representation to its actual value
// chose the one that is 'more precise'
template<typename T>
T BuildTypedVariant_number(const ezVariant& Value, const ezVariant& Binary, T fEpsilon)
{
  T tValue = T();
  T bValue = T();
  bool bText = false;
  bool bBinary = false;

  if (Value.IsValid() && Value.CanConvertTo<T>())
  {
    ezResult Status(EZ_FAILURE);
    tValue = Value.ConvertTo<T>(&Status);

    bText = Status.IsSuccess();
  }

  if (Binary.IsValid() && Binary.CanConvertTo<ezString>())
  {
    ezResult Status(EZ_FAILURE);

    ezUInt8 Data[1024];
    ConvertHexToBinary(Binary.ConvertTo<ezString>(&Status).GetData(), Data, EZ_ARRAY_SIZE(Data));

    bValue = *((T*) &Data[0]);

    bBinary = Status.IsSuccess();
  }

  if (bText && bBinary)
  {
    // if they are really close, prefer the binary value
    if (IsCloseEnough(tValue, bValue, fEpsilon))
      return bValue;
  }

  if (bText)
  {
    return tValue;
  }

  return bValue;
}

// same as BuildTypedVariant_number, but for vector types (must be float's though)
template<typename T>
T BuildTypedVariant_vector(const ezVariant& Value, const ezVariant& Binary, float fEpsilon)
{
  T tValue = T();
  T bValue = T();
  bool bText = false;
  bool bBinary = false;
  const ezInt32 iNumFloats = sizeof(T) / sizeof(float);

  float* ptValueFloats = (float*) &tValue;
  float* pbValueFloats = (float*) &bValue;

  if (Value.IsValid() && Value.CanConvertTo<ezString>())
  {
    ezResult Status(EZ_FAILURE);
    ParseFloats(Value.ConvertTo<ezString>(&Status).GetData(), ptValueFloats, iNumFloats);

    bText = Status.IsSuccess();
  }

  if (Binary.IsValid() && Binary.CanConvertTo<ezString>())
  {
    ezResult Status(EZ_FAILURE);

    ezUInt8 Data[1024];
    ConvertHexToBinary(Binary.ConvertTo<ezString>(&Status).GetData(), Data, EZ_ARRAY_SIZE(Data));

    bValue = *((T*) &Data[0]);

    bBinary = Status.IsSuccess();
  }

  if (bText && bBinary)
  {
    for (ezInt32 i = 0; i < iNumFloats; ++i)
    {
      // if they are really close, prefer the binary value
      // here we do this on a per-component level
      // copy it into the text representation
      if (IsCloseEnough(ptValueFloats[i], pbValueFloats[i], fEpsilon))
        ptValueFloats[i] = pbValueFloats[i];
    }
  }
  
  if (bText)
  {
    return tValue;
  }

  return bValue;
}

ezVariant BuildTypedVariant(const char* szType, const ezVariant& Value, const ezVariant& Binary)
{
  if (ezStringUtils::IsEqual(szType, "int32"))
    return ezVariant(BuildTypedVariant_number<ezInt32>(Value, Binary, 0));

  if (ezStringUtils::IsEqual(szType, "uint32"))
    return ezVariant(BuildTypedVariant_number<ezUInt32>(Value, Binary, 0));

  if (ezStringUtils::IsEqual(szType, "int64"))
    return ezVariant(BuildTypedVariant_number<ezInt64>(Value, Binary, 0));

  if (ezStringUtils::IsEqual(szType, "uint64"))
    return ezVariant(BuildTypedVariant_number<ezUInt64>(Value, Binary, 0));

  if (ezStringUtils::IsEqual(szType, "float"))
    return ezVariant(BuildTypedVariant_number<float>(Value, Binary, 0.00009f));

  if (ezStringUtils::IsEqual(szType, "double"))
    return ezVariant(BuildTypedVariant_number<double>(Value, Binary, 0.000000009f));

  if (ezStringUtils::IsEqual(szType, "time"))
    return ezVariant(ezTime::Seconds(BuildTypedVariant_number<float>(Value, Binary, 0.00009f)));

  if (ezStringUtils::IsEqual(szType, "color"))
    return ezVariant(BuildTypedVariant_vector<ezColor>(Value, Binary, 0.00009f));

  if (ezStringUtils::IsEqual(szType, "vec2"))
    return ezVariant(BuildTypedVariant_vector<ezVec2>(Value, Binary, 0.00009f));

  if (ezStringUtils::IsEqual(szType, "vec3"))
    return ezVariant(BuildTypedVariant_vector<ezVec3>(Value, Binary, 0.00009f));

  if (ezStringUtils::IsEqual(szType, "vec4"))
    return ezVariant(BuildTypedVariant_vector<ezVec4>(Value, Binary, 0.00009f));

  if (ezStringUtils::IsEqual(szType, "quat"))
    return BuildTypedVariant_binary<ezQuat>(Binary);

  if (ezStringUtils::IsEqual(szType, "mat3"))
    return BuildTypedVariant_binary<ezMat3>(Binary);

  if (ezStringUtils::IsEqual(szType, "mat4"))
    return BuildTypedVariant_binary<ezMat4>(Binary);

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
    ezVariant v = BuildTypedVariant(Type.ConvertTo<ezString>().GetData(), Value, Binary);

    if (m_Stack.GetCount() > 1)
    {
      Element& Parent = m_Stack[m_Stack.GetCount() - 2];

      if (Parent.m_iMode == 0)
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

      /// \todo Some kind of error message would be great
    }
  }
  else
  {
    if (m_Stack.GetCount() > 1)
    {
      Element& Parent = m_Stack[m_Stack.GetCount() - 2];

      if (Parent.m_iMode == 0)
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
