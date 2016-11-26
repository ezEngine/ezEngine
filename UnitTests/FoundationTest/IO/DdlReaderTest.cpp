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
        const ezOpenDdlReaderElement* pChild2 = pElement->FindChild(sNameCopy);

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
  writer.SetPrimitiveTypeStringMode(ezOpenDdlWriter::TypeStringMode::Compliant);
  writer.SetFloatPrecisionMode(ezOpenDdlWriter::FloatPrecisionMode::Readable);

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
}
