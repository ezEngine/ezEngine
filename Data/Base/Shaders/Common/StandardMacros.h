#pragma once

#ifndef EZ_PP_CONCAT

/// \brief Concatenates two strings, even when the strings are macros themselves
#  define EZ_PP_CONCAT(x, y) EZ_PP_CONCAT_HELPER(x, y)
#  define EZ_PP_CONCAT_HELPER(x, y) EZ_PP_CONCAT_HELPER2(x, y)
#  define EZ_PP_CONCAT_HELPER2(x, y) x##y

#endif

#ifndef EZ_PP_STRINGIFY

/// \brief Turns some piece of code (usually some identifier name) into a string. Even works on macros.
#  define EZ_PP_STRINGIFY(str) EZ_PP_STRINGIFY_HELPER(str)
#  define EZ_PP_STRINGIFY_HELPER(x) #x

#endif

#ifndef EZ_ON

/// \brief Used in conjunction with EZ_ENABLED and EZ_DISABLED for safe checks. Define something to EZ_ON or EZ_OFF to work with those macros.
#  define EZ_ON =

/// \brief Used in conjunction with EZ_ENABLED and EZ_DISABLED for safe checks. Define something to EZ_ON or EZ_OFF to work with those macros.
#  define EZ_OFF !

/// \brief Used in conjunction with EZ_ON and EZ_OFF for safe checks. Use #if EZ_ENABLED(x) or #if EZ_DISABLED(x) in conditional compilation.
#  define EZ_ENABLED(x) (1 EZ_PP_CONCAT(x, =) 1)

/// \brief Used in conjunction with EZ_ON and EZ_OFF for safe checks. Use #if EZ_ENABLED(x) or #if EZ_DISABLED(x) in conditional compilation.
#  define EZ_DISABLED(x) (1 EZ_PP_CONCAT(x, =) 2)

/// \brief Checks whether x AND y are both defined as EZ_ON or EZ_OFF. Usually used to check whether configurations overlap, to issue an error.
#  define EZ_IS_NOT_EXCLUSIVE(x, y) ((1 EZ_PP_CONCAT(x, =) 1) == (1 EZ_PP_CONCAT(y, =) 1))

#endif

#define BG_FRAME 0
#define BG_RENDER_PASS 1
#define BG_MATERIAL 2
#define BG_DRAW_CALL 3
#define SLOT_AUTO AUTO

/// \brief Binds the resource to the given bind group and slot. Note, that this does not produce valid HLSL code, the code will instead be patched by the shader compiler.
#define BIND_RESOURCE(Slot, BindGroup) : register(EZ_PP_CONCAT(x, Slot), EZ_PP_CONCAT(space, BindGroup))

/// \brief Binds the resource to the given bind group. Note, that this does not produce valid HLSL code, the code will instead be patched by the shader compiler.
#define BIND_GROUP(BindGroup) BIND_RESOURCE(SLOT_AUTO, BindGroup)
