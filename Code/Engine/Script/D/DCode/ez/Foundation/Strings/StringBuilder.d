module ez.Foundation.Strings.StringBuilder;
import ez.Foundation.Memory.AllocatorBase;

extern(C++) struct ezStringBuilder
{
private:
  version(X86_64)
    void[288] _data;
  else
    static assert(false, "plattform not supported");
public:
  void AppendFormat(const(char)* szUtf8Format, ...);

  const(char)* GetData() const;
}

pragma(mangle, "?ezConstructStringBuilder@@YAXAEAVezStringBuilder@@@Z")
extern(C++) void ezConstructStringBuilder(ref ezStringBuilder builder);
pragma(mangle, "?ezDestroyStringBuilder@@YAXAEAVezStringBuilder@@@Z")
extern(C++) void ezDestroyStringBuilder(ref ezStringBuilder builder);

const(char)[] GetString(ref ezStringBuilder builder)
{
  import core.stdc.string : strlen;
  auto data = builder.GetData();
  return data[0..strlen(data)];
}