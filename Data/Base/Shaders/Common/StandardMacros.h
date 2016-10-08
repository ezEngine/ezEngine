#pragma once

/// \brief Concatenates two strings, even when the strings are macros themselves
#define EZ_CONCAT(x,y) EZ_CONCAT_HELPER(x,y)
#define EZ_CONCAT_HELPER(x,y) EZ_CONCAT_HELPER2(x,y)
#define EZ_CONCAT_HELPER2(x,y) x##y

/// \brief Turns some piece of code (usually some identifier name) into a string. Even works on macros.
#define EZ_STRINGIZE(str) EZ_STRINGIZE_HELPER(str)
#define EZ_STRINGIZE_HELPER(x) #x


/// \brief Used in conjunction with EZ_ENABLED and EZ_DISABLED for safe checks. Define something to EZ_ON or EZ_OFF to work with those macros.
#define EZ_ON =

/// \brief Used in conjunction with EZ_ENABLED and EZ_DISABLED for safe checks. Define something to EZ_ON or EZ_OFF to work with those macros.
#define EZ_OFF !

/// \brief Used in conjunction with EZ_ON and EZ_OFF for safe checks. Use #if EZ_ENABLED(x) or #if EZ_DISABLED(x) in conditional compilation.
#define EZ_ENABLED(x) (1 EZ_CONCAT(x,=) 1)

/// \brief Used in conjunction with EZ_ON and EZ_OFF for safe checks. Use #if EZ_ENABLED(x) or #if EZ_DISABLED(x) in conditional compilation.
#define EZ_DISABLED(x) (1 EZ_CONCAT(x,=) 2)

/// \brief Checks whether x AND y are both defined as EZ_ON or EZ_OFF. Usually used to check whether configurations overlap, to issue an error.
#define EZ_IS_NOT_EXCLUSIVE(x, y) ((1 EZ_CONCAT(x,=) 1) == (1 EZ_CONCAT(y,=) 1))
