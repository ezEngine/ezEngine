module dllmain;

import core.sys.windows.dll;

mixin SimpleDllMain!(DllIsUsedFromC.no);