declare function Print(text : string) : void;

export module Foo 
{
    export function GetFoo(): string 
    {
        return "Foo"
    }

    Print("Foo: " + GetFoo())
}
