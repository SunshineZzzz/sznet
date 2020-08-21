#include "NetCmn.h"

namespace sznet
{

void sztool_memcheck()
{
#ifdef SZ_OS_WINDOWS
	int t = _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	t |= _CRTDBG_LEAK_CHECK_DF;
	_CrtSetDbgFlag(t);
#endif
}

void sztool_setbreakalloc(int size)
{
#ifdef SZ_OS_WINDOWS
	_CrtSetBreakAlloc(size);
#endif
}

} // end namespace sznet