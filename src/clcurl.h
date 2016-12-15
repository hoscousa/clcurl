
#ifndef CLCURL_H
#define CLCURL_H

#include "clcontext.h"
#include "Hash.h"



typedef struct {
	CLContext clctx;
	cl_event ev;
} CLCurl;

//CLCurl init_clcurl();
void init_clcurl(CLCurl *curl);
void clcurl_destroy(CLCurl *curl);
void cl_absorb  (CLCurl *ctx, trit_t *const trits, size_t trit_len, size_t offset, size_t length);
void cl_squeeze (CLCurl *ctx, trit_t *const trits, size_t trit_len, size_t offset, size_t length);
void cl_reset(CLCurl *ctx);

#endif
