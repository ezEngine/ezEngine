#include <Foundation/PCH.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/Uuid.h>
#include <Foundation/Types/Variant.h>


ezResult ezOpenDdlUtils::ConvertToColor(const ezOpenDdlReaderElement* pElement, ezColor& out_result)
{
  if (pElement == nullptr)
    return EZ_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return EZ_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetPrimitivesType() == ezOpenDdlPrimitiveType::Float)
  {
    const float* pValues = pElement->GetPrimitivesFloat();

    if (pElement->GetNumPrimitives() == 4)
    {
      out_result.r = pValues[0];
      out_result.g = pValues[1];
      out_result.b = pValues[2];
      out_result.a = pValues[3];

      return EZ_SUCCESS;
    }

    if (pElement->GetNumPrimitives() == 3)
    {
      out_result.r = pValues[0];
      out_result.g = pValues[1];
      out_result.b = pValues[2];
      out_result.a = 1.0f;

      return EZ_SUCCESS;
    }
  }
  else if (pElement->GetPrimitivesType() == ezOpenDdlPrimitiveType::UInt8)
  {
    const ezUInt8* pValues = pElement->GetPrimitivesUInt8();

    if (pElement->GetNumPrimitives() == 4)
    {
      out_result = ezColorGammaUB(pValues[0], pValues[1], pValues[2], pValues[3]);

      return EZ_SUCCESS;
    }

    if (pElement->GetNumPrimitives() == 3)
    {
      out_result = ezColorGammaUB(pValues[0], pValues[1], pValues[2]);

      return EZ_SUCCESS;
    }
  }

  return EZ_FAILURE;
}

ezResult ezOpenDdlUtils::ConvertToColorGamma(const ezOpenDdlReaderElement* pElement, ezColorGammaUB& out_result)
{
  if (pElement == nullptr)
    return EZ_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return EZ_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetPrimitivesType() == ezOpenDdlPrimitiveType::Float)
  {
    const float* pValues = pElement->GetPrimitivesFloat();

    if (pElement->GetNumPrimitives() == 4)
    {
      out_result = ezColor(pValues[0], pValues[1], pValues[2], pValues[3]);

      return EZ_SUCCESS;
    }

    if (pElement->GetNumPrimitives() == 3)
    {
      out_result = ezColor(pValues[0], pValues[1], pValues[2]);

      return EZ_SUCCESS;
    }
  }
  else if (pElement->GetPrimitivesType() == ezOpenDdlPrimitiveType::UInt8)
  {
    const ezUInt8* pValues = pElement->GetPrimitivesUInt8();

    if (pElement->GetNumPrimitives() == 4)
    {
      out_result.r = pValues[0];
      out_result.g = pValues[1];
      out_result.b = pValues[2];
      out_result.a = pValues[3];

      return EZ_SUCCESS;
    }

    if (pElement->GetNumPrimitives() == 3)
    {
      out_result.r = pValues[0];
      out_result.g = pValues[1];
      out_result.b = pValues[2];
      out_result.a = 255;

      return EZ_SUCCESS;
    }
  }

  return EZ_FAILURE;
}

ezResult ezOpenDdlUtils::ConvertToTime(const ezOpenDdlReaderElement* pElement, ezTime& out_result)
{
  if (pElement == nullptr)
    return EZ_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return EZ_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 1)
    return EZ_FAILURE;

  if (pElement->GetPrimitivesType() == ezOpenDdlPrimitiveType::Float)
  {
    const float* pValues = pElement->GetPrimitivesFloat();

    out_result = ezTime::Seconds(pValues[0]);

    return EZ_SUCCESS;
  }

  if (pElement->GetPrimitivesType() == ezOpenDdlPrimitiveType::Double)
  {
    const double* pValues = pElement->GetPrimitivesDouble();

    out_result = ezTime::Seconds(pValues[0]);

    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

ezResult ezOpenDdlUtils::ConvertToVec2(const ezOpenDdlReaderElement* pElement, ezVec2& out_result)
{
  if (pElement == nullptr)
    return EZ_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return EZ_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 2)
    return EZ_FAILURE;

  if (pElement->GetPrimitivesType() == ezOpenDdlPrimitiveType::Float)
  {
    const float* pValues = pElement->GetPrimitivesFloat();

    out_result.Set(pValues[0], pValues[1]);

    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

ezResult ezOpenDdlUtils::ConvertToVec3(const ezOpenDdlReaderElement* pElement, ezVec3& out_result)
{
  if (pElement == nullptr)
    return EZ_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return EZ_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 3)
    return EZ_FAILURE;

  if (pElement->GetPrimitivesType() == ezOpenDdlPrimitiveType::Float)
  {
    const float* pValues = pElement->GetPrimitivesFloat();

    out_result.Set(pValues[0], pValues[1], pValues[2]);

    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

ezResult ezOpenDdlUtils::ConvertToVec4(const ezOpenDdlReaderElement* pElement, ezVec4& out_result)
{
  if (pElement == nullptr)
    return EZ_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return EZ_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 4)
    return EZ_FAILURE;

  if (pElement->GetPrimitivesType() == ezOpenDdlPrimitiveType::Float)
  {
    const float* pValues = pElement->GetPrimitivesFloat();

    out_result.Set(pValues[0], pValues[1], pValues[2], pValues[3]);

    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

ezResult ezOpenDdlUtils::ConvertToMat3(const ezOpenDdlReaderElement* pElement, ezMat3& out_result)
{
  if (pElement == nullptr)
    return EZ_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return EZ_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 9)
    return EZ_FAILURE;

  if (pElement->GetPrimitivesType() == ezOpenDdlPrimitiveType::Float)
  {
    const float* pValues = pElement->GetPrimitivesFloat();

    out_result.SetFromArray(pValues, ezMatrixLayout::ColumnMajor);

    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

ezResult ezOpenDdlUtils::ConvertToMat4(const ezOpenDdlReaderElement* pElement, ezMat4& out_result)
{
  if (pElement == nullptr)
    return EZ_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return EZ_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 16)
    return EZ_FAILURE;

  if (pElement->GetPrimitivesType() == ezOpenDdlPrimitiveType::Float)
  {
    const float* pValues = pElement->GetPrimitivesFloat();

    out_result.SetFromArray(pValues, ezMatrixLayout::ColumnMajor);

    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}


ezResult ezOpenDdlUtils::ConvertToTransform(const ezOpenDdlReaderElement* pElement, ezTransform& out_result)
{
  if (pElement == nullptr)
    return EZ_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return EZ_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 12)
    return EZ_FAILURE;

  if (pElement->GetPrimitivesType() == ezOpenDdlPrimitiveType::Float)
  {
    const float* pValues = pElement->GetPrimitivesFloat();

    out_result.m_Rotation.SetFromArray(pValues, ezMatrixLayout::ColumnMajor);
    out_result.m_vPosition.Set(pValues[9], pValues[10], pValues[11]);

    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

ezResult ezOpenDdlUtils::ConvertToQuat(const ezOpenDdlReaderElement* pElement, ezQuat& out_result)
{
  if (pElement == nullptr)
    return EZ_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return EZ_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 4)
    return EZ_FAILURE;

  if (pElement->GetPrimitivesType() == ezOpenDdlPrimitiveType::Float)
  {
    const float* pValues = pElement->GetPrimitivesFloat();

    out_result.SetElements(pValues[0], pValues[1], pValues[2], pValues[3]);

    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

ezResult ezOpenDdlUtils::ConvertToUuid(const ezOpenDdlReaderElement* pElement, ezUuid& out_result)
{
  if (pElement == nullptr)
    return EZ_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return EZ_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 2)
    return EZ_FAILURE;

  if (pElement->GetPrimitivesType() == ezOpenDdlPrimitiveType::UInt64)
  {
    const ezUInt64* pValues = pElement->GetPrimitivesUInt64();

    out_result = ezUuid(pValues[0], pValues[1]);

    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

ezResult ezOpenDdlUtils::ConvertToAngle(const ezOpenDdlReaderElement* pElement, ezAngle& out_result)
{
  if (pElement == nullptr)
    return EZ_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return EZ_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 1)
    return EZ_FAILURE;

  if (pElement->GetPrimitivesType() == ezOpenDdlPrimitiveType::Float)
  {
    const float* pValues = pElement->GetPrimitivesFloat();

    out_result = ezAngle::Degree(pValues[0]);

    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

ezResult ezOpenDdlUtils::ConvertToVariant(const ezOpenDdlReaderElement* pElement, ezVariant& out_result)
{
  if (pElement == nullptr)
    return EZ_FAILURE;

  // expect a custom type
  if (pElement->IsCustomType())
  {
    if (ezStringUtils::IsEqual(pElement->GetCustomType(), "VarArray"))
    {
      ezVariantArray value;
      ezVariant varChild;

      /// \test This is just quickly hacked
      /// \todo Store array size for reserving var array length

      for (const ezOpenDdlReaderElement* pChild = pElement->GetFirstChild(); pChild != nullptr; pChild = pChild->GetSibling())
      {
        if (ConvertToVariant(pChild, varChild).Failed())
          return EZ_FAILURE;

        value.PushBack(varChild);
      }

      out_result = value;
      return EZ_SUCCESS;
    }

    // always expect exactly one child
    if (pElement->GetNumChildObjects() != 1)
      return EZ_FAILURE;

    if (ezStringUtils::IsEqual(pElement->GetCustomType(), "Color"))
    {
      ezColor value;
      if (ConvertToColor(pElement, value).Failed())
        return EZ_FAILURE;

      out_result = value;
      return EZ_SUCCESS;
    }

    if (ezStringUtils::IsEqual(pElement->GetCustomType(), "ColorGamma"))
    {
      ezColorGammaUB value;
      if (ConvertToColorGamma(pElement, value).Failed())
        return EZ_FAILURE;

      out_result = value;
      return EZ_SUCCESS;
    }

    if (ezStringUtils::IsEqual(pElement->GetCustomType(), "Time"))
    {
      ezTime value;
      if (ConvertToTime(pElement, value).Failed())
        return EZ_FAILURE;

      out_result = value;
      return EZ_SUCCESS;
    }

    if (ezStringUtils::IsEqual(pElement->GetCustomType(), "Vec2"))
    {
      ezVec2 value;
      if (ConvertToVec2(pElement, value).Failed())
        return EZ_FAILURE;

      out_result = value;
      return EZ_SUCCESS;
    }

    if (ezStringUtils::IsEqual(pElement->GetCustomType(), "Vec3"))
    {
      ezVec3 value;
      if (ConvertToVec3(pElement, value).Failed())
        return EZ_FAILURE;

      out_result = value;
      return EZ_SUCCESS;
    }

    if (ezStringUtils::IsEqual(pElement->GetCustomType(), "Vec4"))
    {
      ezVec4 value;
      if (ConvertToVec4(pElement, value).Failed())
        return EZ_FAILURE;

      out_result = value;
      return EZ_SUCCESS;
    }

    if (ezStringUtils::IsEqual(pElement->GetCustomType(), "Mat3"))
    {
      ezMat3 value;
      if (ConvertToMat3(pElement, value).Failed())
        return EZ_FAILURE;

      out_result = value;
      return EZ_SUCCESS;
    }

    if (ezStringUtils::IsEqual(pElement->GetCustomType(), "Mat4"))
    {
      ezMat4 value;
      if (ConvertToMat4(pElement, value).Failed())
        return EZ_FAILURE;

      out_result = value;
      return EZ_SUCCESS;
    }

    if (ezStringUtils::IsEqual(pElement->GetCustomType(), "Transform"))
    {
      ezTransform value;
      if (ConvertToTransform(pElement, value).Failed())
        return EZ_FAILURE;

      out_result = value;
      return EZ_SUCCESS;
    }

    if (ezStringUtils::IsEqual(pElement->GetCustomType(), "Quat"))
    {
      ezQuat value;
      if (ConvertToQuat(pElement, value).Failed())
        return EZ_FAILURE;

      out_result = value;
      return EZ_SUCCESS;
    }

    if (ezStringUtils::IsEqual(pElement->GetCustomType(), "Uuid"))
    {
      ezUuid value;
      if (ConvertToUuid(pElement, value).Failed())
        return EZ_FAILURE;

      out_result = value;
      return EZ_SUCCESS;
    }

    if (ezStringUtils::IsEqual(pElement->GetCustomType(), "Angle"))
    {
      ezAngle value;
      if (ConvertToAngle(pElement, value).Failed())
        return EZ_FAILURE;

      out_result = value;
      return EZ_SUCCESS;
    }
  }
  else
  {
    // always expect exactly one value
    if (pElement->GetNumPrimitives() != 1)
      return EZ_FAILURE;

    switch (pElement->GetPrimitivesType())
    {
    case ezOpenDdlPrimitiveType::Bool:
      out_result = pElement->GetPrimitivesBool()[0];
      return EZ_SUCCESS;

    case ezOpenDdlPrimitiveType::Int8:
      out_result = pElement->GetPrimitivesInt8()[0];
      return EZ_SUCCESS;

    case ezOpenDdlPrimitiveType::Int16:
      out_result = pElement->GetPrimitivesInt16()[0];
      return EZ_SUCCESS;

    case ezOpenDdlPrimitiveType::Int32:
      out_result = pElement->GetPrimitivesInt32()[0];
      return EZ_SUCCESS;

    case ezOpenDdlPrimitiveType::Int64:
      out_result = pElement->GetPrimitivesInt64()[0];
      return EZ_SUCCESS;

    case ezOpenDdlPrimitiveType::UInt8:
      out_result = pElement->GetPrimitivesUInt8()[0];
      return EZ_SUCCESS;

    case ezOpenDdlPrimitiveType::UInt16:
      out_result = pElement->GetPrimitivesUInt16()[0];
      return EZ_SUCCESS;

    case ezOpenDdlPrimitiveType::UInt32:
      out_result = pElement->GetPrimitivesUInt32()[0];
      return EZ_SUCCESS;

    case ezOpenDdlPrimitiveType::UInt64:
      out_result = pElement->GetPrimitivesUInt64()[0];
      return EZ_SUCCESS;

    case ezOpenDdlPrimitiveType::Float:
      out_result = pElement->GetPrimitivesFloat()[0];
      return EZ_SUCCESS;

    case ezOpenDdlPrimitiveType::Double:
      out_result = pElement->GetPrimitivesDouble()[0];
      return EZ_SUCCESS;

    case ezOpenDdlPrimitiveType::String:
      out_result = ezString(pElement->GetPrimitivesString()[0]); // make sure this isn't stored as a string view
      return EZ_SUCCESS;
    }
  }

  return EZ_FAILURE;
}

void ezOpenDdlUtils::StoreColor(ezOpenDdlWriter& writer, const ezColor& value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginObject("Color", szName, bGlobalName);
  {
    writer.BeginPrimitiveList(ezOpenDdlPrimitiveType::Float);
    writer.WriteFloat(value.GetData(), 4);
    writer.EndPrimitiveList();
  }
  writer.EndObject();
}

void ezOpenDdlUtils::StoreColorGamma(ezOpenDdlWriter& writer, const ezColorGammaUB& value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginObject("ColorGamma", szName, bGlobalName);
  {
    writer.BeginPrimitiveList(ezOpenDdlPrimitiveType::UInt8);
    writer.WriteUInt8(value.GetData(), 4);
    writer.EndPrimitiveList();
  }
  writer.EndObject();
}

void ezOpenDdlUtils::StoreTime(ezOpenDdlWriter& writer, const ezTime& value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginObject("Time", szName, bGlobalName);
  {
    const double d = value.GetSeconds();

    writer.BeginPrimitiveList(ezOpenDdlPrimitiveType::Double);
    writer.WriteDouble(&d, 1);
    writer.EndPrimitiveList();
  }
  writer.EndObject();
}

void ezOpenDdlUtils::StoreVec2(ezOpenDdlWriter& writer, const ezVec2& value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginObject("Vec2", szName, bGlobalName);
  {
    writer.BeginPrimitiveList(ezOpenDdlPrimitiveType::Float);
    writer.WriteFloat(value.GetData(), 2);
    writer.EndPrimitiveList();
  }
  writer.EndObject();
}

void ezOpenDdlUtils::StoreVec3(ezOpenDdlWriter& writer, const ezVec3& value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginObject("Vec3", szName, bGlobalName);
  {
    writer.BeginPrimitiveList(ezOpenDdlPrimitiveType::Float);
    writer.WriteFloat(value.GetData(), 3);
    writer.EndPrimitiveList();
  }
  writer.EndObject();
}

void ezOpenDdlUtils::StoreVec4(ezOpenDdlWriter& writer, const ezVec4& value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginObject("Vec4", szName, bGlobalName);
  {
    writer.BeginPrimitiveList(ezOpenDdlPrimitiveType::Float);
    writer.WriteFloat(value.GetData(), 4);
    writer.EndPrimitiveList();
  }
  writer.EndObject();
}

void ezOpenDdlUtils::StoreMat3(ezOpenDdlWriter& writer, const ezMat3& value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginObject("Mat3", szName, bGlobalName);
  {
    writer.BeginPrimitiveList(ezOpenDdlPrimitiveType::Float);

    float f[9];
    value.GetAsArray(f, ezMatrixLayout::ColumnMajor);
    writer.WriteFloat(f, 9);
    writer.EndPrimitiveList();
  }
  writer.EndObject();
}

void ezOpenDdlUtils::StoreMat4(ezOpenDdlWriter& writer, const ezMat4& value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginObject("Mat4", szName, bGlobalName);
  {
    writer.BeginPrimitiveList(ezOpenDdlPrimitiveType::Float);

    float f[16];
    value.GetAsArray(f, ezMatrixLayout::ColumnMajor);
    writer.WriteFloat(f, 16);
    writer.EndPrimitiveList();
  }
  writer.EndObject();
}

void ezOpenDdlUtils::StoreTransform(ezOpenDdlWriter& writer, const ezTransform& value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginObject("Transform", szName, bGlobalName);
  {
    writer.BeginPrimitiveList(ezOpenDdlPrimitiveType::Float);

    float f[12];
    value.m_Rotation.GetAsArray(f, ezMatrixLayout::ColumnMajor);
    f[9] = value.m_vPosition.x;
    f[10] = value.m_vPosition.y;
    f[11] = value.m_vPosition.z;

    writer.WriteFloat(f, 12);
    writer.EndPrimitiveList();
  }
  writer.EndObject();
}

void ezOpenDdlUtils::StoreQuat(ezOpenDdlWriter& writer, const ezQuat& value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginObject("Quat", szName, bGlobalName);
  {
    writer.BeginPrimitiveList(ezOpenDdlPrimitiveType::Float);
    writer.WriteFloat(value.v.GetData(), 4);
    writer.EndPrimitiveList();
  }
  writer.EndObject();
}

void ezOpenDdlUtils::StoreUuid(ezOpenDdlWriter& writer, const ezUuid& value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginObject("Uuid", szName, bGlobalName);
  {
    ezUInt64 ui[2];
    value.GetValues(ui[0], ui[1]);

    writer.BeginPrimitiveList(ezOpenDdlPrimitiveType::UInt64);
    writer.WriteUInt64(ui, 2);
    writer.EndPrimitiveList();
  }
  writer.EndObject();
}

void ezOpenDdlUtils::StoreAngle(ezOpenDdlWriter& writer, const ezAngle& value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginObject("Angle", szName, bGlobalName);
  {
    const float f = value.GetDegree();

    writer.BeginPrimitiveList(ezOpenDdlPrimitiveType::Float);
    writer.WriteFloat(&f, 1);
    writer.EndPrimitiveList();
  }
  writer.EndObject();
}

void ezOpenDdlUtils::StoreVariant(ezOpenDdlWriter& writer, const ezVariant& value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  switch (value.GetType())
  {
  case ezVariant::Type::Invalid:
    return; // store anything ?

  case ezVariant::Type::Bool:
    {
      const bool var = value.Get<bool>();

      writer.BeginPrimitiveList(ezOpenDdlPrimitiveType::Bool, szName, bGlobalName);
      writer.WriteBool(&var);
      writer.EndPrimitiveList();
    }
    return;

  case ezVariant::Type::Int8:
    {
      const ezInt8 var = value.Get<ezInt8>();

      writer.BeginPrimitiveList(ezOpenDdlPrimitiveType::Int8, szName, bGlobalName);
      writer.WriteInt8(&var);
      writer.EndPrimitiveList();
    }
    return;

  case ezVariant::Type::UInt8:
    {
      const ezUInt8 var = value.Get<ezUInt8>();

      writer.BeginPrimitiveList(ezOpenDdlPrimitiveType::UInt8, szName, bGlobalName);
      writer.WriteUInt8(&var);
      writer.EndPrimitiveList();
    }
    return;

  case ezVariant::Type::Int16:
    {
      const ezInt16 var = value.Get<ezInt16>();

      writer.BeginPrimitiveList(ezOpenDdlPrimitiveType::Int16, szName, bGlobalName);
      writer.WriteInt16(&var);
      writer.EndPrimitiveList();
    }
    return;

  case ezVariant::Type::UInt16:
    {
      const ezUInt16 var = value.Get<ezUInt16>();

      writer.BeginPrimitiveList(ezOpenDdlPrimitiveType::UInt16, szName, bGlobalName);
      writer.WriteUInt16(&var);
      writer.EndPrimitiveList();
    }
    return;

  case ezVariant::Type::Int32:
    {
      const ezInt32 var = value.Get<ezInt32>();

      writer.BeginPrimitiveList(ezOpenDdlPrimitiveType::Int32, szName, bGlobalName);
      writer.WriteInt32(&var);
      writer.EndPrimitiveList();
    }
    return;

  case ezVariant::Type::UInt32:
    {
      const ezUInt32 var = value.Get<ezUInt32>();

      writer.BeginPrimitiveList(ezOpenDdlPrimitiveType::UInt32, szName, bGlobalName);
      writer.WriteUInt32(&var);
      writer.EndPrimitiveList();
    }
    return;

  case ezVariant::Type::Int64:
    {
      const ezInt64 var = value.Get<ezInt64>();

      writer.BeginPrimitiveList(ezOpenDdlPrimitiveType::Int64, szName, bGlobalName);
      writer.WriteInt64(&var);
      writer.EndPrimitiveList();
    }
    return;

  case ezVariant::Type::UInt64:
    {
      const ezUInt64 var = value.Get<ezUInt64>();

      writer.BeginPrimitiveList(ezOpenDdlPrimitiveType::UInt64, szName, bGlobalName);
      writer.WriteUInt64(&var);
      writer.EndPrimitiveList();
    }
    return;

  case ezVariant::Type::Float:
    {
      const float var = value.Get<float>();

      writer.BeginPrimitiveList(ezOpenDdlPrimitiveType::Float, szName, bGlobalName);
      writer.WriteFloat(&var);
      writer.EndPrimitiveList();
    }
    return;

  case ezVariant::Type::Double:
    {
      const double var = value.Get<double>();

      writer.BeginPrimitiveList(ezOpenDdlPrimitiveType::Double, szName, bGlobalName);
      writer.WriteDouble(&var);
      writer.EndPrimitiveList();
    }
    return;

  case ezVariant::Type::String:
    {
      const ezString& var = value.Get<ezString>();
      ezOpenDdlUtils::StoreString(writer, var, szName, bGlobalName);
    }
    return;

  case ezVariant::Type::StringView:
    {
      const ezStringView& var = value.Get<ezStringView>();
      ezOpenDdlUtils::StoreString(writer, var, szName, bGlobalName);
    }
    return;

  case ezVariant::Type::Color:
    StoreColor(writer, value.Get<ezColor>(), szName, bGlobalName);
    return;

  case ezVariant::Type::Vector2:
    StoreVec2(writer, value.Get<ezVec2>(), szName, bGlobalName);
    return;

  case ezVariant::Type::Vector3:
    StoreVec3(writer, value.Get<ezVec3>(), szName, bGlobalName);
    return;

  case ezVariant::Type::Vector4:
    StoreVec4(writer, value.Get<ezVec4>(), szName, bGlobalName);
    return;

  case ezVariant::Type::Quaternion:
    StoreQuat(writer, value.Get<ezQuat>(), szName, bGlobalName);
    return;

  case ezVariant::Type::Matrix3:
    StoreMat3(writer, value.Get<ezMat3>(), szName, bGlobalName);
    return;

  case ezVariant::Type::Matrix4:
    StoreMat4(writer, value.Get<ezMat4>(), szName, bGlobalName);
    return;

  case ezVariant::Type::Transform:
    StoreTransform(writer, value.Get<ezTransform>(), szName, bGlobalName);
    return;

  case ezVariant::Type::Time:
    StoreTime(writer, value.Get<ezTime>(), szName, bGlobalName);
    return;

  case ezVariant::Type::Uuid:
    StoreUuid(writer, value.Get<ezUuid>(), szName, bGlobalName);
    return;

  case ezVariant::Type::Angle:
    StoreAngle(writer, value.Get<ezAngle>(), szName, bGlobalName);
    return;

  case ezVariant::Type::ColorGamma:
    StoreColorGamma(writer, value.Get<ezColorGammaUB>(), szName, bGlobalName);
    return;

  case ezVariant::Type::VariantArray:
    {
      /// \test This is just quickly hacked

      writer.BeginObject("VarArray", szName, bGlobalName);

      const ezVariantArray& arr = value.Get<ezVariantArray>();
      for (ezUInt32 i = 0; i < arr.GetCount(); ++i)
      {
        ezOpenDdlUtils::StoreVariant(writer, arr[i]);
      }

      writer.EndObject();
    }
    return;

  default:
    EZ_REPORT_FAILURE("Can't write this type of Variant");
  }
}

void ezOpenDdlUtils::StoreString(ezOpenDdlWriter& writer, const ezStringView& value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginPrimitiveList(ezOpenDdlPrimitiveType::String, szName, bGlobalName);
  writer.WriteString(value);
  writer.EndPrimitiveList();
}


