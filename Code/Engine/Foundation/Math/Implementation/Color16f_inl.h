inline ezColor16f::ezColor16f()
{
}

inline ezColor16f::ezColor16f(ezFloat16 r, ezFloat16 g, ezFloat16 b, ezFloat16 a) :
   r(r), g(g), b(b), a(a)
{
}

inline ezColor16f::ezColor16f(const ezColor& color) :
  r(color.r), g(color.g), b(color.b), a(color.a)
{
}

inline ezColor16f::operator ezColor () const
{
  return ezColor(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b), static_cast<float>(a));
}

inline bool ezColor16f::IsIdentical(const ezColor16f& rhs) const
{
  return r == rhs.r && g == rhs.g && b == rhs.b && a == rhs.a;
}

bool operator== (const ezColor16f& c1, const ezColor16f& c2)
{
  return c1.IsIdentical(c2);
}

bool operator!= (const ezColor16f& c1, const ezColor16f& c2)
{
  return !c1.IsIdentical(c2);
}

