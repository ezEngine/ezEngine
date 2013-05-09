#include <PCH.h>

ezPlugin g_Plugin(true, NULL, NULL);

ezCVarInt     CVar_TestInt    ("test1_Int",   11,   ezCVarFlags::Save, "Desc: test1_Int");
ezCVarFloat   CVar_TestFloat  ("test1_Float", 1.1f, ezCVarFlags::RequiresRestart, "Desc: test1_Float");
ezCVarBool    CVar_TestBool   ("test1_Bool",  false, ezCVarFlags::None, "Desc: test1_Bool");
ezCVarString  CVar_TestString ("test1_String", "test1", ezCVarFlags::Default, "Desc: test1_String");