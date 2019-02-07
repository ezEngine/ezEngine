#include <FoundationTestPCH.h>

#include <Foundation/Containers/Deque.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Strings/StringUtils.h>
#include <FoundationTest/IO/JSONTestHelpers.h>

static ezVariant CreateVariant(ezVariant::Type::Enum t, const void* data);

EZ_CREATE_SIMPLE_TEST(IO, DdlUtils)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezOpenDdlUtils::ConvertToColor")
  {
    const char* szTestData = "\
Color $c1 { float { 1, 0, 0.5 } }\
Color $c2 { float { 2, 1, 1.5, 0.1 } }\
Color $c3 { unsigned_int8 { 128, 2, 32 } }\
Color $c4 { unsigned_int8 { 128, 0, 32, 64 } }\
float $c5 { 1, 0, 0.5 }\
float $c6 { 2, 1, 1.5, 0.1 }\
unsigned_int8 $c7 { 128, 2, 32 }\
unsigned_int8 $c8 { 128, 0, 32, 64 }\
Color $c9 { float { 1, 0 } }\
Color $c10 { float { 1, 0, 3, 4, 5 } }\
Color $c11 { float { } }\
Color $c12 { }\
Color $c13 { double { 1, 1, 1, 2 } }\
";

    StringStream stream(szTestData);
    ezOpenDdlReader doc;
    EZ_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    ezColor c1, c2, c3, c4, c5, c6, c7, c8, c0;

    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToColor(doc.FindElement("t0"), c0).Failed());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToColor(doc.FindElement("c1"), c1).Succeeded());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToColor(doc.FindElement("c2"), c2).Succeeded());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToColor(doc.FindElement("c3"), c3).Succeeded());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToColor(doc.FindElement("c4"), c4).Succeeded());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToColor(doc.FindElement("c5"), c5).Succeeded());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToColor(doc.FindElement("c6"), c6).Succeeded());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToColor(doc.FindElement("c7"), c7).Succeeded());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToColor(doc.FindElement("c8"), c8).Succeeded());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToColor(doc.FindElement("c9"), c0).Failed());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToColor(doc.FindElement("c10"), c0).Failed());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToColor(doc.FindElement("c11"), c0).Failed());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToColor(doc.FindElement("c12"), c0).Failed());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToColor(doc.FindElement("c13"), c0).Failed());

    EZ_TEST_BOOL(c1 == ezColor(1, 0, 0.5f, 1.0f));
    EZ_TEST_BOOL(c2 == ezColor(2, 1, 1.5f, 0.1f));
    EZ_TEST_BOOL(c3 == ezColorGammaUB(128, 2, 32));
    EZ_TEST_BOOL(c4 == ezColorGammaUB(128, 0, 32, 64));
    EZ_TEST_BOOL(c5 == ezColor(1, 0, 0.5f, 1.0f));
    EZ_TEST_BOOL(c6 == ezColor(2, 1, 1.5f, 0.1f));
    EZ_TEST_BOOL(c7 == ezColorGammaUB(128, 2, 32));
    EZ_TEST_BOOL(c8 == ezColorGammaUB(128, 0, 32, 64));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezOpenDdlUtils::ConvertToColorGamma")
  {
    const char* szTestData = "\
Color $c1 { float { 1, 0, 0.5 } }\
Color $c2 { float { 2, 1, 1.5, 0.1 } }\
Color $c3 { unsigned_int8 { 128, 2, 32 } }\
Color $c4 { unsigned_int8 { 128, 0, 32, 64 } }\
float $c5 { 1, 0, 0.5 }\
float $c6 { 2, 1, 1.5, 0.1 }\
unsigned_int8 $c7 { 128, 2, 32 }\
unsigned_int8 $c8 { 128, 0, 32, 64 }\
Color $c9 { float { 1, 0 } }\
Color $c10 { float { 1, 0, 3, 4, 5 } }\
Color $c11 { float { } }\
Color $c12 { }\
Color $c13 { double { 1, 1, 1, 2 } }\
";

    StringStream stream(szTestData);
    ezOpenDdlReader doc;
    EZ_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    ezColorGammaUB c1, c2, c3, c4, c5, c6, c7, c8, c0;

    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToColorGamma(doc.FindElement("t0"), c0).Failed());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c1"), c1).Succeeded());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c2"), c2).Succeeded());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c3"), c3).Succeeded());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c4"), c4).Succeeded());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c5"), c5).Succeeded());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c6"), c6).Succeeded());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c7"), c7).Succeeded());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c8"), c8).Succeeded());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c9"), c0).Failed());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c10"), c0).Failed());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c11"), c0).Failed());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c12"), c0).Failed());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c13"), c0).Failed());

    EZ_TEST_BOOL(c1 == ezColorGammaUB(ezColor(1, 0, 0.5f, 1.0f)));
    EZ_TEST_BOOL(c2 == ezColorGammaUB(ezColor(2, 1, 1.5f, 0.1f)));
    EZ_TEST_BOOL(c3 == ezColorGammaUB(128, 2, 32));
    EZ_TEST_BOOL(c4 == ezColorGammaUB(128, 0, 32, 64));
    EZ_TEST_BOOL(c5 == ezColorGammaUB(ezColor(1, 0, 0.5f, 1.0f)));
    EZ_TEST_BOOL(c6 == ezColorGammaUB(ezColor(2, 1, 1.5f, 0.1f)));
    EZ_TEST_BOOL(c7 == ezColorGammaUB(128, 2, 32));
    EZ_TEST_BOOL(c8 == ezColorGammaUB(128, 0, 32, 64));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezOpenDdlUtils::ConvertToTime")
  {
    const char* szTestData = "\
Time $t1 { float { 0.1 } }\
Time $t2 { double { 0.2 } }\
float $t3 { 0.3 }\
double $t4 { 0.4 }\
Time $t5 { double { 0.2, 2 } }\
Time $t6 { int8 { 0, 2 } }\
";

    StringStream stream(szTestData);
    ezOpenDdlReader doc;
    EZ_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    ezTime t1, t2, t3, t4, t0;

    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToTime(doc.FindElement("t0"), t0).Failed());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToTime(doc.FindElement("t1"), t1).Succeeded());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToTime(doc.FindElement("t2"), t2).Succeeded());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToTime(doc.FindElement("t3"), t3).Succeeded());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToTime(doc.FindElement("t4"), t4).Succeeded());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToTime(doc.FindElement("t5"), t0).Failed());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToTime(doc.FindElement("t6"), t0).Failed());

    EZ_TEST_FLOAT(t1.GetSeconds(), 0.1, 0.0001f);
    EZ_TEST_FLOAT(t2.GetSeconds(), 0.2, 0.0001f);
    EZ_TEST_FLOAT(t3.GetSeconds(), 0.3, 0.0001f);
    EZ_TEST_FLOAT(t4.GetSeconds(), 0.4, 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezOpenDdlUtils::ConvertToVec2")
  {
    const char* szTestData = "\
Vector $v1 { float { 0.1, 2 } }\
float $v2 { 0.3, 3 }\
Vector $v3 { float { 0.1 } }\
Vector $v4 { float { 0.1, 2.2, 3.33 } }\
";

    StringStream stream(szTestData);
    ezOpenDdlReader doc;
    EZ_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    ezVec2 v0, v1, v2;

    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToVec2(doc.FindElement("v0"), v0).Failed());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToVec2(doc.FindElement("v1"), v1).Succeeded());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToVec2(doc.FindElement("v2"), v2).Succeeded());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToVec2(doc.FindElement("v3"), v0).Failed());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToVec2(doc.FindElement("v4"), v0).Failed());

    EZ_TEST_VEC2(v1, ezVec2(0.1f, 2.0f), 0.0001f);
    EZ_TEST_VEC2(v2, ezVec2(0.3f, 3.0f), 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezOpenDdlUtils::ConvertToVec3")
  {
    const char* szTestData = "\
Vector $v1 { float { 0.1, 2, 3.2 } }\
float $v2 { 0.3, 3,0}\
Vector $v3 { float { 0.1,2 } }\
Vector $v4 { float { 0.1, 2.2, 3.33,44 } }\
";

    StringStream stream(szTestData);
    ezOpenDdlReader doc;
    EZ_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    ezVec3 v0, v1, v2;

    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToVec3(doc.FindElement("v0"), v0).Failed());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToVec3(doc.FindElement("v1"), v1).Succeeded());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToVec3(doc.FindElement("v2"), v2).Succeeded());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToVec3(doc.FindElement("v3"), v0).Failed());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToVec3(doc.FindElement("v4"), v0).Failed());

    EZ_TEST_VEC3(v1, ezVec3(0.1f, 2.0f, 3.2f), 0.0001f);
    EZ_TEST_VEC3(v2, ezVec3(0.3f, 3.0f, 0.0f), 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezOpenDdlUtils::ConvertToVec4")
  {
    const char* szTestData = "\
Vector $v1 { float { 0.1, 2, 3.2, 44.5 } }\
float $v2 { 0.3, 3,0, 12.}\
Vector $v3 { float { 0.1,2 } }\
Vector $v4 { float { 0.1, 2.2, 3.33, 44, 67 } }\
";

    StringStream stream(szTestData);
    ezOpenDdlReader doc;
    EZ_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    ezVec4 v0, v1, v2;

    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToVec4(doc.FindElement("v0"), v0).Failed());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToVec4(doc.FindElement("v1"), v1).Succeeded());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToVec4(doc.FindElement("v2"), v2).Succeeded());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToVec4(doc.FindElement("v3"), v0).Failed());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToVec4(doc.FindElement("v4"), v0).Failed());

    EZ_TEST_VEC4(v1, ezVec4(0.1f, 2.0f, 3.2f, 44.5f), 0.0001f);
    EZ_TEST_VEC4(v2, ezVec4(0.3f, 3.0f, 0.0f, 12.0f), 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezOpenDdlUtils::ConvertToMat3")
  {
    const char* szTestData = "\
Group $v1 { float { 1, 2, 3, 4, 5, 6, 7, 8, 9 } }\
";

    StringStream stream(szTestData);
    ezOpenDdlReader doc;
    EZ_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    ezMat3 v0, v1;

    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToMat3(doc.FindElement("v0"), v0).Failed());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToMat3(doc.FindElement("v1"), v1).Succeeded());

    EZ_TEST_BOOL(v1.IsEqual(ezMat3(1, 4, 7, 2, 5, 8, 3, 6, 9), 0.0001f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezOpenDdlUtils::ConvertToMat4")
  {
    const char* szTestData = "\
Group $v1 { float { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 } }\
";

    StringStream stream(szTestData);
    ezOpenDdlReader doc;
    EZ_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    ezMat4 v0, v1;

    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToMat4(doc.FindElement("v0"), v0).Failed());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToMat4(doc.FindElement("v1"), v1).Succeeded());

    EZ_TEST_BOOL(v1.IsEqual(ezMat4(1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15, 4, 8, 12, 16), 0.0001f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezOpenDdlUtils::ConvertToTransform")
  {
    const char* szTestData = "\
Group $v1 { float { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 } }\
";

    StringStream stream(szTestData);
    ezOpenDdlReader doc;
    EZ_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    ezTransform v0, v1;

    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToTransform(doc.FindElement("v0"), v0).Failed());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToTransform(doc.FindElement("v1"), v1).Succeeded());

    EZ_TEST_VEC3(v1.m_vPosition, ezVec3(1, 2, 3), 0.0001f);
    EZ_TEST_BOOL(v1.m_qRotation == ezQuat(4, 5, 6, 7));
    EZ_TEST_VEC3(v1.m_vScale, ezVec3(8, 9, 10), 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezOpenDdlUtils::ConvertToQuat")
  {
    const char* szTestData = "\
Vector $v1 { float { 0.1, 2, 3.2, 44.5 } }\
float $v2 { 0.3, 3,0, 12.}\
Vector $v3 { float { 0.1,2 } }\
Vector $v4 { float { 0.1, 2.2, 3.33, 44, 67 } }\
";

    StringStream stream(szTestData);
    ezOpenDdlReader doc;
    EZ_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    ezQuat v0, v1, v2;

    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToQuat(doc.FindElement("v0"), v0).Failed());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToQuat(doc.FindElement("v1"), v1).Succeeded());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToQuat(doc.FindElement("v2"), v2).Succeeded());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToQuat(doc.FindElement("v3"), v0).Failed());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToQuat(doc.FindElement("v4"), v0).Failed());

    EZ_TEST_BOOL(v1 == ezQuat(0.1f, 2.0f, 3.2f, 44.5f));
    EZ_TEST_BOOL(v2 == ezQuat(0.3f, 3.0f, 0.0f, 12.0f));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezOpenDdlUtils::ConvertToUuid")
  {
    const char* szTestData = "\
Data $v1 { unsigned_int64 { 12345678910, 10987654321 } }\
";

    StringStream stream(szTestData);
    ezOpenDdlReader doc;
    EZ_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    ezUuid v0, v1;

    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToUuid(doc.FindElement("v0"), v0).Failed());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToUuid(doc.FindElement("v1"), v1).Succeeded());

    EZ_TEST_BOOL(v1 == ezUuid(12345678910, 10987654321));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezOpenDdlUtils::ConvertToAngle")
  {
    const char* szTestData = "\
Data $v1 { float { 45.23 } }\
";

    StringStream stream(szTestData);
    ezOpenDdlReader doc;
    EZ_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    ezAngle v0, v1;

    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToAngle(doc.FindElement("v0"), v0).Failed());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToAngle(doc.FindElement("v1"), v1).Succeeded());

    EZ_TEST_FLOAT(v1.GetRadian(), 45.23f, 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezOpenDdlUtils::ConvertToVariant")
  {
    const char* szTestData = "\
Color $v1 { float { 1, 0, 0.5 } }\
ColorGamma $v2 { unsigned_int8 { 128, 0, 32, 64 } }\
Time $v3 { float { 0.1 } }\
Vec2 $v4 { float { 0.1, 2 } }\
Vec3 $v5 { float { 0.1, 2, 3.2 } }\
Vec4 $v6 { float { 0.1, 2, 3.2, 44.5 } }\
Mat3 $v7 { float { 1, 2, 3, 4, 5, 6, 7, 8, 9 } }\
Mat4 $v8 { float { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 } }\
Transform $v9 { float { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 } }\
Quat $v10 { float { 0.1, 2, 3.2, 44.5 } }\
Uuid $v11 { unsigned_int64 { 12345678910, 10987654321 } }\
Angle $v12 { float { 45.23 } }\
";

    StringStream stream(szTestData);
    ezOpenDdlReader doc;
    EZ_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    ezVariant v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12;

    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToVariant(doc.FindElement("v0"), v0).Failed());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToVariant(doc.FindElement("v1"), v1).Succeeded());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToVariant(doc.FindElement("v2"), v2).Succeeded());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToVariant(doc.FindElement("v3"), v3).Succeeded());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToVariant(doc.FindElement("v4"), v4).Succeeded());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToVariant(doc.FindElement("v5"), v5).Succeeded());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToVariant(doc.FindElement("v6"), v6).Succeeded());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToVariant(doc.FindElement("v7"), v7).Succeeded());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToVariant(doc.FindElement("v8"), v8).Succeeded());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToVariant(doc.FindElement("v9"), v9).Succeeded());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToVariant(doc.FindElement("v10"), v10).Succeeded());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToVariant(doc.FindElement("v11"), v11).Succeeded());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToVariant(doc.FindElement("v12"), v12).Succeeded());

    EZ_TEST_BOOL(v1.IsA<ezColor>());
    EZ_TEST_BOOL(v2.IsA<ezColorGammaUB>());
    EZ_TEST_BOOL(v3.IsA<ezTime>());
    EZ_TEST_BOOL(v4.IsA<ezVec2>());
    EZ_TEST_BOOL(v5.IsA<ezVec3>());
    EZ_TEST_BOOL(v6.IsA<ezVec4>());
    EZ_TEST_BOOL(v7.IsA<ezMat3>());
    EZ_TEST_BOOL(v8.IsA<ezMat4>());
    EZ_TEST_BOOL(v9.IsA<ezTransform>());
    EZ_TEST_BOOL(v10.IsA<ezQuat>());
    EZ_TEST_BOOL(v11.IsA<ezUuid>());
    EZ_TEST_BOOL(v12.IsA<ezAngle>());

    EZ_TEST_BOOL(v1.Get<ezColor>() == ezColor(1, 0, 0.5));
    EZ_TEST_BOOL(v2.Get<ezColorGammaUB>() == ezColorGammaUB(128, 0, 32, 64));
    EZ_TEST_FLOAT(v3.Get<ezTime>().GetSeconds(), 0.1, 0.0001f);
    EZ_TEST_VEC2(v4.Get<ezVec2>(), ezVec2(0.1f, 2.0f), 0.0001f);
    EZ_TEST_VEC3(v5.Get<ezVec3>(), ezVec3(0.1f, 2.0f, 3.2f), 0.0001f);
    EZ_TEST_VEC4(v6.Get<ezVec4>(), ezVec4(0.1f, 2.0f, 3.2f, 44.5f), 0.0001f);
    EZ_TEST_BOOL(v7.Get<ezMat3>().IsEqual(ezMat3(1, 4, 7, 2, 5, 8, 3, 6, 9), 0.0001f));
    EZ_TEST_BOOL(v8.Get<ezMat4>().IsEqual(ezMat4(1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15, 4, 8, 12, 16), 0.0001f));
    EZ_TEST_BOOL(v9.Get<ezTransform>().m_qRotation == ezQuat(4, 5, 6, 7));
    EZ_TEST_VEC3(v9.Get<ezTransform>().m_vPosition, ezVec3(1, 2, 3), 0.0001f);
    EZ_TEST_VEC3(v9.Get<ezTransform>().m_vScale, ezVec3(8, 9, 10), 0.0001f);
    EZ_TEST_BOOL(v10.Get<ezQuat>() == ezQuat(0.1f, 2.0f, 3.2f, 44.5f));
    EZ_TEST_BOOL(v11.Get<ezUuid>() == ezUuid(12345678910, 10987654321));
    EZ_TEST_FLOAT(v12.Get<ezAngle>().GetRadian(), 45.23f, 0.0001f);


    /// \test Test primitive types in ezVariant
  }

  //////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "StoreColor")
  {
    StreamComparer sc("Color $v1{float{1,2,3,4}}\n");

    ezOpenDdlWriter js;
    js.SetFloatPrecisionMode(ezOpenDdlWriter::FloatPrecisionMode::Readable);
    js.SetOutputStream(&sc);

    ezOpenDdlUtils::StoreColor(js, ezColor(1, 2, 3, 4), "v1", true);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "StoreColorGamma")
  {
    StreamComparer sc("ColorGamma $v1{uint8{1,2,3,4}}\n");

    ezOpenDdlWriter js;
    js.SetOutputStream(&sc);

    ezOpenDdlUtils::StoreColorGamma(js, ezColorGammaUB(1, 2, 3, 4), "v1", true);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "StoreTime")
  {
    StreamComparer sc("Time $v1{double{2.3}}\n");

    ezOpenDdlWriter js;
    js.SetFloatPrecisionMode(ezOpenDdlWriter::FloatPrecisionMode::Readable);
    js.SetOutputStream(&sc);

    ezOpenDdlUtils::StoreTime(js, ezTime::Seconds(2.3), "v1", true);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "StoreVec2")
  {
    StreamComparer sc("Vec2 $v1{float{1,2}}\n");

    ezOpenDdlWriter js;
    js.SetFloatPrecisionMode(ezOpenDdlWriter::FloatPrecisionMode::Readable);
    js.SetOutputStream(&sc);

    ezOpenDdlUtils::StoreVec2(js, ezVec2(1, 2), "v1", true);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "StoreVec3")
  {
    StreamComparer sc("Vec3 $v1{float{1,2,3}}\n");

    ezOpenDdlWriter js;
    js.SetFloatPrecisionMode(ezOpenDdlWriter::FloatPrecisionMode::Readable);
    js.SetOutputStream(&sc);

    ezOpenDdlUtils::StoreVec3(js, ezVec3(1, 2, 3), "v1", true);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "StoreVec4")
  {
    StreamComparer sc("Vec4 $v1{float{1,2,3,4}}\n");

    ezOpenDdlWriter js;
    js.SetFloatPrecisionMode(ezOpenDdlWriter::FloatPrecisionMode::Readable);
    js.SetOutputStream(&sc);

    ezOpenDdlUtils::StoreVec4(js, ezVec4(1, 2, 3, 4), "v1", true);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "StoreMat3")
  {
    StreamComparer sc("Mat3 $v1{float{1,4,7,2,5,8,3,6,9}}\n");

    ezOpenDdlWriter js;
    js.SetFloatPrecisionMode(ezOpenDdlWriter::FloatPrecisionMode::Readable);
    js.SetOutputStream(&sc);

    ezOpenDdlUtils::StoreMat3(js, ezMat3(1, 2, 3, 4, 5, 6, 7, 8, 9), "v1", true);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "StoreMat4")
  {
    StreamComparer sc("Mat4 $v1{float{1,5,9,13,2,6,10,14,3,7,11,15,4,8,12,16}}\n");

    ezOpenDdlWriter js;
    js.SetFloatPrecisionMode(ezOpenDdlWriter::FloatPrecisionMode::Readable);
    js.SetOutputStream(&sc);

    ezOpenDdlUtils::StoreMat4(js, ezMat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), "v1", true);
  }

  // EZ_TEST_BLOCK(ezTestBlock::Enabled, "StoreTransform")
  //{
  //  StreamComparer sc("Transform $v1{float{1,4,7,2,5,8,3,6,9,10}}\n");

  //  ezOpenDdlWriter js;
  //  js.SetFloatPrecisionMode(ezOpenDdlWriter::FloatPrecisionMode::Readable);
  //  js.SetOutputStream(&sc);

  //  ezOpenDdlUtils::StoreTransform(js, ezTransform(ezVec3(10, 20, 30), ezMat3(1, 2, 3, 4, 5, 6, 7, 8, 9)), "v1", true);
  //}

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "StoreQuat")
  {
    StreamComparer sc("Quat $v1{float{1,2,3,4}}\n");

    ezOpenDdlWriter js;
    js.SetFloatPrecisionMode(ezOpenDdlWriter::FloatPrecisionMode::Readable);
    js.SetOutputStream(&sc);

    ezOpenDdlUtils::StoreQuat(js, ezQuat(1, 2, 3, 4), "v1", true);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "StoreUuid")
  {
    StreamComparer sc("Uuid $v1{u4{12345678910,10987654321}}\n");

    ezOpenDdlWriter js;
    js.SetPrimitiveTypeStringMode(ezOpenDdlWriter::TypeStringMode::Shortest);
    js.SetOutputStream(&sc);

    ezOpenDdlUtils::StoreUuid(js, ezUuid(12345678910, 10987654321), "v1", true);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "StoreAngle")
  {
    StreamComparer sc("Angle $v1{float{2.3}}\n");

    ezOpenDdlWriter js;
    js.SetFloatPrecisionMode(ezOpenDdlWriter::FloatPrecisionMode::Readable);
    js.SetOutputStream(&sc);

    ezOpenDdlUtils::StoreAngle(js, ezAngle::Radian(2.3f), "v1", true);
  }

  // this test also covers all the types that Variant supports
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "StoreVariant")
  {
    ezUInt8 rawData[sizeof(float) * 16]; // enough for mat4

    for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(rawData); ++i)
      rawData[i] = i + 1;
    rawData[EZ_ARRAY_SIZE(rawData) - 1] = 0; // string terminator

    for (ezUInt32 t = ezVariant::Type::FirstStandardType + 1; t < ezVariant::Type::LastStandardType; ++t)
    {
      const ezVariant var = CreateVariant((ezVariant::Type::Enum)t, rawData);

      ezMemoryStreamStorage storage;
      ezMemoryStreamWriter writer(&storage);
      ezMemoryStreamReader reader(&storage);

      ezOpenDdlWriter js;
      js.SetFloatPrecisionMode(ezOpenDdlWriter::FloatPrecisionMode::Exact);
      js.SetOutputStream(&writer);

      ezOpenDdlUtils::StoreVariant(js, var, "bla");

      ezOpenDdlReader doc;
      EZ_TEST_BOOL(doc.ParseDocument(reader).Succeeded());

      const auto pVarElem = doc.GetRootElement()->FindChild("bla");

      ezVariant result;
      ezOpenDdlUtils::ConvertToVariant(pVarElem, result);

      EZ_TEST_BOOL(var == result);
    }
  }
}

static ezVariant CreateVariant(ezVariant::Type::Enum t, const void* data)
{
  switch (t)
  {
    case ezVariant::Type::Bool:
      return ezVariant(*((bool*)data));
    case ezVariant::Type::Int8:
      return ezVariant(*((ezInt8*)data));
    case ezVariant::Type::UInt8:
      return ezVariant(*((ezUInt8*)data));
    case ezVariant::Type::Int16:
      return ezVariant(*((ezInt16*)data));
    case ezVariant::Type::UInt16:
      return ezVariant(*((ezUInt16*)data));
    case ezVariant::Type::Int32:
      return ezVariant(*((ezInt32*)data));
    case ezVariant::Type::UInt32:
      return ezVariant(*((ezUInt32*)data));
    case ezVariant::Type::Int64:
      return ezVariant(*((ezInt64*)data));
    case ezVariant::Type::UInt64:
      return ezVariant(*((ezUInt64*)data));
    case ezVariant::Type::Float:
      return ezVariant(*((float*)data));
    case ezVariant::Type::Double:
      return ezVariant(*((double*)data));
    case ezVariant::Type::Color:
      return ezVariant(*((ezColor*)data));
    case ezVariant::Type::Vector2:
      return ezVariant(*((ezVec2*)data));
    case ezVariant::Type::Vector3:
      return ezVariant(*((ezVec3*)data));
    case ezVariant::Type::Vector4:
      return ezVariant(*((ezVec4*)data));
    case ezVariant::Type::Vector2I:
      return ezVariant(*((ezVec2I32*)data));
    case ezVariant::Type::Vector3I:
      return ezVariant(*((ezVec3I32*)data));
    case ezVariant::Type::Vector4I:
      return ezVariant(*((ezVec4I32*)data));
    case ezVariant::Type::Vector2U:
      return ezVariant(*((ezVec2U32*)data));
    case ezVariant::Type::Vector3U:
      return ezVariant(*((ezVec3U32*)data));
    case ezVariant::Type::Vector4U:
      return ezVariant(*((ezVec4U32*)data));
    case ezVariant::Type::Quaternion:
      return ezVariant(*((ezQuat*)data));
    case ezVariant::Type::Matrix3:
      return ezVariant(*((ezMat3*)data));
    case ezVariant::Type::Matrix4:
      return ezVariant(*((ezMat4*)data));
    case ezVariant::Type::Transform:
      return ezVariant(*((ezTransform*)data));
    case ezVariant::Type::String:
    case ezVariant::Type::StringView: // string views are stored as full strings as well
      return ezVariant((const char*)data);
    case ezVariant::Type::DataBuffer:
    {
      ezDataBuffer db;
      db.SetCountUninitialized(sizeof(float) * 16);
      for (ezUInt32 i = 0; i < db.GetCount(); ++i)
        db[i] = ((ezUInt8*)data)[i];

      return ezVariant(db);
    }
    case ezVariant::Type::Time:
      return ezVariant(*((ezTime*)data));
    case ezVariant::Type::Uuid:
      return ezVariant(*((ezUuid*)data));
    case ezVariant::Type::Angle:
      return ezVariant(*((ezAngle*)data));
    case ezVariant::Type::ColorGamma:
      return ezVariant(*((ezColorGammaUB*)data));

    default:
      EZ_REPORT_FAILURE("Unknown type");
  }

  return ezVariant();
}
