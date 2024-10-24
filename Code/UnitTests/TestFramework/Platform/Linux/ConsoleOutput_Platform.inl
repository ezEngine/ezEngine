
void OutputToConsole_Platform(ezUInt8 uiColor, ezTestOutput::Enum type, ezInt32 iIndentation, const char* szMsg)
{
  printf("%*s%s\n", iIndentation, "", szMsg);
}
