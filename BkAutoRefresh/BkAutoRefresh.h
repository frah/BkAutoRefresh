#ifdef BKAUTOREFRESH_EXPORTS
#define BKAUTOREFRESH_API __declspec(dllexport)
#else
#define BKAUTOREFRESH_API __declspec(dllimport)
#endif

#define PLUGIN_NAME					"BkAutoRefresh"

#define BK_COMMAND_RELOAD_QUERY		32998
#define BK_COMMAND_COPY				57634

#define RFC2646_SIGNATURE_SEPARATOR	"-- "
#define RFC2646_QUOTE_INDICATOR		'>'

#define MAX_DCLICK_ACTION_NUM		3

typedef struct {
	char* regexp;
	char* command;
} DCLICK_ACTION;
