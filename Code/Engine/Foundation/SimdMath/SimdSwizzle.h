#pragma once

struct ezSwizzle
{
  enum Enum
  {
    //XX
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

    //XY
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

    //XZ
    XZXX = 0x0200,
    XZXY = 0x0201,
    XZXZ = 0x0202,
    XZXW = 0x0203,

    XZYX = 0x0210,
    XZYY = 0x0211,
    XZYZ = 0x0212,
    XZYW = 0x0213,

    XZZX = 0x0220,
    XZZY = 0x0221,
    XZZZ = 0x0222,
    XZZW = 0x0223,

    XZWX = 0x0230,
    XZWY = 0x0231,
    XZWZ = 0x0232,
    XZWW = 0x0233,

    //XW
    XWXX = 0x0300,
    XWXY = 0x0301,
    XWXZ = 0x0302,
    XWXW = 0x0303,

    XWYX = 0x0310,
    XWYY = 0x0311,
    XWYZ = 0x0312,
    XWYW = 0x0313,

    XWZX = 0x0320,
    XWZY = 0x0321,
    XWZZ = 0x0322,
    XWZW = 0x0323,

    XWWX = 0x0330,
    XWWY = 0x0331,
    XWWZ = 0x0332,
    XWWW = 0x0333,

    //////////////
    //YX
    YXXX = 0x1000,
    YXXY = 0x1001,
    YXXZ = 0x1002,
    YXXW = 0x1003,

    YXYX = 0x1010,
    YXYY = 0x1011,
    YXYZ = 0x1012,
    YXYW = 0x1013,

    YXZX = 0x1020,
    YXZY = 0x1021,
    YXZZ = 0x1022,
    YXZW = 0x1023,

    YXWX = 0x1030,
    YXWY = 0x1031,
    YXWZ = 0x1032,
    YXWW = 0x1033,

    //YY
    YYXX = 0x1100,
    YYXY = 0x1101,
    YYXZ = 0x1102,
    YYXW = 0x1103,

    YYYX = 0x1110,
    YYYY = 0x1111,
    YYYZ = 0x1112,
    YYYW = 0x1113,

    YYZX = 0x1120,
    YYZY = 0x1121,
    YYZZ = 0x1122,
    YYZW = 0x1123,

    YYWX = 0x1130,
    YYWY = 0x1131,
    YYWZ = 0x1132,
    YYWW = 0x1133,

    //YZ
    YZXX = 0x1200,
    YZXY = 0x1201,
    YZXZ = 0x1202,
    YZXW = 0x1203,

    YZYX = 0x1210,
    YZYY = 0x1211,
    YZYZ = 0x1212,
    YZYW = 0x1213,

    YZZX = 0x1220,
    YZZY = 0x1221,
    YZZZ = 0x1222,
    YZZW = 0x1223,

    YZWX = 0x1230,
    YZWY = 0x1231,
    YZWZ = 0x1232,
    YZWW = 0x1233,

    //YW
    YWXX = 0x1300,
    YWXY = 0x1301,
    YWXZ = 0x1302,
    YWXW = 0x1303,

    YWYX = 0x1310,
    YWYY = 0x1311,
    YWYZ = 0x1312,
    YWYW = 0x1313,

    YWZX = 0x1320,
    YWZY = 0x1321,
    YWZZ = 0x1322,
    YWZW = 0x1323,

    YWWX = 0x1330,
    YWWY = 0x1331,
    YWWZ = 0x1332,
    YWWW = 0x1333,

    //////////////
    //ZX
    ZXXX = 0x2000,
    ZXXY = 0x2001,
    ZXXZ = 0x2002,
    ZXXW = 0x2003,

    ZXYX = 0x2010,
    ZXYY = 0x2011,
    ZXYZ = 0x2012,
    ZXYW = 0x2013,

    ZXZX = 0x2020,
    ZXZY = 0x2021,
    ZXZZ = 0x2022,
    ZXZW = 0x2023,

    ZXWX = 0x2030,
    ZXWY = 0x2031,
    ZXWZ = 0x2032,
    ZXWW = 0x2033,

    //ZY
    ZYXX = 0x2100,
    ZYXY = 0x2101,
    ZYXZ = 0x2102,
    ZYXW = 0x2103,

    ZYYX = 0x2110,
    ZYYY = 0x2111,
    ZYYZ = 0x2112,
    ZYYW = 0x2113,

    ZYZX = 0x2120,
    ZYZY = 0x2121,
    ZYZZ = 0x2122,
    ZYZW = 0x2123,

    ZYWX = 0x2130,
    ZYWY = 0x2131,
    ZYWZ = 0x2132,
    ZYWW = 0x2133,

    //ZZ
    ZZXX = 0x2200,
    ZZXY = 0x2201,
    ZZXZ = 0x2202,
    ZZXW = 0x2203,

    ZZYX = 0x2210,
    ZZYY = 0x2211,
    ZZYZ = 0x2212,
    ZZYW = 0x2213,

    ZZZX = 0x2220,
    ZZZY = 0x2221,
    ZZZZ = 0x2222,
    ZZZW = 0x2223,

    ZZWX = 0x2230,
    ZZWY = 0x2231,
    ZZWZ = 0x2232,
    ZZWW = 0x2233,

    //ZW
    ZWXX = 0x2300,
    ZWXY = 0x2301,
    ZWXZ = 0x2302,
    ZWXW = 0x2303,

    ZWYX = 0x2310,
    ZWYY = 0x2311,
    ZWYZ = 0x2312,
    ZWYW = 0x2313,

    ZWZX = 0x2320,
    ZWZY = 0x2321,
    ZWZZ = 0x2322,
    ZWZW = 0x2323,

    ZWWX = 0x2330,
    ZWWY = 0x2331,
    ZWWZ = 0x2332,
    ZWWW = 0x2333,

    //////////////
    //WX
    WXXX = 0x3000,
    WXXY = 0x3001,
    WXXZ = 0x3002,
    WXXW = 0x3003,

    WXYX = 0x3010,
    WXYY = 0x3011,
    WXYZ = 0x3012,
    WXYW = 0x3013,

    WXZX = 0x3020,
    WXZY = 0x3021,
    WXZZ = 0x3022,
    WXZW = 0x3023,

    WXWX = 0x3030,
    WXWY = 0x3031,
    WXWZ = 0x3032,
    WXWW = 0x3033,

    //WY
    WYXX = 0x3100,
    WYXY = 0x3101,
    WYXZ = 0x3102,
    WYXW = 0x3103,

    WYYX = 0x3110,
    WYYY = 0x3111,
    WYYZ = 0x3112,
    WYYW = 0x3113,

    WYZX = 0x3120,
    WYZY = 0x3121,
    WYZZ = 0x3122,
    WYZW = 0x3123,

    WYWX = 0x3130,
    WYWY = 0x3131,
    WYWZ = 0x3132,
    WYWW = 0x3133,

    //WZ
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

    //WW
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

