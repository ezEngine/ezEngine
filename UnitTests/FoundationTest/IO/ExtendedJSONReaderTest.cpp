#include <PCH.h>
#include <Foundation/IO/ExtendedJSONReader.h>
#include <Foundation/Containers/Deque.h>
#include <FoundationTest/IO/JSONTestHelpers.h>

void TraverseTree(const ezVariant& var, ezDeque<ezString>& Compare);

EZ_CREATE_SIMPLE_TEST(IO, ExtendedJSONReader)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Test")
  {
    const char* szTestData =
"\
{\n\
\"var1\" : { \"$t\" : \"uint32\", \"$v\" : \"23\",  \"$b\" : \"0x17000000\" },\n\
\"var2\" : { \"$t\" : \"int32\",  \"$v\" : \"-42\", \"$b\" : \"0xD6FFFFFF\" },\n\
\"var3\" : { \"$t\" : \"int64\",  \"$v\" : \"-42\", \"$b\" : \"0xD6FFFFFFFFFFFFFF\" },\n\
\"var4\" : { \"$t\" : \"uint64\", \"$v\" : \"23\",  \"$b\" : \"0x1700000000000000\" },\n\
\"var5\" : { \"$t\" : \"float\", \"$v\" : \"-65.5000\", \"$b\" : \"0x000083C2\" },\n\
\"var6\" : { \"$t\" : \"double\", \"$v\" : \"2621.06250000\", \"$b\" : \"0x00000000207AA440\" },\n\
\"var7\" : { \"$t\" : \"time\", \"$v\" : \"0.50001\", \"$b\" : \"0x000000000000E03F\" },\n\
\"var8\" : { \"$t\" : \"vec2\", \"$v\" : \"(1.0000, 2.0000)\", \"$b\" : \"0x0000803F00000040\" },\n\
\"var9\" : { \"$t\" : \"vec3\", \"$v\" : \"(1.0000, 2.0000, 3.0000)\", \"$b\" : \"0x0000803F0000004000004040\" },\n\
\"var10\": { \"$t\" : \"vec4\", \"$v\" : \"(1.0000, 2.0000, 3.0000, 4.0000)\", \"$b\" : \"0x0000803F000000400000404000008040\" },\n\
\"var11\": { \"$t\" : \"quat\", \"$b\" : \"0x0000803F000000400000404000008040\" },\n\
\"var12\": { \"$t\" : \"mat3\", \"$b\" : \"0x0000803F000080400000E040000000400000A04000000041000040400000C04000001041\" },\n\
\"var13\": { \"$t\" : \"mat4\", \"$b\" : \"0x0000803F0000A0400000104100005041000000400000C0400000204100006041000040400000E040000030410000704100008040000000410000404100008041\" },\n\
\"var14\": { \"$t\" : \"color\", \"$v\" : \"(1.0001, 2.0000, 3.0000, 4.0000)\", \"$b\" : \"0x0000803F000000400000404000008040\" },\n\
\"var15\": { \"$t\" : \"uuid\", \"$v\" : \"(0, 0)\", \"$b\" : \"0x00000000000000000000000000000000\" },\n\
}";

    // NOTE: The way this test is implemented, it might break, if the HashMap uses another insertion algorithm.
    // ezVariantDictionary is an ezHashmap and this test currently relies on one exact order in of the result.
    // If this should ever change (or be arbitrary at runtime), the test needs to be implemented in a more robust way.

    ezTime t = ezTime::Seconds(0.3f);
    ezVariant vt = t;

    EZ_TEST_BOOL(vt.CanConvertTo<ezTime>());

    ezResult r(EZ_FAILURE);
    ezTime t2 = vt.ConvertTo<ezTime>(&r);
    EZ_IGNORE_UNUSED(t2);

    EZ_TEST_BOOL(r.Succeeded());

    StringStream stream(szTestData);

    ezExtendedJSONReader reader;
    EZ_TEST_BOOL(reader.Parse(stream).Succeeded());

    ezDeque<ezString> sCompare;
    sCompare.PushBack("<object>");

      sCompare.PushBack("var2");
      sCompare.PushBack("int32 -42");

      sCompare.PushBack("var10");
      sCompare.PushBack("vec4 (1.0000, 2.0000, 3.0000, 4.0000)");

      sCompare.PushBack("var13");
      sCompare.PushBack("mat4 (1.0000, 5.0000, 9.0000, 13.0000, 2.0000, 6.0000, 10.0000, 14.0000, 3.0000, 7.0000, 11.0000, 15.0000, 4.0000, 8.0000, 12.0000, 16.0000)");

      sCompare.PushBack("var1");
      sCompare.PushBack("uint32 23");

      sCompare.PushBack("var5");
      sCompare.PushBack("float -65.5000");

      sCompare.PushBack("var9");
      sCompare.PushBack("vec3 (1.0000, 2.0000, 3.0000)");

      sCompare.PushBack("var4");
      sCompare.PushBack("uint64 23");

      sCompare.PushBack("var3");
      sCompare.PushBack("int64 -42");

      sCompare.PushBack("var12");
      sCompare.PushBack("mat3 (1.0000, 4.0000, 7.0000, 2.0000, 5.0000, 8.0000, 3.0000, 6.0000, 9.0000)");

      sCompare.PushBack("var14");
      sCompare.PushBack("color (1.0001, 2.0000, 3.0000, 4.0000)");

      sCompare.PushBack("var7");
      sCompare.PushBack("time 0.5000");

      sCompare.PushBack("var15");
      sCompare.PushBack("uuid { 00000000-0000-0000-0000-000000000000 }");

      sCompare.PushBack("var6");
      sCompare.PushBack("double 2621.0625");

      sCompare.PushBack("var11");
      sCompare.PushBack("quat (1.0000, 2.0000, 3.0000, 4.0000)");

      sCompare.PushBack("var8");
      sCompare.PushBack("vec2 (1.0000, 2.0000)");

    sCompare.PushBack("</object>");

    TraverseTree(reader.GetTopLevelObject(), sCompare);

    EZ_TEST_BOOL(sCompare.IsEmpty());
  }

}
