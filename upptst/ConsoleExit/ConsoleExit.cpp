#include <Core/Core.h>

using namespace Upp;

CONSOLE_APP_MAIN
{
	LOG("Hello!");
	Buffer<int> x(200);
	Exit(1);
}
