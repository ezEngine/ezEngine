#pragma once

EZ_FORCE_INLINE bool ezPathUtils::IsPathSeparator (ezUInt32 c)
{
  return (c == '/' || c == '\\');
}

