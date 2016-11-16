#include <Foundation/PCH.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlReader.h>
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
  if (!pElement->IsCustomType())
    return EZ_FAILURE;

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

  return EZ_FAILURE;
}


