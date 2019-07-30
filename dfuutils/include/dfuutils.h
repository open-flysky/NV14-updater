#ifdef DFUUTILS_EXPORTS
#define DFUUTILS_API __declspec(dllexport)
#else
#define DFUUTILS_API __declspec(dllimport)
#endif

// Klasa jest wyeksportowana z dfuutils.dll
class DFUUTILS_API Cdfuutils {
public:
	Cdfuutils(void);
	// TODO: W tym miejscu dodaj metody.
};

extern DFUUTILS_API int ndfuutils;

DFUUTILS_API int fndfuutils(void);
