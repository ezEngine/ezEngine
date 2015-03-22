#include <PCH.h>
#include <Foundation/IO/JSONParser.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Strings/StringUtils.h>
#include <FoundationTest/IO/JSONTestHelpers.h>

enum ParseFunction
{
  Variable,
  ValueNULL,
  ValueString,
  ValueDouble,
  ValueBool,
  BeginObject,
  EndObject,
  BeginArray,
  EndArray,
};

struct ParseResult
{
  ParseFunction m_Function;

  union
  {
    const char* m_szValue;
    bool m_bValue;
    double m_fValue;
  };

  ParseResult(ParseFunction f)
  {
    m_Function = f;
  }

  ParseResult(ParseFunction f, const char* v)
  {
    m_Function = f;
    m_szValue = v;
  }

  ParseResult(const char* v)
  {
    m_Function = ValueString;
    m_szValue = v;
  }

  ParseResult(bool v)
  {
    m_Function = ValueBool;
    m_bValue = v;
  }

  ParseResult(double v)
  {
    m_Function = ValueDouble;
    m_fValue = v;
  }
};

class TestReader : public ezJSONParser
{
private:
  ezDeque<ParseResult> m_Results;

public:
  TestReader()
  {
    m_iExpectedParsingErrors = 0;
    m_bSkipObject = false;
    m_bSkipArray = false;
  }

  ~TestReader()
  {
    EZ_TEST_INT(m_iExpectedParsingErrors, 0);
    EZ_TEST_INT(m_Results.GetCount(), 0);
  }

  void ParseStream(ezStreamReaderBase& stream)
  {
    SetInputStream(stream);
    ParseAll();
  }

  void Add(ParseResult pr)
  {
    m_Results.PushBack(pr);
  }

  virtual bool OnVariable(const char* szVarName) override
  {
    EZ_TEST_BOOL(!m_Results.IsEmpty());
    EZ_TEST_BOOL(m_Results.PeekFront().m_Function == Variable);
    EZ_TEST_STRING(m_Results.PeekFront().m_szValue, szVarName);

    m_Results.PopFront();

    m_bSkipObject = ezStringUtils::IsEqual(szVarName, "skip_obj");
    m_bSkipArray = ezStringUtils::IsEqual(szVarName, "skip_array");

    return !ezStringUtils::IsEqual(szVarName, "skip_var");
  }

  virtual void OnReadValue(const char* szValue) override
  {
    EZ_TEST_BOOL(!m_Results.IsEmpty());
    EZ_TEST_BOOL(m_Results.PeekFront().m_Function == ValueString);
    EZ_TEST_STRING(m_Results.PeekFront().m_szValue, szValue);

    m_Results.PopFront();
  }

  virtual void OnReadValue(double fValue) override
  {
    EZ_TEST_BOOL(!m_Results.IsEmpty());
    EZ_TEST_BOOL(m_Results.PeekFront().m_Function == ValueDouble);
    EZ_TEST_DOUBLE(m_Results.PeekFront().m_fValue, fValue, 0.0001);

    m_Results.PopFront();
  }

  virtual void OnReadValue(bool bValue) override
  {
    EZ_TEST_BOOL(!m_Results.IsEmpty());
    EZ_TEST_BOOL(m_Results.PeekFront().m_Function == ValueBool);
    EZ_TEST_BOOL(m_Results.PeekFront().m_bValue == bValue);

    m_Results.PopFront();
  }

  virtual void OnReadValueNULL() override
  {
    EZ_TEST_BOOL(!m_Results.IsEmpty());
    EZ_TEST_BOOL(m_Results.PeekFront().m_Function == ValueNULL);

    m_Results.PopFront();
  }

  virtual void OnBeginObject() override
  {
    EZ_TEST_BOOL(!m_Results.IsEmpty());
    EZ_TEST_BOOL(m_Results.PeekFront().m_Function == BeginObject);

    m_Results.PopFront();

    if (m_bSkipObject)
      SkipObject();
  }

  virtual void OnEndObject() override
  {
    EZ_TEST_BOOL(!m_Results.IsEmpty());
    EZ_TEST_BOOL(m_Results.PeekFront().m_Function == EndObject);

    m_Results.PopFront();
  }

  virtual void OnBeginArray() override
  {
    EZ_TEST_BOOL(!m_Results.IsEmpty());
    EZ_TEST_BOOL(m_Results.PeekFront().m_Function == BeginArray);

    m_Results.PopFront();

    if (m_bSkipArray)
      SkipArray();
  }

  virtual void OnEndArray() override
  {
    EZ_TEST_BOOL(!m_Results.IsEmpty());
    EZ_TEST_BOOL(m_Results.PeekFront().m_Function == EndArray);

    m_Results.PopFront();
  }

  ezInt32 m_iExpectedParsingErrors;

  virtual void OnParsingError(const char* szMessage, bool bFatal, ezUInt32 uiLine, ezUInt32 uiColumn) override
  {
    --m_iExpectedParsingErrors;

    if (m_iExpectedParsingErrors >= 0)
      return;

    EZ_TEST_FAILURE("JSON Parsing Error", "(%u, %u): %s", uiLine, uiColumn, szMessage);
  }

  bool m_bSkipObject;
  bool m_bSkipArray;
};

EZ_CREATE_SIMPLE_TEST(IO, JSONParser)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "All Features")
  {
    const char* szTestData =
"{\n\
\"myarray2\":[\"\",2.2],\n\
\"myarray\" : [1, 2.2, 3.3, false, \"ende\" ],\n\
\"String\"/**/ : \"testvalue\",\n\
\"double\"/***/ : 43.56,\n\
\"float\" :/**//*a*/ 64.720001,\n\
\"bool\" : true,\n\
\"int\" : 23,\n\
\"MyNull\" : null,\n\
\"object\" :\n\
/* totally \n weird \t stuff \n\n\n going on here // thats a line comment \n */ \
// more line comments \n\n\n\n\
{\n\
  \"variable in object\" : \"bla\",\n\
    \"Subobject\" :\n\
  {\n\
    \"variable in subobject\" : \"blub\",\n\
      \"array in sub\" : [\n\
    {\n\
      \"obj var\" : 234\n\
            /*stuff ] */ \
    },\n\
    {\n\
      \"obj var 2\" : -235\n//breakingcomment\n\
    }, true, 4, false ]\n\
  }\n\
},\n\
\"test\" : \"text\"\n\
}";

    StringStream stream(szTestData);

    TestReader reader;

    reader.Add(ParseResult(BeginObject));

      reader.Add(ParseResult(Variable, "myarray2"));
      reader.Add(ParseResult(BeginArray));
        reader.Add(ParseResult(""));
        reader.Add(ParseResult(2.2));
      reader.Add(ParseResult(EndArray));

      reader.Add(ParseResult(Variable, "myarray"));
      reader.Add(ParseResult(BeginArray));
        reader.Add(ParseResult(1.0));
        reader.Add(ParseResult(2.2));
        reader.Add(ParseResult(3.3));
        reader.Add(ParseResult(false));
        reader.Add(ParseResult("ende"));
      reader.Add(ParseResult(EndArray));

      reader.Add(ParseResult(Variable, "String"));
      reader.Add(ParseResult("testvalue"));

      reader.Add(ParseResult(Variable, "double"));
      reader.Add(ParseResult(43.56));

      reader.Add(ParseResult(Variable, "float"));
      reader.Add(ParseResult(64.720001));

      reader.Add(ParseResult(Variable, "bool"));
      reader.Add(ParseResult(true));

      reader.Add(ParseResult(Variable, "int"));
      reader.Add(ParseResult(23.0));

      reader.Add(ParseResult(Variable, "MyNull"));
      reader.Add(ParseResult(ValueNULL));

      reader.Add(ParseResult(Variable, "object"));
      reader.Add(ParseResult(BeginObject));

        reader.Add(ParseResult(Variable, "variable in object"));
        reader.Add(ParseResult("bla"));

        reader.Add(ParseResult(Variable, "Subobject"));
        reader.Add(ParseResult(BeginObject));

          reader.Add(ParseResult(Variable, "variable in subobject"));
          reader.Add(ParseResult("blub"));

          reader.Add(ParseResult(Variable, "array in sub"));
          reader.Add(ParseResult(BeginArray));

            reader.Add(ParseResult(BeginObject));

              reader.Add(ParseResult(Variable, "obj var"));
              reader.Add(ParseResult(234.0));

            reader.Add(ParseResult(EndObject));

            reader.Add(ParseResult(BeginObject));

              reader.Add(ParseResult(Variable, "obj var 2"));
              reader.Add(ParseResult(-235.0));

            reader.Add(ParseResult(EndObject));

            reader.Add(ParseResult(true));
            reader.Add(ParseResult(4.0));
            reader.Add(ParseResult(false));

          reader.Add(ParseResult(EndArray));

        reader.Add(ParseResult(EndObject));

      reader.Add(ParseResult(EndObject));

      reader.Add(ParseResult(Variable, "test"));
      reader.Add(ParseResult("text"));

    reader.Add(ParseResult(EndObject));

    reader.ParseStream(stream);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Empty Document (string")
  {
    const char* szTestData = "";

    StringStream stream(szTestData);

    TestReader reader;

    reader.ParseStream(stream);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Empty Document (Whitespace)")
  {
    const char* szTestData = " \n  \t ";

    StringStream stream(szTestData);

    TestReader reader;

    reader.ParseStream(stream);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Empty Document")
  {
    const char* szTestData = "{}";

    StringStream stream(szTestData);

    TestReader reader;

    reader.Add(ParseResult(BeginObject));
    reader.Add(ParseResult(EndObject));

    reader.ParseStream(stream);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Two Empty Documents")
  {
    const char* szTestData = "{}{}";

    StringStream stream(szTestData);

    TestReader reader;

    // only the first will be read
    reader.Add(ParseResult(BeginObject));
    reader.Add(ParseResult(EndObject));

    reader.ParseStream(stream);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "No Whitespace")
  {
    // I want C++ 11 raw string literals
    const char* szTestData = "{\"a\":4,\"b\":true,\"c\":\"\",\"d\":\"v\"}";

    StringStream stream(szTestData);

    TestReader reader;

    reader.Add(ParseResult(BeginObject));

      reader.Add(ParseResult(Variable, "a"));
      reader.Add(ParseResult(4.0));

      reader.Add(ParseResult(Variable, "b"));
      reader.Add(ParseResult(true));

      reader.Add(ParseResult(Variable, "c"));
      reader.Add(ParseResult(""));

      reader.Add(ParseResult(Variable, "d"));
      reader.Add(ParseResult("v"));

    reader.Add(ParseResult(EndObject));

    reader.ParseStream(stream);

    EZ_TEST_INT(reader.m_iExpectedParsingErrors, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Empty Blocks")
  {
    // I want C++ 11 raw string literals
    const char* szTestData = "{\"a\":{},\"b\":[],\"c\":[{}],\"d\":{}}";

    StringStream stream(szTestData);

    TestReader reader;

    reader.Add(ParseResult(BeginObject));

      reader.Add(ParseResult(Variable, "a"));
      reader.Add(ParseResult(BeginObject));
      reader.Add(ParseResult(EndObject));

      reader.Add(ParseResult(Variable, "b"));
      reader.Add(ParseResult(BeginArray));
      reader.Add(ParseResult(EndArray));

      reader.Add(ParseResult(Variable, "c"));
      reader.Add(ParseResult(BeginArray));
        reader.Add(ParseResult(BeginObject));
        reader.Add(ParseResult(EndObject));
      reader.Add(ParseResult(EndArray));

      reader.Add(ParseResult(Variable, "d"));
      reader.Add(ParseResult(BeginObject));
      reader.Add(ParseResult(EndObject));

    reader.Add(ParseResult(EndObject));

    reader.ParseStream(stream);

    EZ_TEST_INT(reader.m_iExpectedParsingErrors, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Superfluous separators")
  {
    // I want C++ 11 raw string literals

    // allow superfluous commas in objects
    // allow ONE superfluous comma at the end of arrays (more will fail)
    const char* szTestData = "{\"a\":{,},,\"b\":[3.],\"c\":[.3,],\"d\":{},}//the end is near (here somewhere)";

    StringStream stream(szTestData);

    TestReader reader;

    reader.Add(ParseResult(BeginObject));

      reader.Add(ParseResult(Variable, "a"));
      reader.Add(ParseResult(BeginObject));
      reader.Add(ParseResult(EndObject));

      reader.Add(ParseResult(Variable, "b"));
      reader.Add(ParseResult(BeginArray));
        reader.Add(ParseResult(3.0));
      reader.Add(ParseResult(EndArray));

      reader.Add(ParseResult(Variable, "c"));
      reader.Add(ParseResult(BeginArray));
        reader.Add(ParseResult(0.3));
      reader.Add(ParseResult(EndArray));

      reader.Add(ParseResult(Variable, "d"));
      reader.Add(ParseResult(BeginObject));
      reader.Add(ParseResult(EndObject));

    reader.Add(ParseResult(EndObject));

    reader.ParseStream(stream);

    EZ_TEST_INT(reader.m_iExpectedParsingErrors, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Too many commas in array")
  {
    // I want C++ 11 raw string literals
    const char* szTestData = "{\"a\":[,]/**/}";

    StringStream stream(szTestData);

    TestReader reader;
    reader.m_iExpectedParsingErrors = 1;

    reader.Add(ParseResult(BeginObject));

      reader.Add(ParseResult(Variable, "a"));
      reader.Add(ParseResult(BeginArray));
    //  reader.Add(ParseResult(EndArray)); // not reached anymore due to syntax error

    //reader.Add(ParseResult(EndObject));

    reader.ParseStream(stream);

    EZ_TEST_INT(reader.m_iExpectedParsingErrors, 0); // must fail to parse
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Comments")
  {
    // I want C++ 11 raw string literals
    const char* szTestData = "{\"a\":tr/**/u/*\n*//**/e/* */, \"b\":234/* adf */56//78\n}";

    StringStream stream(szTestData);

    TestReader reader;

    reader.Add(ParseResult(BeginObject));

      reader.Add(ParseResult(Variable, "a"));
      reader.Add(ParseResult(true));

      reader.Add(ParseResult(Variable, "b"));
      reader.Add(ParseResult(23456.0));

    reader.Add(ParseResult(EndObject));

    reader.ParseStream(stream);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Skip Object")
  {
    // I want C++ 11 raw string literals
    const char* szTestData = "{ \"skip_obj\" : { \"a\" : 1, \"b\" : 2, \"c\" : [ { }, { \"e\" : { } } ] }, \"d\" : 3, \"skip_obj\" : { } }";

    StringStream stream(szTestData);

    TestReader reader;

    reader.Add(ParseResult(BeginObject));

    reader.Add(ParseResult(Variable, "skip_obj"));
    reader.Add(ParseResult(BeginObject));
    // skip here

    reader.Add(ParseResult(Variable, "d"));
    reader.Add(ParseResult(3.0));

    reader.Add(ParseResult(Variable, "skip_obj"));
    reader.Add(ParseResult(BeginObject));
    // skip here

    reader.Add(ParseResult(EndObject));

    reader.ParseStream(stream);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Skip Array")
  {
    // I want C++ 11 raw string literals
    const char* szTestData = "{ \"skip_array\" : [ \"a\",  1, \"b\",  2, \"c\", [ { }, { \"e\" : { } } ] ], \"d\" : 3, \"skip_array\" : [ ] }";

    StringStream stream(szTestData);

    TestReader reader;

    reader.Add(ParseResult(BeginObject));

    reader.Add(ParseResult(Variable, "skip_array"));
    reader.Add(ParseResult(BeginArray));
    // skip here

    reader.Add(ParseResult(Variable, "d"));
    reader.Add(ParseResult(3.0));

    reader.Add(ParseResult(Variable, "skip_array"));
    reader.Add(ParseResult(BeginArray));
    // skip here

    reader.Add(ParseResult(EndObject));

    reader.ParseStream(stream);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Skip Variable")
  {
    // I want C++ 11 raw string literals
    const char* szTestData = "{ \"skip_var\" : { \"a\" : 1, \"b\" : 2, \"c\" : [ { }, { \"e\" : { } } ] }, \"d\" : 3, \"f\" : { \"g\" : 4, \"skip_var\" : [ { }, 3, [], true ], \"h\" : 5} }";

    StringStream stream(szTestData);

    TestReader reader;

    reader.Add(ParseResult(BeginObject));

      reader.Add(ParseResult(Variable, "skip_var"));
      // skip here

      reader.Add(ParseResult(Variable, "d"));
      reader.Add(ParseResult(3.0));

      reader.Add(ParseResult(Variable, "f"));
      reader.Add(ParseResult(BeginObject));

        reader.Add(ParseResult(Variable, "g"));
        reader.Add(ParseResult(4.0));

        reader.Add(ParseResult(Variable, "skip_var"));
        // skip here

        reader.Add(ParseResult(Variable, "h"));
        reader.Add(ParseResult(5.0));

      reader.Add(ParseResult(EndObject));

    reader.Add(ParseResult(EndObject));

    reader.ParseStream(stream);
  }
}
