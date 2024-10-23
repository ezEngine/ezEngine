
bool ezDefaultAssertHandler_Platform(const char* szSourceFile, ezUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szAssertMsg, const char* szTemp)
{
  EZ_IGNORE_UNUSED(szSourceFile);
  EZ_IGNORE_UNUSED(uiLine);
  EZ_IGNORE_UNUSED(szFunction);
  EZ_IGNORE_UNUSED(szExpression);
  EZ_IGNORE_UNUSED(szAssertMsg);
  EZ_IGNORE_UNUSED(szTemp);

  // always do a debug-break
  // in release-builds this will just crash the app
  return true;
}
