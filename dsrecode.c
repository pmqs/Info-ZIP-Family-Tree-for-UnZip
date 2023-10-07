#include <librcc.h>

static rcc_class_default_charset default_oem[] =
{
    { "ru", "IBM866" },
    { NULL, NULL }
};

static rcc_class_default_charset default_iso[] =
{
    { "ru", "CP1251" },
    { NULL, NULL }
};

#define OEM_CLASS 0
#define ISO_CLASS 1
#define OUT_CLASS 2
static rcc_class classes[] =
{
    { "oem", RCC_CLASS_STANDARD, NULL, default_oem, "OEM_INTERN", 0 },
    { "iso", RCC_CLASS_STANDARD, NULL, default_iso, "ISO_INTERN", 0 },
    { "out", RCC_CLASS_STANDARD, "LC_CTYPE", NULL, "Output", 0 },
    { NULL }
};

int initialized = 0;

#ifdef RCC_LAZY
#include <dlfcn.h>
# define RCC_LIBRARY "librcc.so.0"
int (*rccInit2)(void);
int (*rccFree2)(void);
int (*rccInitDefaultContext2)(const char *locale_variable,
                              unsigned int max_languages,
                              unsigned int max_classes,
                              rcc_class_ptr defclasses,
                              rcc_init_flags flags);
int (*rccInitDb42)(rcc_context ctx, const char *name, rcc_db4_flags flags);
char* (*rccSizedRecode2)(rcc_context ctx, rcc_class_id from, rcc_class_id to,
                         const char *buf, size_t len, size_t *rlen);
int (*rccLoad2)(rcc_context ctx, const char *name);


static char *rccRecode2(rcc_context ctx, rcc_class_id from,
                        rcc_class_id to, const char *buf)
{
    return rccSizedRecode2(ctx, from, to, buf, 0, NULL);
}

void *rcc_handle;
#else /* RCC_LAZY */
#define rccInit2 rccInit
#define rccFree2 rccFree
#define rccInitDefaultContext2 rccInitDefaultContext
#define rccInitDb42 rccInitDb4
#define rccRecode2 rccRecode
#define rccLoad2 rccLoad
#endif /* RCC_LAZY */

static void rccUnzipFree(void)
{
    if (initialized > 0) {
	rccFree2();
#ifdef RCC_LAZY
	dlclose(rcc_handle);
#endif /* RCC_LAZY */
	initialized = 0;
    }
}


static int rccUnzipInit(void)
{
    if (initialized) return 0;

#ifdef RCC_LAZY
    rcc_handle = dlopen(RCC_LIBRARY, RTLD_NOW);
    if (!rcc_handle) {
	initialized = -1;
	return 1;
    }

    rccInit2 = dlsym(rcc_handle, "rccInit");
    rccFree2 = dlsym(rcc_handle, "rccFree");
    rccInitDefaultContext2 = dlsym(rcc_handle, "rccInitDefaultContext");
    rccInitDb42 = dlsym(rcc_handle, "rccInitDb4");
    rccSizedRecode2 = dlsym(rcc_handle, "rccSizedRecode");
    rccLoad2 = dlsym(rcc_handle, "rccLoad");

    if ((!rccInit2) || (!rccFree2) || (!rccInitDefaultContext2) ||
        (!rccInitDb42) || (!rccSizedRecode2) || (!rccLoad2)) {
	dlclose(rcc_handle);
	initialized = -1;
	return 1;
    }
#endif /* RCC_LAZY */

    rccInit2();
    rccInitDefaultContext2(NULL, 0, 0, classes, 0);
    rccLoad2(NULL, "zip");
    rccInitDb42(NULL, NULL, 0);
    atexit(rccUnzipFree);
    initialized = 1;
    return 0;
}



void _DS_OEM_INTERN(char *string)
{
    char *str;
    rccUnzipInit();

    if (initialized>0) {
	str = rccRecode2(NULL, OEM_CLASS, OUT_CLASS, string);

	if (str) {
	    strncpy(string,str,FILNAMSIZ);
	    free(str);
	}
    }
}

void _DS_ISO_INTERN(char *string)
{
    char *str;
    rccUnzipInit();

    if (initialized>0) {
	str = rccRecode2(NULL, ISO_CLASS, OUT_CLASS, string);

	if (str) {
	    strncpy(string,str,FILNAMSIZ);
	    free(str);
	}
    }
}
