#ifdef BKAUTOREFRESH_EXPORTS
#define BKAUTOREFRESH_API __declspec(dllexport)
#else
#define BKAUTOREFRESH_API __declspec(dllimport)
#endif

#define RFC2646_SIGNATURE_SEPARATOR	"-- "
#define RFC2646_QUOTE_INDICATOR		'>'

#ifdef __cplusplus
extern "C"{
#endif

extern int strcount(const char* str, const char c);
extern bool ends_with(const char* src, const char* val);

#ifdef __cplusplus
}
#endif
