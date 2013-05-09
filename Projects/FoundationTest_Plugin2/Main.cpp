#include <PCH.h>

ezPlugin g_Plugin(true, NULL, NULL, "FoundationTest_Plugin1");

ezCVarInt     CVar_TestInt    ("test2_Int",   22,   ezCVarFlags::None, "Desc: test2_Int");
ezCVarFloat   CVar_TestFloat  ("test2_Float", 2.2f, ezCVarFlags::Default, "Desc: test2_Float");
ezCVarBool    CVar_TestBool   ("test2_Bool",  true, ezCVarFlags::Save, "Desc: test2_Bool");
ezCVarString  CVar_TestString ("test2_String", "test2", ezCVarFlags::RequiresRestart, "Desc: test2_String");

