
#pragma once

/// \file

/// \brief Concatenates two strings, even when the strings are macros themselves
#define EZ_CONCAT(x,y) EZ_CONCAT_HELPER(x,y)
#define EZ_CONCAT_HELPER(x,y) EZ_CONCAT_HELPER2(x,y)
#define EZ_CONCAT_HELPER2(x,y) x##y

/// \brief Turns some piece of code (usually some identifier name) into a string. Even works on macros.
#define EZ_STRINGIZE(str) EZ_STRINGIZE_HELPER(str)
#define EZ_STRINGIZE_HELPER(x) #x

/// \brief Max value of two compile-time constant expression.
#define EZ_COMPILE_TIME_MAX(a, b) ((a) > (b) ? (a) : (b))

/// \brief Min value of two compile-time constant expression.
#define EZ_COMPILE_TIME_MIN(a, b) ((a) < (b) ? (a) : (b))


/// \brief Creates a bit mask with only the n-th Bit set. Useful when creating enum values for flags.
#define EZ_BIT(n) (1ull << (n))

