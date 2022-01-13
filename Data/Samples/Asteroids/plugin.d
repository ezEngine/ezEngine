import core.stdc.stdio;

import core.sys.windows.windows;
import core.sys.windows.dll;
import core.memory;

mixin SimpleDllMain;

/*
extern (C++)
{
    abstract class CppClass
    {
        int QueryValue(int a);
    }

    interface ezDComponentBase
    {
    public:
        void SayHello(int i);
    }

    class MyDComponent : ezDComponentBase
    {
        override void SayHello(int i)
        {
            ezLog.Print("DDDDDDDDDDDDDDDDD");
            ezLog.Printf("\n\nHello again From D = %i\n\n", i);
        }
    }

}
extern (C++) class ezLog
{
public:
    static void Print(const(char)* szFormat);
    static void Printf(const(char)* szFormat, ...);
}

extern(C++, ezMath)
{
    alias ubyte     ezUInt8;
    alias ushort    ezUInt16;
    alias uint      ezUInt32;
    alias ulong     ezUInt64;

    alias char      ezInt8;
    alias short     ezInt16;
    alias int       ezInt32;
    alias long      ezInt64;

  ezUInt32 PowerOfTwo_Ceil(ezUInt32 value);
}

extern (C) export int dll(int i, CppClass source)
{
	ezLog.Print("D calling");
	ezLog.Printf("\n\nD calling with value %i\n\n", i);

    return PowerOfTwo_Ceil(source.QueryValue(i) + 23);
}

extern (C) export MyDComponent CreateComponent()
{
    ezLog.Print("\n\nCreating MyDComponent\n\n");

    MyDComponent c = new MyDComponent();
    core.memory.GC.addRoot(cast(void*)c);

    return c;
}

*/