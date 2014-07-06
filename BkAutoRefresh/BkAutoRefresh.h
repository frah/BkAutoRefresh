#ifdef BKAUTOREFRESH_EXPORTS
#define BKAUTOREFRESH_API __declspec(dllexport)
#else
#define BKAUTOREFRESH_API __declspec(dllimport)
#endif

#define BK_COMMAND_RELOAD_QUERY		32998

#define RFC2646_SIGNATURE_SEPARATOR	"-- "
#define RFC2646_QUOTE_INDICATOR		'>'
