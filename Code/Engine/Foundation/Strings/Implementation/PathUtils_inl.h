#pragma once

EZ_ALWAYS_INLINE bool ezPathUtils::IsPathSeparator(ezUInt32 c)
{
  return (c == '/' || c == '\\');
}

