#if defined(__GNUC__) || defined(__clang__)
# define DLL_PUBLIC __attribute__ ((dllexport))
#else
# define DLL_PUBLIC __declspec(dllexport)
#endif
DLL_PUBLIC int libtest_main()
{
	return 0;
}
