
#pragma once

/// Concatenates two strings, even when the strings are macros themselves
#define EZ_CONCAT(x,y) EZ_CONCAT_HELPER(x,y)
#define EZ_CONCAT_HELPER(x,y) EZ_CONCAT_HELPER2(x,y)
#define EZ_CONCAT_HELPER2(x,y) x##y

/// Stringizes a string, even macros
#define EZ_STRINGIZE(str) EZ_STRINGIZE_HELPER(str)
#define EZ_STRINGIZE_HELPER(x) #x

// Only use these if a compile-time constant expression is needed
#define EZ_COMPILE_TIME_MAX(a, b) (a) > (b) ? (a) : (b)
#define EZ_COMPILE_TIME_MIN(a, b) (a) < (b) ? (a) : (b)