#include <PCH.h>
#include <Foundation/IO/JSONReader.h>

class StringStream : public ezStreamReaderBase
{
public:

  StringStream(const void* pData)
  {
    m_pData = pData;
    m_uiLength = ezStringUtils::GetStringElementCount((const char*) pData);
  }

  virtual ezUInt64 ReadBytes(void* pReadBuffer, ezUInt64 uiBytesToRead)
  {
    uiBytesToRead = ezMath::Min(uiBytesToRead, m_uiLength);
    m_uiLength -= uiBytesToRead;

    if (uiBytesToRead > 0)
    {
      ezMemoryUtils::Copy((ezUInt8*) pReadBuffer, (ezUInt8*) m_pData, (size_t) uiBytesToRead);
      m_pData = ezMemoryUtils::AddByteOffsetConst(m_pData, (ptrdiff_t) uiBytesToRead);
    }

    return uiBytesToRead;
  }

private:
  const void* m_pData;
  ezUInt64 m_uiLength;
};

void TraverseTree(const ezVariant& var, ezDeque<ezString>& Compare)
{
  switch (var.GetType())
  {
  case ezVariant::Type::VariantDictionary:
    {
      EZ_TEST_STRING(Compare.PeekFront().GetData(), "<object>");
      Compare.PopFront();

      const ezVariantDictionary& vd = var.Get<ezVariantDictionary>();

      for (auto it = vd.GetIterator(); it.IsValid(); ++it)
      {
        EZ_TEST_STRING(Compare.PeekFront().GetData(), it.Key().GetData());
        Compare.PopFront();

        TraverseTree(it.Value(), Compare);
      }

      EZ_TEST_STRING(Compare.PeekFront().GetData(), "</object>");
      Compare.PopFront();
    }
    break;

  case ezVariant::Type::VariantArray:
    {
      EZ_TEST_STRING(Compare.PeekFront().GetData(), "<array>");
      Compare.PopFront();

      const ezVariantArray& va = var.Get<ezVariantArray>();

      for (ezUInt32 i = 0; i < va.GetCount(); ++i)
      {
        TraverseTree(va[i], Compare);
      }

      EZ_TEST_STRING(Compare.PeekFront().GetData(), "</array>");
      Compare.PopFront();
    }
    break;

  case ezVariant::Type::Bool:
    EZ_TEST_STRING(Compare.PeekFront().GetData(), var.Get<bool>() ? "true" : "false");
    Compare.PopFront();
    break;

  case ezVariant::Type::Double:
    {
      ezStringBuilder sTemp;
      sTemp.Format("%.4f", var.Get<double>());
      EZ_TEST_STRING(Compare.PeekFront().GetData(), sTemp.GetData());
      Compare.PopFront();
    }
    break;

  case ezVariant::Type::String:
    EZ_TEST_STRING(Compare.PeekFront().GetData(), var.Get<ezString>().GetData());
    Compare.PopFront();
    break;
  }
}

EZ_CREATE_SIMPLE_TEST(IO, JSONReader)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Test")
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

    // NOTE: The way this test is implemented, it might break, if the HashMap uses another insertion algorithm.
    // ezVariantDictionary is an ezHashmap and this test currently relies on one exact order in of the result.
    // If this should ever change (or be arbitrary at runtime), the test needs to be implemented in a more robust way.

    StringStream stream(szTestData);

    ezJSONReader reader;
    EZ_TEST_BOOL(reader.Parse(stream).IsSuccess());

    ezDeque<ezString> sCompare;
    sCompare.PushBack("<object>");
      sCompare.PushBack("bool");
      sCompare.PushBack("true");

      sCompare.PushBack("int");
      sCompare.PushBack("23.0000");

      sCompare.PushBack("myarray");
      sCompare.PushBack("<array>");
        sCompare.PushBack("1.0000");
        sCompare.PushBack("2.2000");
        sCompare.PushBack("3.3000");
        sCompare.PushBack("false");
        sCompare.PushBack("ende");
      sCompare.PushBack("</array>");

      sCompare.PushBack("object");
      sCompare.PushBack("<object>");

        sCompare.PushBack("variable in object");
        sCompare.PushBack("bla");
      
        sCompare.PushBack("Subobject");
        sCompare.PushBack("<object>");

          sCompare.PushBack("array in sub");
          sCompare.PushBack("<array>");

            sCompare.PushBack("<object>");
              sCompare.PushBack("obj var");
              sCompare.PushBack("234.0000");
            sCompare.PushBack("</object>");

            sCompare.PushBack("<object>");
              sCompare.PushBack("obj var 2");
              sCompare.PushBack("-235.0000");
            sCompare.PushBack("</object>");

            sCompare.PushBack("true");
            sCompare.PushBack("4.0000");
            sCompare.PushBack("false");

          sCompare.PushBack("</array>");

          sCompare.PushBack("variable in subobject");
          sCompare.PushBack("blub");

        sCompare.PushBack("</object>");

      sCompare.PushBack("</object>");

      sCompare.PushBack("test");
      sCompare.PushBack("text");

      sCompare.PushBack("String");
      sCompare.PushBack("testvalue");

      sCompare.PushBack("float");
      sCompare.PushBack("64.7200");

      sCompare.PushBack("double");
      sCompare.PushBack("43.5600");

      sCompare.PushBack("myarray2");
      sCompare.PushBack("<array>");
        sCompare.PushBack("");
        sCompare.PushBack("2.2000");
      sCompare.PushBack("</array>");

    sCompare.PushBack("</object>");

    TraverseTree(reader.GetTopLevelObject(), sCompare);

    EZ_TEST_BOOL(sCompare.IsEmpty());
  }

}
