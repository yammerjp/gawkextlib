#include "config.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include "gawkextlib.h"
#include "unused.h"

#ifndef SKIP_MAIN_FILE_DECLARATIONS
const gawk_api_t *api;	/* for convenience macros to work */
awk_ext_id_t *ext_id;
int plugin_is_GPL_compatible;
#else
extern const gawk_api_t *api;	/* for convenience macros to work */
extern awk_ext_id_t *ext_id;
#endif

#ifdef HAVE_LIBINTL_H
#include <libintl.h>
#define _(msgid)  dgettext(PACKAGE, msgid)
#else
#define _(msgid)  msgid
#endif


/* same as gawkapi.h:make_null_string but avoids needless memset */
static inline awk_value_t *
make_nul_string(awk_value_t *result)
{
	result->val_type = AWK_UNDEFINED;
	return result;
}

/* performance tweak: */
#define make_null_string(X) make_nul_string(X)

#define RET_NULSTR	return make_null_string(result)
#define RET_NUM(X)	return make_number((X), result)


#define set_ERRNO(X)		update_ERRNO_string(X)

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define make_string_malloc(str, len, result)	\
	r_make_string(api, ext_id, str, len, 1, result)

#define make_string_no_malloc(str, len, result)	\
	r_make_string(api, ext_id, str, len, 0, result)

#ifndef SKIP_MAIN_FILE_DECLARATIONS
#define GAWKEXTLIB_COMMON_INIT { \
	if (!bindtextdomain(PACKAGE, LOCALEDIR)) \
		warning(ext_id, _("bindtextdomain(`%s', `%s') failed"), \
			PACKAGE, LOCALEDIR); \
}
#endif


/* Convenience macro to check the number of arguments */
#define check_nargs(name, min, max) { \
    if (nargs < min)  \
        fatal(ext_id, _("%s: requires at least %d argument(s)"), name, min); \
    else if (do_lint && max >= min && nargs > max) \
        lintwarn(ext_id, _("%s: called with more than %d argument(s)"), name, max); \
}