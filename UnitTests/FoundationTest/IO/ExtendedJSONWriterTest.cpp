#include <PCH.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/JSONWriter.h>
#include <Foundation/IO/ExtendedJSONWriter.h>
#include <FoundationTest/IO/JSONTestHelpers.h>

EZ_CREATE_SIMPLE_TEST(IO, ExtendedJSONWriter)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Object")
  {
    StreamComparer sc(
"\"TestObject\" : {\n\
  \n\
}");

    ezExtendedJSONWriter js;
    js.SetOutputStream(&sc);

    js.BeginObject("TestObject");
    js.EndObject();
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Anonymous Object")
  {
    StreamComparer sc(
"{\n\
  \n\
}");

    ezExtendedJSONWriter js;
    js.SetOutputStream(&sc);

    js.BeginObject();
    js.EndObject();
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "AddVariableBool")
  {
    StreamComparer sc("\"var1\" : true,\n\"var2\" : false");

    ezExtendedJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableBool("var1", true);
    js.AddVariableBool("var2", false);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "AddVariableInt32")
  {
    StreamComparer sc("\
\"var1\" : { \"$t\" : \"int32\", \"$v\" : \"23\", \"$b\" : \"0x17000000\" },\n\
\"var2\" : { \"$t\" : \"int32\", \"$v\" : \"-42\", \"$b\" : \"0xD6FFFFFF\" }");

    ezExtendedJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableInt32("var1", 23);
    js.AddVariableInt32("var2", -42);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "AddVariableUInt32")
  {
    StreamComparer sc("\
\"var1\" : { \"$t\" : \"uint32\", \"$v\" : \"23\", \"$b\" : \"0x17000000\" },\n\
\"var2\" : { \"$t\" : \"uint32\", \"$v\" : \"42\", \"$b\" : \"0x2A000000\" }");

    ezExtendedJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableUInt32("var1", 23);
    js.AddVariableUInt32("var2", 42);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "AddVariableInt64")
  {
    StreamComparer sc("\
\"var1\" : { \"$t\" : \"int64\", \"$v\" : \"23\", \"$b\" : \"0x1700000000000000\" },\n\
\"var2\" : { \"$t\" : \"int64\", \"$v\" : \"-42\", \"$b\" : \"0xD6FFFFFFFFFFFFFF\" }");

    ezExtendedJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableInt64("var1", 23);
    js.AddVariableInt64("var2", -42);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "AddVariableUInt64")
  {
    StreamComparer sc("\
\"var1\" : { \"$t\" : \"uint64\", \"$v\" : \"23\", \"$b\" : \"0x1700000000000000\" },\n\
\"var2\" : { \"$t\" : \"uint64\", \"$v\" : \"42\", \"$b\" : \"0x2A00000000000000\" }");

    ezExtendedJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableUInt64("var1", 23);
    js.AddVariableUInt64("var2", 42);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "AddVariableFloat")
  {
    StreamComparer sc("\
\"var1\" : { \"$t\" : \"float\", \"$v\" : \"-65.5000\", \"$b\" : \"0x000083C2\" },\n\
\"var2\" : { \"$t\" : \"float\", \"$v\" : \"2621.2500\", \"$b\" : \"0x00D42345\" }");

    ezExtendedJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableFloat("var1", -65.5f);
    js.AddVariableFloat("var2", 2621.25f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "AddVariableDouble")
  {
    StreamComparer sc("\
\"var1\" : { \"$t\" : \"double\", \"$v\" : \"-65.12500000\", \"$b\" : \"0x00000000004850C0\" },\n\
\"var2\" : { \"$t\" : \"double\", \"$v\" : \"2621.06250000\", \"$b\" : \"0x00000000207AA440\" }");

    ezExtendedJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableDouble("var1", -65.125f);
    js.AddVariableDouble("var2", 2621.0625f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "AddVariableString")
  {
    StreamComparer sc("\"var1\" : \"bla\",\n\"var2\" : \"blub\"");

    ezExtendedJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableString("var1", "bla");
    js.AddVariableString("var2", "blub");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "AddVariableNULL")
  {
    StreamComparer sc("\"var1\" : null");

    ezExtendedJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableNULL("var1");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "AddVariableTime")
  {
    StreamComparer sc("\
\"var1\" : { \"$t\" : \"time\", \"$v\" : \"0.5000\", \"$b\" : \"0x000000000000E03F\" },\n\
\"var2\" : { \"$t\" : \"time\", \"$v\" : \"2.2500\", \"$b\" : \"0x0000000000000240\" }");


    ezExtendedJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableTime("var1", ezTime::Seconds(0.5));
    js.AddVariableTime("var2", ezTime::Seconds(2.25));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "AddVariableVec2")
  {
    StreamComparer sc("\"var1\" : { \"$t\" : \"vec2\", \"$v\" : \"(1.0000, 2.0000)\", \"$b\" : \"0x0000803F00000040\" }");

    ezExtendedJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableVec2("var1", ezVec2(1, 2));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "AddVariableVec3")
  {
    StreamComparer sc("\"var1\" : { \"$t\" : \"vec3\", \"$v\" : \"(1.0000, 2.0000, 3.0000)\", \"$b\" : \"0x0000803F0000004000004040\" }");

    ezExtendedJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableVec3("var1", ezVec3(1, 2, 3));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "AddVariableVec4")
  {
    StreamComparer sc("\"var1\" : { \"$t\" : \"vec4\", \"$v\" : \"(1.0000, 2.0000, 3.0000, 4.0000)\", \"$b\" : \"0x0000803F000000400000404000008040\" }");

    ezExtendedJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableVec4("var1", ezVec4(1, 2, 3, 4));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "AddVariableQuat")
  {
    StreamComparer sc("\"var1\" : { \"$t\" : \"quat\", \"$b\" : \"0x0000803F000000400000404000008040\" }");

    ezExtendedJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableQuat("var1", ezQuat(1, 2, 3, 4));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "AddVariableMat3")
  {
    StreamComparer sc("\"var1\" : { \"$t\" : \"mat3\", \"$b\" : \"0x0000803F000080400000E040000000400000A04000000041000040400000C04000001041\" }");

    ezExtendedJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableMat3("var1", ezMat3(1, 2, 3, 4, 5, 6, 7, 8, 9));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "AddVariableMat4")
  {
    StreamComparer sc("\"var1\" : { \"$t\" : \"mat4\", \"$b\" : \"0x0000803F0000A0400000104100005041000000400000C0400000204100006041000040400000E040000030410000704100008040000000410000404100008041\" }");

    ezExtendedJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableMat4("var1", ezMat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "AddVariableVariant")
  {
    StreamComparer sc("\
\"var1\" : { \"$t\" : \"int32\", \"$v\" : \"23\", \"$b\" : \"0x17000000\" },\n\
\"var2\" : { \"$t\" : \"float\", \"$v\" : \"42.5000\", \"$b\" : \"0x00002A42\" },\n\
\"var3\" : { \"$t\" : \"double\", \"$v\" : \"21.25000000\", \"$b\" : \"0x0000000000403540\" },\n\
\"var4\" : true,\n\
\"var5\" : \"pups\"");

    ezExtendedJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableVariant("var1", ezVariant(23));
    js.AddVariableVariant("var2", ezVariant(42.5f));
    js.AddVariableVariant("var3", ezVariant(21.25));
    js.AddVariableVariant("var4", ezVariant(true));
    js.AddVariableVariant("var5", ezVariant("pups"));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Arrays")
  {
    StreamComparer sc("\
{\n\
  \"EmptyArray\" : [  ],\n\
  \"NamedArray\" : [ { \"$t\" : \"int32\", \"$v\" : \"13\", \"$b\" : \"0x0D000000\" } ],\n\
  \"NamedArray2\" : [ { \"$t\" : \"int32\", \"$v\" : \"1337\", \"$b\" : \"0x39050000\" }, { \"$t\" : \"int32\", \"$v\" : \"-4996\", \"$b\" : \"0x7CECFFFF\" } ],\n\
  \"Nested\" : [ null, [ { \"$t\" : \"int32\", \"$v\" : \"1\", \"$b\" : \"0x01000000\" }, { \"$t\" : \"int32\", \"$v\" : \"2\", \"$b\" : \"0x02000000\" }, { \"$t\" : \"int32\", \"$v\" : \"3\", \"$b\" : \"0x03000000\" } ], [ { \"$t\" : \"int32\", \"$v\" : \"4\", \"$b\" : \"0x04000000\" }, { \"$t\" : \"int32\", \"$v\" : \"5\", \"$b\" : \"0x05000000\" }, { \"$t\" : \"int32\", \"$v\" : \"6\", \"$b\" : \"0x06000000\" } ], [  ], \"That was an empty array\" ]\n\
}");

    ezExtendedJSONWriter js;
    js.SetOutputStream(&sc);

    js.BeginObject();
      js.BeginArray("EmptyArray");
      js.EndArray();

      js.BeginArray("NamedArray");
        js.WriteInt32(13);
      js.EndArray();

      js.BeginArray("NamedArray2");
        js.WriteInt32(1337);
        js.WriteInt32(-4996);
      js.EndArray();

      js.BeginVariable("Nested");
        js.BeginArray();
          js.WriteNULL();

          js.BeginArray();
            js.WriteInt32(1);
            js.WriteInt32(2);
            js.WriteInt32(3);
          js.EndArray();

          js.BeginArray();
            js.WriteInt32(4);
            js.WriteInt32(5);
            js.WriteInt32(6);
          js.EndArray();

          js.BeginArray();
          js.EndArray();

          js.WriteString("That was an empty array");
        js.EndArray();
      js.EndVariable();

    js.EndObject();
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Complex Objects")
  {
    StreamComparer sc("\
{\n\
  \"String\" : \"testvalue\",\n\
  \"double\" : { \"$t\" : \"double\", \"$v\" : \"43.56000000\", \"$b\" : \"0x48E17A14AEC74540\" },\n\
  \"float\" : { \"$t\" : \"float\", \"$v\" : \"64.7200\", \"$b\" : \"0xA4708142\" },\n\
  \"bool\" : true,\n\
  \"int\" : { \"$t\" : \"int32\", \"$v\" : \"23\", \"$b\" : \"0x17000000\" },\n\
  \"myarray\" : [ { \"$t\" : \"int32\", \"$v\" : \"1\", \"$b\" : \"0x01000000\" }, { \"$t\" : \"float\", \"$v\" : \"2.2000\", \"$b\" : \"0xCDCC0C40\" }, { \"$t\" : \"double\", \"$v\" : \"3.30000000\", \"$b\" : \"0x6666666666660A40\" }, false, \"ende\" ],\n\
  \"object\" : {\n\
    \"variable in object\" : \"bla\",\n\
    \"Subobject\" : {\n\
      \"variable in subobject\" : \"bla\",\n\
      \"array in sub\" : [ {\n\
          \"obj var\" : { \"$t\" : \"uint64\", \"$v\" : \"234\", \"$b\" : \"0xEA00000000000000\" }\n\
        },\n\
        {\n\
          \"obj var 2\" : { \"$t\" : \"int64\", \"$v\" : \"-235\", \"$b\" : \"0x15FFFFFFFFFFFFFF\" }\n\
        }, true, { \"$t\" : \"int32\", \"$v\" : \"4\", \"$b\" : \"0x04000000\" }, false ]\n\
    }\n\
  },\n\
  \"test\" : \"text\"\n\
}");

    ezExtendedJSONWriter js;
    js.SetOutputStream(&sc);

    js.BeginObject();

      js.AddVariableString("String", "testvalue");
      js.AddVariableDouble("double", 43.56);
      js.AddVariableFloat("float", 64.72f);
      js.AddVariableBool("bool", true);
      js.AddVariableInt32("int", 23);

      js.BeginArray("myarray");
        js.WriteInt32(1);
        js.WriteFloat(2.2f);
        js.WriteDouble(3.3);
        js.WriteBool(false);
        js.WriteString("ende");
      js.EndArray();

      js.BeginObject("object");
        js.AddVariableString("variable in object", "bla");
        js.BeginObject("Subobject");
          js.AddVariableString("variable in subobject", "bla");

          js.BeginArray("array in sub");
            js.BeginObject();
              js.AddVariableUInt64("obj var", 234);
            js.EndObject();
            js.BeginObject();
              js.AddVariableInt64("obj var 2", -235);
            js.EndObject();
            js.WriteBool(true);
            js.WriteInt32(4);
            js.WriteBool(false);
          js.EndArray();
        js.EndObject();
      js.EndObject();

      js.AddVariableString("test", "text");

    js.EndObject();
  }
}
