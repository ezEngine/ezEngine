#include <PCH.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Strings/StringUtils.h>
#include <FoundationTest/IO/JSONTestHelpers.h>


EZ_CREATE_SIMPLE_TEST(IO, DdlReader)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "All Features")
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

    ezOpenDdlReader reader;
    reader.SetInputStream(stream);

    EZ_TEST_BOOL(reader.ParseDocument().Succeeded());

    auto pRoot = reader.GetRootElement();
    EZ_TEST_INT(pRoot->GetNumChildObjects(), 1);
  }
}
