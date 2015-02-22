#ifdef __GNUC__
 #define DLL_PUBLIC __attribute__ ((dllexport))
#else
 #define DLL_PUBLIC __declspec(dllexport) // Note: actually gcc seems to also supports this syntax.
#endif
DLL_PUBLIC int libtest_main()
{
	return 0;
}
