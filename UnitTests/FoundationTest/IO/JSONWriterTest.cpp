#include <PCH.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/JSONWriter.h>
#include <Foundation/IO/ExtendedJSONWriter.h>
#include <FoundationTest/IO/JSONTestHelpers.h>


EZ_CREATE_SIMPLE_TEST(IO, StandardJSONWriter)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Object")
  {
    StreamComparer sc(
"\"TestObject\" : {\n\
  \n\
}");

    ezStandardJSONWriter js;
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

    ezStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.BeginObject();
    js.EndObject();
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "AddVariableBool")
  {
    StreamComparer sc("\"var1\" : true,\n\"var2\" : false");

    ezStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableBool("var1", true);
    js.AddVariableBool("var2", false);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "AddVariableInt32")
  {
    StreamComparer sc("\"var1\" : 23,\n\"var2\" : -42");

    ezStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableInt32("var1", 23);
    js.AddVariableInt32("var2", -42);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "AddVariableUInt32")
  {
    StreamComparer sc("\"var1\" : 23,\n\"var2\" : 42");

    ezStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableUInt32("var1", 23);
    js.AddVariableUInt32("var2", 42);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "AddVariableInt64")
  {
    StreamComparer sc("\"var1\" : 23,\n\"var2\" : -42");

    ezStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableInt64("var1", 23);
    js.AddVariableInt64("var2", -42);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "AddVariableUInt64")
  {
    StreamComparer sc("\"var1\" : 23,\n\"var2\" : 42");

    ezStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableUInt64("var1", 23);
    js.AddVariableUInt64("var2", 42);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "AddVariableFloat")
  {
    StreamComparer sc("\"var1\" : -65.5,\n\"var2\" : 2621.25");

    ezStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableFloat("var1", -65.5f);
    js.AddVariableFloat("var2", 2621.25f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "AddVariableDouble")
  {
    StreamComparer sc("\"var1\" : -65.125,\n\"var2\" : 2621.0625");

    ezStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableDouble("var1", -65.125f);
    js.AddVariableDouble("var2", 2621.0625f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "AddVariableString")
  {
    StreamComparer sc("\"var1\" : \"bla\",\n\"var2\" : \"blub\",\n\"special\" : \"I\\\\m\\t\\\"s\\bec/al\\\" \\f\\n//\\\\\\r\"");

    ezStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableString("var1", "bla");
    js.AddVariableString("var2", "blub");

    js.AddVariableString("special", "I\\m\t\"s\bec/al\" \f\n//\\\r");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "AddVariableNULL")
  {
    StreamComparer sc("\"var1\" : null");

    ezStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableNULL("var1");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "AddVariableTime")
  {
    StreamComparer sc("\"var1\" : 0.5,\n\"var2\" : 2.25");

    ezStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableTime("var1", ezTime::Seconds(0.5));
    js.AddVariableTime("var2", ezTime::Seconds(2.25));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "AddVariableUuid")
  {
    ezUuid guid;
    ezUInt64 val[2];
    val[0] = 0x1122334455667788;
    val[1] = 0x99AABBCCDDEEFF00;
    ezMemoryUtils::Copy(reinterpret_cast<ezUInt64*>(&guid), val, 2);
    
    StreamComparer sc("\"uuid_var\" : { \"$t\" : \"uuid\", \"$b\" : \"0x887766554433221100FFEEDDCCBBAA99\" }");

    ezStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableUuid("uuid_var", guid);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "AddVariableColor")
  {
    StreamComparer sc("\"var1\" : { \"$t\" : \"color\", \"$v\" : \"(1.0000, 2.0000, 3.0000, 4.0000)\", \"$b\" : \"0x0000803F000000400000404000008040\" }");

    ezStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableColor("var1", ezColor(1, 2, 3, 4));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "AddVariableVec2")
  {
    StreamComparer sc("\"var1\" : { \"$t\" : \"vec2\", \"$v\" : \"(1.0000, 2.0000)\", \"$b\" : \"0x0000803F00000040\" }");

    ezStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableVec2("var1", ezVec2(1, 2));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "AddVariableVec3")
  {
    StreamComparer sc("\"var1\" : { \"$t\" : \"vec3\", \"$v\" : \"(1.0000, 2.0000, 3.0000)\", \"$b\" : \"0x0000803F0000004000004040\" }");

    ezStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableVec3("var1", ezVec3(1, 2, 3));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "AddVariableVec4")
  {
    StreamComparer sc("\"var1\" : { \"$t\" : \"vec4\", \"$v\" : \"(1.0000, 2.0000, 3.0000, 4.0000)\", \"$b\" : \"0x0000803F000000400000404000008040\" }");

    ezStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableVec4("var1", ezVec4(1, 2, 3, 4));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "AddVariableQuat")
  {
    StreamComparer sc("\"var1\" : { \"$t\" : \"quat\", \"$b\" : \"0x0000803F000000400000404000008040\" }");

    ezStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableQuat("var1", ezQuat(1, 2, 3, 4));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "AddVariableMat3")
  {
    StreamComparer sc("\"var1\" : { \"$t\" : \"mat3\", \"$b\" : \"0x0000803F000080400000E040000000400000A04000000041000040400000C04000001041\" }");

    ezStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableMat3("var1", ezMat3(1, 2, 3, 4, 5, 6, 7, 8, 9));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "AddVariableMat4")
  {
    StreamComparer sc("\"var1\" : { \"$t\" : \"mat4\", \"$b\" : \"0x0000803F0000A0400000104100005041000000400000C0400000204100006041000040400000E040000030410000704100008040000000410000404100008041\" }");

    ezStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableMat4("var1", ezMat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "AddVariableVariant")
  {
    StreamComparer sc("\
\"var1\" : 23,\n\
\"var2\" : 42.5,\n\
\"var3\" : 21.25,\n\
\"var4\" : true,\n\
\"var5\" : \"pups\"");

    ezStandardJSONWriter js;
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
  \"NamedArray\" : [ 13 ],\n\
  \"NamedArray2\" : [ 1337, -4996 ],\n\
  \"Nested\" : [ null, [ 1, 2, 3 ], [ 4, 5, 6 ], [  ], \"That was an empty array\" ]\n\
}");

    ezStandardJSONWriter js;
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
    ezStringUtf8 sExp(L"\
{\n\
  \"String\" : \"testvälue\",\n\
  \"double\" : 43.56,\n\
  \"float\" : 64.720001,\n\
  \"bööl\" : true,\n\
  \"int\" : 23,\n\
  \"myarray\" : [ 1, 2.2, 3.3, false, \"ende\" ],\n\
  \"object\" : {\n\
    \"variable in object\" : \"bla/*asdf*/ //tuff\",\n\
    \"Subobject\" : {\n\
      \"variable in subobject\" : \"bla\\\\\",\n\
      \"array in sub\" : [ {\n\
          \"obj var\" : 234\n\
        },\n\
        {\n\
          \"obj var 2\" : -235\n\
        }, true, 4, false ]\n\
    }\n\
  },\n\
  \"test\" : \"text\"\n\
}");

    StreamComparer sc(sExp.GetData());

    ezStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.BeginObject();

      js.AddVariableString("String", ezStringUtf8(L"testvälue").GetData()); // Unicode / Utf-8 test (in string)
      js.AddVariableDouble("double", 43.56);
      js.AddVariableFloat("float", 64.72f);
      js.AddVariableBool(ezStringUtf8(L"bööl").GetData(), true); // Unicode / Utf-8 test (identifier)
      js.AddVariableInt32("int", 23);

      js.BeginArray("myarray");
        js.WriteInt32(1);
        js.WriteFloat(2.2f);
        js.WriteDouble(3.3);
        js.WriteBool(false);
        js.WriteString("ende");
      js.EndArray();

      js.BeginObject("object");
        js.AddVariableString("variable in object", "bla/*asdf*/ //tuff"); // 'comment' in string
        js.BeginObject("Subobject");
          js.AddVariableString("variable in subobject", "bla\\"); // character to be escaped

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


