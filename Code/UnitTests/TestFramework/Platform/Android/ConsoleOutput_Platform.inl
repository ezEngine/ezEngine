#include <android/log.h>

void OutputToConsole_Platform(ezUInt8 uiColor, ezTestOutput::Enum type, ezInt32 iIndentation, const char* szMsg)
{
  printf("%*s%s\n", iIndentation, "", szMsg);
  __android_log_print(ANDROID_LOG_DEBUG, "ezEngine", "%*s%s\n", iIndentation, "", szMsg);
}
