
inline ezColorLinear16f::ezColorLinear16f()
{
}

inline ezColorLinear16f::ezColorLinear16f(ezFloat16 r, ezFloat16 g, ezFloat16 b, ezFloat16 a) :
   r(r), g(g), b(b), a(a)
{
}

inline ezColorLinear16f::ezColorLinear16f(const ezColor& color) :
  r(color.r), g(color.g), b(color.b), a(color.a)
{
}

inline ezColor ezColorLinear16f::ToLinearFloat() const
{
  return ezColor(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b), static_cast<float>(a));
}


