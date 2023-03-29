#ifndef AE_FOUNDATION_GENERAL_DEFINES_H
#define AE_FOUNDATION_GENERAL_DEFINES_H

// class 'type' needs to have dll-interface to be used by clients of class 'type2' -> dll export / import issues (mostly with templates)
#pragma warning(disable : 4251)

// Configure the DLL Import/Export Define
#ifdef AE_COMPILE_ENGINE_AS_DLL
#  ifdef BUILDSYSTEM_BUILDING_KRAUTFOUNDATION_LIB
#    define AE_FOUNDATION_DLL __declspec(dllexport)
#  else
#    define AE_FOUNDATION_DLL __declspec(dllimport)
#  endif
#else
#  define AE_FOUNDATION_DLL
#endif

typedef unsigned char aeUInt8;
typedef unsigned short aeUInt16;
typedef unsigned int aeUInt32;
typedef unsigned long long aeUInt64;

typedef char aeInt8;
typedef short aeInt16;
typedef int aeInt32;
typedef long long aeInt64;

#endif
