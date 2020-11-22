#pragma once


#ifdef _MSC_VER
	#define native_main wmain
	#define MK_SEH_TRY __try
	#define MK_SEH_EXCEPT __except(1) /* EXCEPTION_EXECUTE_HANDLER */
	typedef wchar_t nchar;
#else
	#define native_main main
	#define MK_SEH_TRY
	#define MK_SEH_EXCEPT while(false)
	typedef char nchar;
#endif
