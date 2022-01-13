module ez.foundation.strings.formatstring;

extern(C++, class) class ezFormatString
{
    ~this();

    extern(D) this(string text)
    {
        import std.string : toStringz;

        m_szString = toStringz(text);
    }

    final bool IsEmpty() const;
    //string GetText() const { return }

private:
    const (char*) m_szString;
}
