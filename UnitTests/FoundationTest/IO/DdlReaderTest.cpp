#include <PCH.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Strings/StringUtils.h>
#include <FoundationTest/IO/JSONTestHelpers.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OpenDdlUtils.h>

// Since ezOpenDdlReader is implemented by deriving from ezOpenDdlParser, this tests both classes

static void WriteObjectToDDL(const ezOpenDdlReaderElement* pElement, ezOpenDdlWriter& writer)
{
  if (pElement->HasName())
  {
    EZ_TEST_BOOL(!ezStringUtils::IsNullOrEmpty(pElement->GetName()));
  }

  if (pElement->IsCustomType())
  {
    writer.BeginObject(pElement->GetCustomType(), pElement->GetName(), pElement->IsNameGlobal());

    ezUInt32 uiChildren = 0;
    auto pChild = pElement->GetFirstChild();
    while (pChild)
    {
      ++uiChildren;

      if (pChild->HasName())
      {
        ezString sNameCopy = pChild->GetName();
        const ezOpenDdlReaderElement* pChild2 = pElement->FindChildElement(sNameCopy);

        EZ_TEST_BOOL(pChild == pChild2);
      }

      WriteObjectToDDL(pChild, writer);
      pChild = pChild->GetSibling();
    }

    writer.EndObject();

    EZ_TEST_INT(uiChildren, pElement->GetNumChildObjects());
  }
  else
  {
    const ezOpenDdlPrimitiveType type = pElement->GetPrimitivesType();

    writer.BeginPrimitiveList(type, pElement->GetName(), pElement->IsNameGlobal());

    switch (type)
    {
    case ezOpenDdlPrimitiveType::Bool:
      writer.WriteBool(pElement->GetPrimitivesBool(), pElement->GetNumPrimitives());
      break;

    case ezOpenDdlPrimitiveType::Int8:
      writer.WriteInt8(pElement->GetPrimitivesInt8(), pElement->GetNumPrimitives());
      break;

    case ezOpenDdlPrimitiveType::Int16:
      writer.WriteInt16(pElement->GetPrimitivesInt16(), pElement->GetNumPrimitives());
      break;

    case ezOpenDdlPrimitiveType::Int32:
      writer.WriteInt32(pElement->GetPrimitivesInt32(), pElement->GetNumPrimitives());
      break;

    case ezOpenDdlPrimitiveType::Int64:
      writer.WriteInt64(pElement->GetPrimitivesInt64(), pElement->GetNumPrimitives());
      break;

    case ezOpenDdlPrimitiveType::UInt8:
      writer.WriteUInt8(pElement->GetPrimitivesUInt8(), pElement->GetNumPrimitives());
      break;

    case ezOpenDdlPrimitiveType::UInt16:
      writer.WriteUInt16(pElement->GetPrimitivesUInt16(), pElement->GetNumPrimitives());
      break;

    case ezOpenDdlPrimitiveType::UInt32:
      writer.WriteUInt32(pElement->GetPrimitivesUInt32(), pElement->GetNumPrimitives());
      break;

    case ezOpenDdlPrimitiveType::UInt64:
      writer.WriteUInt64(pElement->GetPrimitivesUInt64(), pElement->GetNumPrimitives());
      break;

    case ezOpenDdlPrimitiveType::Float:
      writer.WriteFloat(pElement->GetPrimitivesFloat(), pElement->GetNumPrimitives());
      break;

    case ezOpenDdlPrimitiveType::Double:
      writer.WriteDouble(pElement->GetPrimitivesDouble(), pElement->GetNumPrimitives());
      break;

    case ezOpenDdlPrimitiveType::String:
      {
        for (ezUInt32 i = 0; i < pElement->GetNumPrimitives(); ++i)
        {
          writer.WriteString(pElement->GetPrimitivesString()[i]);
        }
      }
      break;

    case ezOpenDdlPrimitiveType::Custom:
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      break;
    }

    writer.EndPrimitiveList();
  }
}

static void WriteToDDL(const ezOpenDdlReader& doc, ezStreamWriter& output)
{
  ezOpenDdlWriter writer;
  writer.SetOutputStream(&output);

  const auto* pRoot = doc.GetRootElement();
  EZ_ASSERT_DEV(pRoot != nullptr, "Invalid root");

  if (pRoot == nullptr)
    return;

  auto pChild = pRoot->GetFirstChild();
  while (pChild)
  {
    WriteObjectToDDL(pChild, writer);
    pChild = pChild->GetSibling();
  }
}

static void WriteToString(const ezOpenDdlReader& doc, ezStringBuilder& string)
{
  ezMemoryStreamStorage storage;
  ezMemoryStreamWriter writer(&storage);

  WriteToDDL(doc, writer);

  ezUInt8 term = 0;
  writer.WriteBytes(&term, 1);
  string = (const char*)storage.GetData();
}

static void TestEqual(const char* original, const char* recreation)
{
  ezUInt32 uiChar = 0;

  do
  {
    const ezUInt8 cOrg = original[uiChar];
    const ezUInt8 cAlt = recreation[uiChar];

    if (cOrg != cAlt)
    {
      EZ_TEST_FAILURE("String compare failed", "DDL Original and recreation don't match at character %u ('%c' -> '%c')", uiChar, cOrg, cAlt);
      return;
    }

    ++uiChar;
  }
  while (original[uiChar - 1] != '\0');
}

// These functions test the reader by doing a round trip from string -> reader -> writer -> string and then comparing the string to the original
// Therefore the original must be formatted exactly as the writer would format it (mostly regarding indentation, newlines, spaces and floats)
// and may not contain things that get removed (ie. comments)
static void TestDoc(const ezOpenDdlReader& doc, const char* szOriginal)
{
  ezStringBuilder recreation;
  WriteToString(doc, recreation);

  TestEqual(szOriginal, recreation);
}

EZ_CREATE_SIMPLE_TEST(IO, DdlReader)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Basics and Comments")
  {
    const char* szTestData =
"Node{\
  Name{ string{ \"ConstantColor\" } }\
\
  OutputPins\
  {\
    float %MyFloats{ 1.2, 3, 4e1, .5, 6_0, .7e-2 } \
    double$MyDoubles{1.2,3,4e1,.5,6_0,.7e-2} \
    int8{0,1/**/27,,  ,128  , -127/*comment*/ , -128,  -129}\
    string{ \"float4\" }\
    bool{ true, false , true,, false }\
  }\
\
  Properties\
  {\
    Property\
    {\
      Name{ string{ \"Color\" } }\
      Type{ string{ \"color\" } }\
    }\
  }\
}\
// some comment \
";

    StringStream stream(szTestData);

    ezOpenDdlReader doc;
    EZ_TEST_BOOL(doc.ParseDocument(stream).Succeeded());
    EZ_TEST_BOOL(!doc.HadFatalParsingError());

    auto pRoot = doc.GetRootElement();
    EZ_TEST_INT(pRoot->GetNumChildObjects(), 1);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Structure")
  {
    const char* szTestData =
"Node\n\
{\n\
  Name\n\
  {\n\
    string { \"ConstantColor\" }\n\
  }\n\
  OutputPins\n\
  {\n\
    float %MyFloats { 1.2,3,40,0.5,60 }\n\
    double $MyDoubles { 1.2,3,40,0.5,60 }\n\
    int8 { 0,127,32,-127,-127 }\n\
    string { \"float4\" }\n\
    bool { true,false,true,false }\n\
  }\n\
  Properties\n\
  {\n\
    Property\n\
    {\n\
      Name\n\
      {\n\
        string { \"Color\" }\n\
      }\n\
      Type\n\
      {\n\
        string { \"color\" }\n\
      }\n\
    }\n\
  }\n\
}\n\
";

    StringStream stream(szTestData);

    ezOpenDdlReader doc;
    EZ_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    auto pElement = doc.FindElement("MyDoubles");
    EZ_TEST_BOOL(pElement != nullptr);
    if (pElement)
    {
      EZ_TEST_STRING(pElement->GetName(), "MyDoubles");
    }

    EZ_TEST_BOOL(doc.FindElement("MyFloats") == nullptr);
    EZ_TEST_BOOL(doc.FindElement("Node") == nullptr);

    TestDoc(doc, szTestData);
    EZ_TEST_BOOL(!doc.HadFatalParsingError());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "All Primitives")
  {
    const char* szTestData = "\
bool { true,false,true,true,false }\n\
string { \"s1\",\"\\n\\t\\r\" }\n\
float { 0,1.1,-3,23.42 }\n\
double { 0,1.1,-3,23.42 }\n\
int8 { 0,12,34,56,78,109,127,-14,-56,-127 }\n\
int16 { 0,102,3040,5600,7008,109,10207,-1004,-5060,-10207 }\n\
int32 { 0,100002,300040,56000000,700008,1000009,100000207,-100000004,-506000000,-1020700000 }\n\
int64 { 0,100002111,300040222,560000003333,70000844444,1000009555555,100000207666666,-1000000047777777,-50600000008888888,-102070000099999 }\n\
unsigned_int8 { 0,12,34,56,78,109,127,255,156,207 }\n\
unsigned_int16 { 0,102,3040,56000,7008,109,10207,40004,50600,10207 }\n\
unsigned_int32 { 0,100002,300040,56000000,700008,1000009,100000207,100000004,2000001000,1020700000 }\n\
unsigned_int64 { 0,100002111,300040222,560000003333,70000844444,1000009555555,100000207666666,1000000047777777,50600000008888888,102070000099999 }\n\
";

    StringStream stream(szTestData);

    ezOpenDdlReader doc;
    EZ_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    TestDoc(doc, szTestData);
    EZ_TEST_BOOL(!doc.HadFatalParsingError());
  }

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
Group $v1 { float { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 } }\
";

    StringStream stream(szTestData);
    ezOpenDdlReader doc;
    EZ_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    ezTransform v0, v1;

    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToTransform(doc.FindElement("v0"), v0).Failed());
    EZ_TEST_BOOL(ezOpenDdlUtils::ConvertToTransform(doc.FindElement("v1"), v1).Succeeded());

    EZ_TEST_BOOL(v1.m_Rotation.IsEqual(ezMat3(1, 4, 7, 2, 5, 8, 3, 6, 9), 0.0001f));
    EZ_TEST_VEC3(v1.m_vPosition, ezVec3(10, 11, 12), 0.0001f);
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

    EZ_TEST_FLOAT(v1.GetDegree(), 45.23f, 0.0001f);
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
Transform $v9 { float { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 } }\
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
    EZ_TEST_BOOL(v9.Get<ezTransform>().m_Rotation.IsEqual(ezMat3(1, 4, 7, 2, 5, 8, 3, 6, 9), 0.0001f));
    EZ_TEST_VEC3(v9.Get<ezTransform>().m_vPosition, ezVec3(10, 11, 12), 0.0001f);
    EZ_TEST_BOOL(v10.Get<ezQuat>() == ezQuat(0.1f, 2.0f, 3.2f, 44.5f));
    EZ_TEST_BOOL(v11.Get<ezUuid>() == ezUuid(12345678910, 10987654321));
    EZ_TEST_FLOAT(v12.Get<ezAngle>().GetDegree(), 45.23f, 0.0001f);
  }
}
