module ez.Foundation.Types.Delegate;
import ez.Foundation.Types.Types;
import std.traits;
import core.stdc.string : memcpy;

template Signature(T)
{
  alias CT = ReturnType!T function(ParameterTypeTuple!T);
  alias Signature = typeof(*(CT.init));
}

struct ezDelegate(T)
{
public:
  alias ReturnType = std.traits.ReturnType!T;
  alias ParameterTypes = ParameterTypeTuple!T;

  this(ReturnType function(ParameterTypes) func)
  {
    static assert(func.sizeof <= DATA_SIZE);
    memcpy(m_Data.ptr, &func, func.sizeof);
    m_pDispatchFunction = &DispatchDFunction;
  }

  this(ReturnType delegate(ParameterTypes) func)
  {
    m_pInstance = func.ptr;
    static assert(func.sizeof <= DATA_SIZE);
    memcpy(m_Data.ptr, &func, func.sizeof);
    m_pDispatchFunction = &DispatchDDelegate;
  }

  ReturnType opCall(ParameterTypes args)
  {
    return m_pDispatchFunction(this, args);
  }
private:

  extern(C++) static ReturnType DispatchDFunction(ref typeof(this) self, ParameterTypes args)
  {
    auto func = *cast(ReturnType function(ParameterTypes)*)(cast(void*)self.m_Data.ptr);
    return func(args);
  }

  extern(C++) static ReturnType DispatchDDelegate(ref typeof(this) self, ParameterTypes args)
  {
    auto del = *cast(ReturnType delegate(ParameterTypes)*)(cast(void*)self.m_Data.ptr);
    return del(args);
  }

  void* m_pInstance;
  extern(C++) ReturnType function(ref typeof(this), ParameterTypes) m_pDispatchFunction;
  enum size_t DATA_SIZE = 16;
  ezUInt8 m_Data[DATA_SIZE];
};

auto ezMakeDelegate(T)(T func)
{
  static assert(isFunctionPointer!T || isDelegate!T, T.stringof ~ " is neither a function pointer nor a delegate.");
  return ezDelegate!(Signature!T)(func);
}