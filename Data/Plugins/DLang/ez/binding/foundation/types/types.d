module ez.foundation.types.types;

alias ezUInt8 = ubyte;
alias ezUInt16 = ushort;
alias ezUInt32 = uint;
alias ezUInt64 = ulong;

alias ezInt8 = char;
alias ezInt16 = short;
alias ezInt32 = int;
alias ezInt64 = long;

// CODEGEN-BEGIN: Enum("ezResultEnum")
enum ezResultEnum : int
{
  EZ_FAILURE = 0,
  EZ_SUCCESS = 1,
}
// CODEGEN-END

extern(C++, struct) struct ezResult
{
  // CODEGEN-BEGIN: Struct("ezResult")
  this(ezResultEnum res);
  // Operator: =
  // Operator: ==
  // Operator: !=
  bool Succeeded() const;
  bool Failed() const;
  void IgnoreResult();
  void AssertSuccess(const(char)* msg = null) const;
private:
  ezResultEnum e;
  // Operator: =
  // CODEGEN-END
}

string CstrToDstr(const(char)* str)
{
  import std.string;

  return fromStringz(str).idup;
}