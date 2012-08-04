#ifndef _DLL_H_
#define _DLL_H_

#ifdef DLL
#define DllExport   __declspec( dllexport ) 
#else
#define DllExport
#endif

#endif
