
bool ezDefaultAssertHandler_Platform(const char* szSourceFile, ezUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szAssertMsg)
{
  // always do a debug-break
  // in release-builds this will just crash the app
  return true;
}
