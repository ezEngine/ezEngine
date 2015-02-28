module main;
import core.exception;
import DTest.TestFramework;

extern(C++) int ezTestMain(int argc, char **argv);

int main(string[] args)
{
	char*[] cargs = new char*[](args.length);
	foreach(size_t i, arg; args)
	{
		cargs[i] = (args[i] ~ "\0").dup.ptr;
	}
	return ezTestMain(cast(int)cargs.length, cargs.ptr);
}