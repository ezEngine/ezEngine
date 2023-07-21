#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

#include <Duktape/duktape.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

//////////////////////////////////////////////////////////////////////////

void ezTypeScriptBinding::PushVec2(duk_context* pDuk, const ezVec2& value)
{
  ezDuktapeHelper duk(pDuk);

  duk.PushGlobalObject();                                   // [ global ]
  EZ_VERIFY(duk.PushLocalObject("__Vec2").Succeeded(), ""); // [ global __Vec2 ]
  duk_get_prop_string(duk, -1, "Vec2");                     // [ global __Vec2 Vec2 ]
  duk_push_number(duk, value.x);                            // [ global __Vec2 Vec2 x ]
  duk_push_number(duk, value.y);                            // [ global __Vec2 Vec2 x y ]
  duk_new(duk, 2);                                          // [ global __Vec2 result ]
  duk_remove(duk, -2);                                      // [ global result ]
  duk_remove(duk, -2);                                      // [ result ]

  EZ_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, +1);
}

void ezTypeScriptBinding::SetVec2(duk_context* pDuk, ezInt32 iObjIdx, const ezVec2& value)
{
  ezDuktapeHelper duk(pDuk);

  duk.SetNumberProperty("x", value.x, iObjIdx);
  duk.SetNumberProperty("y", value.y, iObjIdx);

  EZ_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

void ezTypeScriptBinding::SetVec2Property(duk_context* pDuk, const char* szPropertyName, ezInt32 iObjIdx, const ezVec2& value)
{
  ezDuktapeHelper duk(pDuk);

  EZ_VERIFY(duk.PushLocalObject(szPropertyName, iObjIdx).Succeeded(), "invalid property");
  SetVec2(pDuk, -1, value);
  duk.PopStack();

  EZ_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

ezVec2 ezTypeScriptBinding::GetVec2(duk_context* pDuk, ezInt32 iObjIdx, const ezVec2& vFallback /*= ezVec2::ZeroVector()*/)
{
  if (duk_is_null_or_undefined(pDuk, iObjIdx))
    return vFallback;

  ezVec2 res;

  EZ_VERIFY(duk_get_prop_string(pDuk, iObjIdx, "x"), "");
  res.x = static_cast<float>(duk_get_number_default(pDuk, -1, vFallback.x));
  duk_pop(pDuk);
  EZ_VERIFY(duk_get_prop_string(pDuk, iObjIdx, "y"), "");
  res.y = static_cast<float>(duk_get_number_default(pDuk, -1, vFallback.y));
  duk_pop(pDuk);

  return res;
}

ezVec2 ezTypeScriptBinding::GetVec2Property(
  duk_context* pDuk, const char* szPropertyName, ezInt32 iObjIdx, const ezVec2& vFallback /*= ezVec2::ZeroVector()*/)
{
  ezDuktapeHelper duk(pDuk);

  if (duk.PushLocalObject(szPropertyName, iObjIdx).Failed()) // [ prop ]
  {
    EZ_DUK_RETURN_AND_VERIFY_STACK(duk, vFallback, 0);
  }

  const ezVec2 res = GetVec2(pDuk, -1, vFallback);
  duk.PopStack(); // [ ]
  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, res, 0);
}

//////////////////////////////////////////////////////////////////////////

void ezTypeScriptBinding::PushVec3(duk_context* pDuk, const ezVec3& value)
{
  ezDuktapeHelper duk(pDuk);

  duk.PushGlobalObject();                                   // [ global ]
  EZ_VERIFY(duk.PushLocalObject("__Vec3").Succeeded(), ""); // [ global __Vec3 ]
  duk_get_prop_string(duk, -1, "Vec3");                     // [ global __Vec3 Vec3 ]
  duk_push_number(duk, value.x);                            // [ global __Vec3 Vec3 x ]
  duk_push_number(duk, value.y);                            // [ global __Vec3 Vec3 x y ]
  duk_push_number(duk, value.z);                            // [ global __Vec3 Vec3 x y z ]
  duk_new(duk, 3);                                          // [ global __Vec3 result ]
  duk_remove(duk, -2);                                      // [ global result ]
  duk_remove(duk, -2);                                      // [ result ]

  EZ_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, +1);
}

void ezTypeScriptBinding::SetVec3(duk_context* pDuk, ezInt32 iObjIdx, const ezVec3& value)
{
  ezDuktapeHelper duk(pDuk);

  duk.SetNumberProperty("x", value.x, iObjIdx);
  duk.SetNumberProperty("y", value.y, iObjIdx);
  duk.SetNumberProperty("z", value.z, iObjIdx);
}

void ezTypeScriptBinding::SetVec3Property(duk_context* pDuk, const char* szPropertyName, ezInt32 iObjIdx, const ezVec3& value)
{
  ezDuktapeHelper duk(pDuk);

  EZ_VERIFY(duk.PushLocalObject(szPropertyName, iObjIdx).Succeeded(), "invalid property");
  SetVec3(pDuk, -1, value);
  duk.PopStack();

  EZ_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

ezVec3 ezTypeScriptBinding::GetVec3(duk_context* pDuk, ezInt32 iObjIdx, const ezVec3& vFallback /*= ezVec3::ZeroVector()*/)
{
  if (duk_is_null_or_undefined(pDuk, iObjIdx))
    return vFallback;

  ezVec3 res;

  EZ_VERIFY(duk_get_prop_string(pDuk, iObjIdx, "x"), "");
  res.x = static_cast<float>(duk_get_number_default(pDuk, -1, vFallback.x));
  duk_pop(pDuk);
  EZ_VERIFY(duk_get_prop_string(pDuk, iObjIdx, "y"), "");
  res.y = static_cast<float>(duk_get_number_default(pDuk, -1, vFallback.y));
  duk_pop(pDuk);
  EZ_VERIFY(duk_get_prop_string(pDuk, iObjIdx, "z"), "");
  res.z = static_cast<float>(duk_get_number_default(pDuk, -1, vFallback.z));
  duk_pop(pDuk);

  return res;
}

ezVec3 ezTypeScriptBinding::GetVec3Property(
  duk_context* pDuk, const char* szPropertyName, ezInt32 iObjIdx, const ezVec3& vFallback /*= ezVec3::ZeroVector()*/)
{
  ezDuktapeHelper duk(pDuk);

  if (duk.PushLocalObject(szPropertyName, iObjIdx).Failed()) // [ prop ]
  {
    EZ_DUK_RETURN_AND_VERIFY_STACK(duk, vFallback, 0);
  }

  const ezVec3 res = GetVec3(pDuk, -1, vFallback);
  duk.PopStack(); // [ ]

  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, res, 0);
}

//////////////////////////////////////////////////////////////////////////

void ezTypeScriptBinding::PushMat3(duk_context* pDuk, const ezMat3& value)
{
  ezDuktapeHelper duk(pDuk);

  duk.PushGlobalObject();                                   // [ global ]
  EZ_VERIFY(duk.PushLocalObject("__Mat3").Succeeded(), ""); // [ global __Mat3 ]
  duk_get_prop_string(duk, -1, "Mat3");                     // [ global __Mat3 Mat3 ]

  float rm[9];
  value.GetAsArray(rm, ezMatrixLayout::RowMajor);

  for (ezUInt32 i = 0; i < 9; ++i)
  {
    duk_push_number(duk, rm[i]); // [ global __Mat3 Mat3 9params ]
  }

  duk_new(duk, 9);     // [ global __Mat3 result ]
  duk_remove(duk, -2); // [ global result ]
  duk_remove(duk, -2); // [ result ]

  EZ_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, +1);
}

void ezTypeScriptBinding::SetMat3(duk_context* pDuk, ezInt32 iObjIdx, const ezMat3& value)
{
  ezDuktapeHelper duk(pDuk);

  EZ_VERIFY(duk.PushLocalObject("m_ElementsCM", iObjIdx).Succeeded(), "invalid property");
  duk.SetNumberProperty("0", value.m_fElementsCM[0], -1);
  duk.SetNumberProperty("1", value.m_fElementsCM[1], -1);
  duk.SetNumberProperty("2", value.m_fElementsCM[2], -1);
  duk.SetNumberProperty("3", value.m_fElementsCM[3], -1);
  duk.SetNumberProperty("4", value.m_fElementsCM[4], -1);
  duk.SetNumberProperty("5", value.m_fElementsCM[5], -1);
  duk.SetNumberProperty("6", value.m_fElementsCM[6], -1);
  duk.SetNumberProperty("7", value.m_fElementsCM[7], -1);
  duk.SetNumberProperty("8", value.m_fElementsCM[8], -1);
  duk.PopStack();

  EZ_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

void ezTypeScriptBinding::SetMat3Property(duk_context* pDuk, const char* szPropertyName, ezInt32 iObjIdx, const ezMat3& value)
{
  ezDuktapeHelper duk(pDuk);

  EZ_VERIFY(duk.PushLocalObject(szPropertyName, iObjIdx).Succeeded(), "invalid property");
  SetMat3(pDuk, -1, value);
  duk.PopStack();

  EZ_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

ezMat3 ezTypeScriptBinding::GetMat3(duk_context* pDuk, ezInt32 iObjIdx, const ezMat3& mFallback /*= ezMat3::ZeroVector()*/)
{
  if (duk_is_null_or_undefined(pDuk, iObjIdx))
    return mFallback;

  ezMat3 res;

  ezDuktapeHelper duk(pDuk);

  EZ_VERIFY(duk.PushLocalObject("m_ElementsCM", iObjIdx).Succeeded(), "invalid property");

  res.m_fElementsCM[0] = duk.GetFloatProperty("0", 0.0f);
  res.m_fElementsCM[1] = duk.GetFloatProperty("1", 0.0f);
  res.m_fElementsCM[2] = duk.GetFloatProperty("2", 0.0f);
  res.m_fElementsCM[3] = duk.GetFloatProperty("3", 0.0f);
  res.m_fElementsCM[4] = duk.GetFloatProperty("4", 0.0f);
  res.m_fElementsCM[5] = duk.GetFloatProperty("5", 0.0f);
  res.m_fElementsCM[6] = duk.GetFloatProperty("6", 0.0f);
  res.m_fElementsCM[7] = duk.GetFloatProperty("7", 0.0f);
  res.m_fElementsCM[8] = duk.GetFloatProperty("8", 0.0f);

  duk.PopStack();

  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, res, 0);
}

ezMat3 ezTypeScriptBinding::GetMat3Property(
  duk_context* pDuk, const char* szPropertyName, ezInt32 iObjIdx, const ezMat3& mFallback /*= ezMat3::ZeroVector()*/)
{
  ezDuktapeHelper duk(pDuk);

  if (duk.PushLocalObject(szPropertyName, iObjIdx).Failed()) // [ prop ]
  {
    EZ_DUK_RETURN_AND_VERIFY_STACK(duk, mFallback, 0);
  }

  const ezMat3 res = GetMat3(pDuk, -1, mFallback);
  duk.PopStack(); // [ ]

  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, res, 0);
}

//////////////////////////////////////////////////////////////////////////

void ezTypeScriptBinding::PushMat4(duk_context* pDuk, const ezMat4& value)
{
  ezDuktapeHelper duk(pDuk);

  duk.PushGlobalObject();                                   // [ global ]
  EZ_VERIFY(duk.PushLocalObject("__Mat4").Succeeded(), ""); // [ global __Mat4 ]
  duk_get_prop_string(duk, -1, "Mat4");                     // [ global __Mat4 Mat4 ]

  float rm[16];
  value.GetAsArray(rm, ezMatrixLayout::RowMajor);

  for (ezUInt32 i = 0; i < 16; ++i)
  {
    duk_push_number(duk, rm[i]); // [ global __Mat4 Mat4 16params ]
  }

  duk_new(duk, 16);    // [ global __Mat4 result ]
  duk_remove(duk, -2); // [ global result ]
  duk_remove(duk, -2); // [ result ]

  EZ_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, +1);
}

void ezTypeScriptBinding::SetMat4(duk_context* pDuk, ezInt32 iObjIdx, const ezMat4& value)
{
  ezDuktapeHelper duk(pDuk);

  EZ_VERIFY(duk.PushLocalObject("m_ElementsCM", iObjIdx).Succeeded(), "invalid property");
  duk.SetNumberProperty("0", value.m_fElementsCM[0], -1);
  duk.SetNumberProperty("1", value.m_fElementsCM[1], -1);
  duk.SetNumberProperty("2", value.m_fElementsCM[2], -1);
  duk.SetNumberProperty("3", value.m_fElementsCM[3], -1);
  duk.SetNumberProperty("4", value.m_fElementsCM[4], -1);
  duk.SetNumberProperty("5", value.m_fElementsCM[5], -1);
  duk.SetNumberProperty("6", value.m_fElementsCM[6], -1);
  duk.SetNumberProperty("7", value.m_fElementsCM[7], -1);
  duk.SetNumberProperty("8", value.m_fElementsCM[8], -1);
  duk.SetNumberProperty("9", value.m_fElementsCM[9], -1);
  duk.SetNumberProperty("10", value.m_fElementsCM[10], -1);
  duk.SetNumberProperty("11", value.m_fElementsCM[11], -1);
  duk.SetNumberProperty("12", value.m_fElementsCM[12], -1);
  duk.SetNumberProperty("13", value.m_fElementsCM[13], -1);
  duk.SetNumberProperty("14", value.m_fElementsCM[14], -1);
  duk.SetNumberProperty("15", value.m_fElementsCM[15], -1);
  duk.PopStack();

  EZ_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

void ezTypeScriptBinding::SetMat4Property(duk_context* pDuk, const char* szPropertyName, ezInt32 iObjIdx, const ezMat4& value)
{
  ezDuktapeHelper duk(pDuk);

  EZ_VERIFY(duk.PushLocalObject(szPropertyName, iObjIdx).Succeeded(), "invalid property");
  SetMat4(pDuk, -1, value);
  duk.PopStack();

  EZ_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

ezMat4 ezTypeScriptBinding::GetMat4(duk_context* pDuk, ezInt32 iObjIdx, const ezMat4& mFallback /*= ezMat4::ZeroVector()*/)
{
  if (duk_is_null_or_undefined(pDuk, iObjIdx))
    return mFallback;

  ezMat4 res;

  ezDuktapeHelper duk(pDuk);

  EZ_VERIFY(duk.PushLocalObject("m_ElementsCM", iObjIdx).Succeeded(), "invalid property");

  res.m_fElementsCM[0] = duk.GetFloatProperty("0", 0.0f);
  res.m_fElementsCM[1] = duk.GetFloatProperty("1", 0.0f);
  res.m_fElementsCM[2] = duk.GetFloatProperty("2", 0.0f);
  res.m_fElementsCM[3] = duk.GetFloatProperty("3", 0.0f);
  res.m_fElementsCM[4] = duk.GetFloatProperty("4", 0.0f);
  res.m_fElementsCM[5] = duk.GetFloatProperty("5", 0.0f);
  res.m_fElementsCM[6] = duk.GetFloatProperty("6", 0.0f);
  res.m_fElementsCM[7] = duk.GetFloatProperty("7", 0.0f);
  res.m_fElementsCM[8] = duk.GetFloatProperty("8", 0.0f);
  res.m_fElementsCM[9] = duk.GetFloatProperty("9", 0.0f);
  res.m_fElementsCM[10] = duk.GetFloatProperty("10", 0.0f);
  res.m_fElementsCM[11] = duk.GetFloatProperty("11", 0.0f);
  res.m_fElementsCM[12] = duk.GetFloatProperty("12", 0.0f);
  res.m_fElementsCM[13] = duk.GetFloatProperty("13", 0.0f);
  res.m_fElementsCM[14] = duk.GetFloatProperty("14", 0.0f);
  res.m_fElementsCM[15] = duk.GetFloatProperty("15", 0.0f);

  duk.PopStack();

  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, res, 0);
}

ezMat4 ezTypeScriptBinding::GetMat4Property(
  duk_context* pDuk, const char* szPropertyName, ezInt32 iObjIdx, const ezMat4& mFallback /*= ezMat4::ZeroVector()*/)
{
  ezDuktapeHelper duk(pDuk);

  if (duk.PushLocalObject(szPropertyName, iObjIdx).Failed()) // [ prop ]
  {
    EZ_DUK_RETURN_AND_VERIFY_STACK(duk, mFallback, 0);
  }

  const ezMat4 res = GetMat4(pDuk, -1, mFallback);
  duk.PopStack(); // [ ]

  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, res, 0);
}

//////////////////////////////////////////////////////////////////////////

void ezTypeScriptBinding::PushQuat(duk_context* pDuk, const ezQuat& value)
{
  ezDuktapeHelper duk(pDuk);

  duk.PushGlobalObject();                                   // [ global ]
  EZ_VERIFY(duk.PushLocalObject("__Quat").Succeeded(), ""); // [ global __Quat ]
  duk_get_prop_string(duk, -1, "Quat");                     // [ global __Quat Quat ]
  duk_push_number(duk, value.x);                          // [ global __Quat Quat x ]
  duk_push_number(duk, value.y);                          // [ global __Quat Quat x y ]
  duk_push_number(duk, value.z);                          // [ global __Quat Quat x y z ]
  duk_push_number(duk, value.w);                            // [ global __Quat Quat x y z w ]
  duk_new(duk, 4);                                          // [ global __Quat result ]
  duk_remove(duk, -2);                                      // [ global result ]
  duk_remove(duk, -2);                                      // [ result ]

  EZ_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, +1);
}

void ezTypeScriptBinding::SetQuat(duk_context* pDuk, ezInt32 iObjIdx, const ezQuat& value)
{
  ezDuktapeHelper duk(pDuk);

  duk.SetNumberProperty("x", value.x, iObjIdx);
  duk.SetNumberProperty("y", value.y, iObjIdx);
  duk.SetNumberProperty("z", value.z, iObjIdx);
  duk.SetNumberProperty("w", value.w, iObjIdx);
}

void ezTypeScriptBinding::SetQuatProperty(duk_context* pDuk, const char* szPropertyName, ezInt32 iObjIdx, const ezQuat& value)
{
  ezDuktapeHelper duk(pDuk);

  EZ_VERIFY(duk.PushLocalObject(szPropertyName, iObjIdx).Succeeded(), "invalid property");
  SetQuat(pDuk, -1, value);
  duk.PopStack();

  EZ_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

ezQuat ezTypeScriptBinding::GetQuat(duk_context* pDuk, ezInt32 iObjIdx, ezQuat qFallback /*= ezQuat::IdentityQuaternion()*/)
{
  if (duk_is_null_or_undefined(pDuk, iObjIdx))
    return qFallback;

  ezQuat res;

  EZ_VERIFY(duk_get_prop_string(pDuk, iObjIdx, "x"), "");
  res.x = static_cast<float>(duk_get_number_default(pDuk, -1, qFallback.x));
  duk_pop(pDuk);
  EZ_VERIFY(duk_get_prop_string(pDuk, iObjIdx, "y"), "");
  res.y = static_cast<float>(duk_get_number_default(pDuk, -1, qFallback.y));
  duk_pop(pDuk);
  EZ_VERIFY(duk_get_prop_string(pDuk, iObjIdx, "z"), "");
  res.z = static_cast<float>(duk_get_number_default(pDuk, -1, qFallback.z));
  duk_pop(pDuk);
  EZ_VERIFY(duk_get_prop_string(pDuk, iObjIdx, "w"), "");
  res.w = static_cast<float>(duk_get_number_default(pDuk, -1, qFallback.w));
  duk_pop(pDuk);

  return res;
}

ezQuat ezTypeScriptBinding::GetQuatProperty(
  duk_context* pDuk, const char* szPropertyName, ezInt32 iObjIdx, ezQuat qFallback /*= ezQuat::IdentityQuaternion()*/)
{
  ezDuktapeHelper duk(pDuk);

  if (duk.PushLocalObject(szPropertyName, iObjIdx).Failed()) // [ prop ]
  {
    EZ_DUK_RETURN_AND_VERIFY_STACK(duk, qFallback, 0);
  }

  const ezQuat res = GetQuat(pDuk, -1, qFallback);
  duk.PopStack(); // [ ]
  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, res, 0);
}

//////////////////////////////////////////////////////////////////////////

void ezTypeScriptBinding::PushColor(duk_context* pDuk, const ezColor& value)
{
  ezDuktapeHelper duk(pDuk);

  duk.PushGlobalObject();                                    // [ global ]
  EZ_VERIFY(duk.PushLocalObject("__Color").Succeeded(), ""); // [ global __Color ]
  duk_get_prop_string(duk, -1, "Color");                     // [ global __Color Color ]
  duk_push_number(duk, value.r);                             // [ global __Color Color r ]
  duk_push_number(duk, value.g);                             // [ global __Color Color r g ]
  duk_push_number(duk, value.b);                             // [ global __Color Color r g b ]
  duk_push_number(duk, value.a);                             // [ global __Color Color r g b a ]
  duk_new(duk, 4);                                           // [ global __Color result ]
  duk_remove(duk, -2);                                       // [ global result ]
  duk_remove(duk, -2);                                       // [ result ]

  EZ_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, +1);
}

void ezTypeScriptBinding::SetColor(duk_context* pDuk, ezInt32 iObjIdx, const ezColor& value)
{
  ezDuktapeHelper duk(pDuk);

  duk.SetNumberProperty("r", value.r, iObjIdx);
  duk.SetNumberProperty("g", value.g, iObjIdx);
  duk.SetNumberProperty("b", value.b, iObjIdx);
  duk.SetNumberProperty("a", value.a, iObjIdx);

  EZ_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

void ezTypeScriptBinding::SetColorProperty(duk_context* pDuk, const char* szPropertyName, ezInt32 iObjIdx, const ezColor& value)
{
  ezDuktapeHelper duk(pDuk);

  EZ_VERIFY(duk.PushLocalObject(szPropertyName, iObjIdx).Succeeded(), "invalid property");
  SetColor(pDuk, -1, value);
  duk.PopStack();

  EZ_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

ezColor ezTypeScriptBinding::GetColor(duk_context* pDuk, ezInt32 iObjIdx, const ezColor& fallback /*= ezColor::White*/)
{
  ezDuktapeHelper duk(pDuk);

  ezColor res;
  res.r = duk.GetFloatProperty("r", fallback.r, iObjIdx);
  res.g = duk.GetFloatProperty("g", fallback.g, iObjIdx);
  res.b = duk.GetFloatProperty("b", fallback.b, iObjIdx);
  res.a = duk.GetFloatProperty("a", fallback.a, iObjIdx);

  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, res, 0);
}

ezColor ezTypeScriptBinding::GetColorProperty(
  duk_context* pDuk, const char* szPropertyName, ezInt32 iObjIdx, const ezColor& fallback /*= ezColor::White*/)
{
  ezDuktapeHelper duk(pDuk);

  if (duk.PushLocalObject(szPropertyName, iObjIdx).Failed()) // [ prop ]
  {
    EZ_DUK_RETURN_AND_VERIFY_STACK(duk, fallback, 0);
  }

  const ezColor res = GetColor(pDuk, -1, fallback);
  duk.PopStack(); // [ ]
  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, res, 0);
}

//////////////////////////////////////////////////////////////////////////

void ezTypeScriptBinding::PushTransform(duk_context* pDuk, const ezTransform& value)
{
  ezDuktapeHelper duk(pDuk);

  duk.PushGlobalObject();                                        // [ global ]
  EZ_VERIFY(duk.PushLocalObject("__Transform").Succeeded(), ""); // [ global __Transform ]
  duk_get_prop_string(duk, -1, "Transform");                     // [ global __Transform Transform ]
  duk_new(duk, 0);                                               // [ global __Transform object ]
  SetVec3Property(pDuk, "position", -1, value.m_vPosition);      // [ global __Transform object ]
  SetQuatProperty(pDuk, "rotation", -1, value.m_qRotation);      // [ global __Transform object ]
  SetVec3Property(pDuk, "scale", -1, value.m_vScale);            // [ global __Transform object ]
  duk_remove(duk, -2);                                           // [ global object ]
  duk_remove(duk, -2);                                           // [ object ]

  EZ_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, +1);
}

void ezTypeScriptBinding::SetTransform(duk_context* pDuk, ezInt32 iObjIdx, const ezTransform& value)
{
  SetVec3Property(pDuk, "position", iObjIdx, value.m_vPosition);
  SetQuatProperty(pDuk, "rotation", iObjIdx, value.m_qRotation);
  SetVec3Property(pDuk, "scale", iObjIdx, value.m_vScale);
}

void ezTypeScriptBinding::SetTransformProperty(duk_context* pDuk, const char* szPropertyName, ezInt32 iObjIdx, const ezTransform& value)
{
  ezDuktapeHelper duk(pDuk);

  EZ_VERIFY(duk.PushLocalObject(szPropertyName, iObjIdx).Succeeded(), "invalid property");
  SetTransform(pDuk, -1, value);
  duk.PopStack();

  EZ_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

ezTransform ezTypeScriptBinding::GetTransform(duk_context* pDuk, ezInt32 iObjIdx, const ezTransform& fallback /*= ezTransform::IdentityTransform()*/)
{
  if (duk_is_null_or_undefined(pDuk, iObjIdx))
    return fallback;

  ezTransform res;

  res.m_vPosition = GetVec3Property(pDuk, "position", iObjIdx, fallback.m_vPosition);
  res.m_qRotation = GetQuatProperty(pDuk, "rotation", iObjIdx, fallback.m_qRotation);
  res.m_vScale = GetVec3Property(pDuk, "scale", iObjIdx, fallback.m_vScale);

  return res;
}

ezTransform ezTypeScriptBinding::GetTransformProperty(
  duk_context* pDuk, const char* szPropertyName, ezInt32 iObjIdx, const ezTransform& fallback /*= ezTransform::IdentityTransform()*/)
{
  ezDuktapeHelper duk(pDuk);

  if (duk.PushLocalObject(szPropertyName, iObjIdx).Failed()) // [ prop ]
  {
    EZ_DUK_RETURN_AND_VERIFY_STACK(duk, fallback, 0);
  }

  const ezTransform res = GetTransform(pDuk, -1, fallback);
  duk.PopStack(); // [ ]
  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, res, 0);
}

//////////////////////////////////////////////////////////////////////////

void ezTypeScriptBinding::PushVariant(duk_context* pDuk, const ezVariant& value)
{
  ezDuktapeHelper duk(pDuk);

  switch (value.GetType())
  {
    case ezVariant::Type::Angle:
      duk.PushNumber(value.Get<ezAngle>().GetRadian());
      break;

    case ezVariant::Type::Time:
      duk.PushNumber(value.Get<ezTime>().GetSeconds());
      break;

    case ezVariant::Type::Bool:
      duk.PushBool(value.Get<bool>());
      break;

    case ezVariant::Type::Int8:
    case ezVariant::Type::UInt8:
    case ezVariant::Type::Int16:
    case ezVariant::Type::UInt16:
    case ezVariant::Type::Int32:
    case ezVariant::Type::UInt32:
    case ezVariant::Type::Int64:
    case ezVariant::Type::UInt64:
    case ezVariant::Type::Float:
    case ezVariant::Type::Double:
      duk.PushNumber(value.ConvertTo<double>());
      break;

    case ezVariant::Type::Color:
    case ezVariant::Type::ColorGamma:
      PushColor(duk, value.ConvertTo<ezColor>());
      break;

    case ezVariant::Type::Vector2:
      PushVec2(duk, value.Get<ezVec2>());
      break;

    case ezVariant::Type::Vector3:
      PushVec3(duk, value.Get<ezVec3>());
      break;

    case ezVariant::Type::Quaternion:
      PushQuat(duk, value.Get<ezQuat>());
      break;

    case ezVariant::Type::Transform:
      PushTransform(duk, value.Get<ezTransform>());
      break;

    case ezVariant::Type::String:
      duk.PushString(value.Get<ezString>());
      break;

    case ezVariant::Type::StringView:
      duk.PushString(value.Get<ezStringView>());
      break;

    case ezVariant::Type::Vector2I:
    {
      const ezVec2I32 v = value.Get<ezVec2I32>();
      PushVec2(duk, ezVec2(static_cast<float>(v.x), static_cast<float>(v.y)));
      break;
    }

    case ezVariant::Type::Vector2U:
    {
      const ezVec2U32 v = value.Get<ezVec2U32>();
      PushVec2(duk, ezVec2(static_cast<float>(v.x), static_cast<float>(v.y)));
      break;
    }

    case ezVariant::Type::Vector3I:
    {
      const ezVec3I32 v = value.Get<ezVec3I32>();
      PushVec3(duk, ezVec3(static_cast<float>(v.x), static_cast<float>(v.y), static_cast<float>(v.z)));
      break;
    }

    case ezVariant::Type::Vector3U:
    {
      const ezVec3U32 v = value.Get<ezVec3U32>();
      PushVec3(duk, ezVec3(static_cast<float>(v.x), static_cast<float>(v.y), static_cast<float>(v.z)));
      break;
    }

    case ezVariant::Type::Matrix3:
      PushMat3(duk, value.Get<ezMat3>());
      break;

    case ezVariant::Type::Matrix4:
      PushMat4(duk, value.Get<ezMat4>());
      break;

      // TODO: implement these types
      // case ezVariant::Type::Vector4:
      // case ezVariant::Type::Vector4I:
      // case ezVariant::Type::Vector4U:

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      duk.PushUndefined();
      break;
  }

  EZ_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, +1);
}

void ezTypeScriptBinding::SetVariantProperty(duk_context* pDuk, const char* szPropertyName, ezInt32 iObjIdx, const ezVariant& value)
{
  ezDuktapeHelper duk(pDuk);

  PushVariant(pDuk, value);
  duk.SetCustomProperty(szPropertyName, iObjIdx);

  EZ_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

ezVariant ezTypeScriptBinding::GetVariant(duk_context* pDuk, ezInt32 iObjIdx, const ezRTTI* pType)
{
  ezDuktapeHelper duk(pDuk);

  if (pType->IsDerivedFrom<ezEnumBase>() || pType->IsDerivedFrom<ezBitflagsBase>())
  {
    return duk.GetIntValue(iObjIdx);
  }

  switch (pType->GetVariantType())
  {
    case ezVariant::Type::Invalid:
    {
      if (pType->GetTypeName() == "ezVariant")
      {
        switch (duk_get_type(duk.GetContext(), iObjIdx))
        {
          case DUK_TYPE_BOOLEAN:
            return duk.GetBoolValue(iObjIdx);
          case DUK_TYPE_NUMBER:
            return duk.GetFloatValue(iObjIdx);
          case DUK_TYPE_STRING:
            return duk.GetStringValue(iObjIdx);

          default:
            return ezVariant();
        }
      }

      return ezVariant();
    }

    case ezVariant::Type::Bool:
      return duk.GetBoolValue(iObjIdx);

    case ezVariant::Type::Angle:
      return ezAngle::Radian(duk.GetFloatValue(iObjIdx));

    case ezVariant::Type::Time:
      return ezTime::Seconds(duk.GetFloatValue(iObjIdx));

    case ezVariant::Type::Int8:
    case ezVariant::Type::Int16:
    case ezVariant::Type::Int32:
    case ezVariant::Type::Int64:
      return duk.GetIntValue(iObjIdx);

    case ezVariant::Type::UInt8:
    case ezVariant::Type::UInt16:
    case ezVariant::Type::UInt32:
    case ezVariant::Type::UInt64:
      return duk.GetUIntValue(iObjIdx);

    case ezVariant::Type::Float:
      return duk.GetFloatValue(iObjIdx);

    case ezVariant::Type::Double:
      return duk.GetNumberValue(iObjIdx);

    case ezVariant::Type::String:
      return duk.GetStringValue(iObjIdx);

    case ezVariant::Type::StringView:
      return ezVariant(ezStringView(duk.GetStringValue(iObjIdx)), false);

    case ezVariant::Type::Vector2:
      return ezTypeScriptBinding::GetVec2(duk, iObjIdx);

    case ezVariant::Type::Vector3:
      return ezTypeScriptBinding::GetVec3(duk, iObjIdx);

    case ezVariant::Type::Quaternion:
      return ezTypeScriptBinding::GetQuat(duk, iObjIdx);

    case ezVariant::Type::Transform:
      return ezTypeScriptBinding::GetTransform(duk, iObjIdx);

    case ezVariant::Type::Color:
      return ezTypeScriptBinding::GetColor(duk, iObjIdx);

    case ezVariant::Type::ColorGamma:
      return ezColorGammaUB(ezTypeScriptBinding::GetColor(duk, iObjIdx));

    case ezVariant::Type::Vector2I:
    {
      const ezVec2 v = ezTypeScriptBinding::GetVec2(duk, iObjIdx);
      return ezVec2I32(static_cast<ezInt32>(v.x), static_cast<ezInt32>(v.y));
    }

    case ezVariant::Type::Vector3I:
    {
      const ezVec3 v = ezTypeScriptBinding::GetVec3(duk, iObjIdx);
      return ezVec3I32(static_cast<ezInt32>(v.x), static_cast<ezInt32>(v.y), static_cast<ezInt32>(v.z));
    }

    case ezVariant::Type::Vector2U:
    {
      const ezVec2 v = ezTypeScriptBinding::GetVec2(duk, iObjIdx);
      return ezVec2U32(static_cast<ezUInt32>(v.x), static_cast<ezUInt32>(v.y));
    }

    case ezVariant::Type::Vector3U:
    {
      const ezVec3 v = ezTypeScriptBinding::GetVec3(duk, iObjIdx);
      return ezVec3U32(static_cast<ezUInt32>(v.x), static_cast<ezUInt32>(v.y), static_cast<ezUInt32>(v.z));
    }

    case ezVariant::Type::Matrix3:
      return ezTypeScriptBinding::GetMat3(duk, iObjIdx);

    case ezVariant::Type::Matrix4:
      return ezTypeScriptBinding::GetMat4(duk, iObjIdx);

      // case ezVariant::Type::Vector4:
      //  break;
      // case ezVariant::Type::Vector4I:
      //  break;
      // case ezVariant::Type::Vector4U:
      //  break;

      // case ezVariant::Type::Uuid:
      //  break;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  return ezVariant();
}

ezVariant ezTypeScriptBinding::GetVariantProperty(duk_context* pDuk, const char* szPropertyName, ezInt32 iObjIdx, const ezRTTI* pType)
{
  ezDuktapeHelper duk(pDuk);

  if (duk.PushLocalObject(szPropertyName, iObjIdx).Failed()) // [ prop ]
  {
    return ezVariant();
  }

  const ezVariant res = GetVariant(pDuk, -1, pType);
  duk.PopStack(); // [ ]

  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, res, 0);
}
