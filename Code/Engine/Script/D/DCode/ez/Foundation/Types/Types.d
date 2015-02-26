module ez.Foundation.Types.Types;

enum GcSupport : bool
{
  No = false,
  Yes = true
}

extern(C++):

alias ubyte     ezUInt8;
alias ushort    ezUInt16;
alias uint      ezUInt32;
alias ulong     ezUInt64;

alias char      ezInt8;
alias short     ezInt16;
alias int       ezInt32;
alias long      ezInt64;

static assert(bool.sizeof     == 1);
static assert(char.sizeof     == 1);
static assert(float.sizeof    == 4);
static assert(double.sizeof   == 8);
static assert(ezInt8.sizeof   == 1);
static assert(ezInt16.sizeof  == 2);
static assert(ezInt32.sizeof  == 4);
static assert(ezInt64.sizeof  == 8);
static assert(ezUInt8.sizeof  == 1);
static assert(ezUInt16.sizeof == 2);
static assert(ezUInt32.sizeof == 4);
static assert(ezUInt64.sizeof == 8); 