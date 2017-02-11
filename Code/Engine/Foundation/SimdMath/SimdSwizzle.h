#pragma once

///\todo complete this enum with all possible combinations
struct ezSwizzle
{
  enum Enum
  {
    XXXX = 0x0000,
    XXXY = 0x0001,
    XXXZ = 0x0002,
    XXXW = 0x0003,

    XXYX = 0x0010,
    XXYY = 0x0011,
    XXYZ = 0x0012,
    XXYW = 0x0013,

    XXZX = 0x0020,
    XXZY = 0x0021,
    XXZZ = 0x0022,
    XXZW = 0x0023,

    XXWX = 0x0030,
    XXWY = 0x0031,
    XXWZ = 0x0032,
    XXWW = 0x0033,


    XYXX = 0x0100,
    XYXY = 0x0101,
    XYXZ = 0x0102,
    XYXW = 0x0103,

    XYYX = 0x0110,
    XYYY = 0x0111,
    XYYZ = 0x0112,
    XYYW = 0x0113,

    XYZX = 0x0120,
    XYZY = 0x0121,
    XYZZ = 0x0122,
    XYZW = 0x0123,

    XYWX = 0x0130,
    XYWY = 0x0131,
    XYWZ = 0x0132,
    XYWW = 0x0133,


    YYYX = 0x1110,
    YYYY = 0x1111,
    YYYZ = 0x1112,
    YYYW = 0x1113,


    ZZZX = 0x2220,
    ZZZY = 0x2221,
    ZZZZ = 0x2222,
    ZZZW = 0x2223,


    WZXX = 0x3200,
    WZXY = 0x3201,
    WZXZ = 0x3202,
    WZXW = 0x3203,

    WZYX = 0x3210,
    WZYY = 0x3211,
    WZYZ = 0x3212,
    WZYW = 0x3213,

    WZZX = 0x3220,
    WZZY = 0x3221,
    WZZZ = 0x3222,
    WZZW = 0x3223,

    WZWX = 0x3230,
    WZWY = 0x3231,
    WZWZ = 0x3232,
    WZWW = 0x3233,


    WWXX = 0x3300,
    WWXY = 0x3301,
    WWXZ = 0x3302,
    WWXW = 0x3303,

    WWYX = 0x3310,
    WWYY = 0x3311,
    WWYZ = 0x3312,
    WWYW = 0x3313,

    WWZX = 0x3320,
    WWZY = 0x3321,
    WWZZ = 0x3322,
    WWZW = 0x3323,

    WWWX = 0x3330,
    WWWY = 0x3331,
    WWWZ = 0x3332,
    WWWW = 0x3333
  };
};

