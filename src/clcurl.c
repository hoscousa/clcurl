#include "clcurl.h"
#include "curl.cl.h"
#include "curl.h.h"
#include <stdio.h>
#include <string.h>

typedef cl_ulong cl_trit_t;

void transform(trit_t *const state);

void init_clcurl(CLCurl *curl) {
	if(!curl) {
		curl = malloc(sizeof(CLCurl));
	}

	curl->clctx.kernel.src = (unsigned char*[]){ curl_h, curl_cl};
	curl->clctx.kernel.size = (size_t []){ curl_h_len, curl_cl_len };
	curl->clctx.kernel.names = (char *[]){"absorb", "squeeze"};
	curl->clctx.kernel.buffer = (BufferInfo [])
	{
		{sizeof(cl_trit_t)* STATE_LENGTH, CL_MEM_READ_WRITE},/* state */
			{sizeof(cl_trit_t)*TRANSACTION_LENGTH, CL_MEM_READ_WRITE},/* trit buffer (resizable) */
			{sizeof(size_t), CL_MEM_READ_WRITE},/* offset */
			{sizeof(size_t), CL_MEM_READ_ONLY}/* top */
	};
	curl->clctx.kernel.num_src = 2;
	curl->clctx.kernel.num_kernels = 2;
	curl->clctx.kernel.num_buffers = 4;
	init_cl(&(curl->clctx));
}
void clcurl_destroy(CLCurl *curl) {
	finalize_cl(&(curl->clctx));
}

void cl_absorb(CLCurl *ctx, trit_t *const trits, size_t trit_len, size_t offset, size_t length) {

	cl_event ev/*, evb[3]*/;
	const size_t local_work_size = HASH_LENGTH;
	const size_t global_work_size = local_work_size * 1;
	const size_t global_work_offset = 1;
	CLContext *cl = &(ctx->clctx);
	size_t top = offset + length;

	if(ctx->ev != NULL) {
		clWaitForEvents(1, &(ctx->ev));
	}
	clEnqueueWriteBuffer(cl->clcmdq[0], cl->buffers[0][1], CL_TRUE, 0, sizeof(trit_t)*trit_len, trits, 0, NULL, NULL);
	clEnqueueWriteBuffer(cl->clcmdq[0], cl->buffers[0][2], CL_TRUE, 0, sizeof(size_t), &offset, 0, NULL, NULL);
	clEnqueueWriteBuffer(cl->clcmdq[0], cl->buffers[0][3], CL_TRUE,  0, sizeof(size_t), &top, 0, NULL, NULL);

	clEnqueueNDRangeKernel(cl->clcmdq[0], cl->clkernel[0][0], 1, &global_work_offset, &global_work_size, &local_work_size, 0, NULL,&ev);

	ctx->ev = ev;
}

void cl_squeeze(CLCurl *ctx, trit_t *const trits, size_t trit_len, size_t offset, size_t length) {

	cl_event ev/*, evb[3]*/;
	const size_t local_work_size = HASH_LENGTH;
	const size_t global_work_size = local_work_size * 1;
	const size_t global_work_offset = 0;
	size_t top = offset + length;
	CLContext *cl = &(ctx->clctx);

	/*
	   clEnqueueWriteBuffer(cl->clcmdq[0], cl->buffers[0][1], CL_TRUE,  0, sizeof(trit_t) * trit_len, trits, ctx->ev == NULL? 0:1, &(ctx->ev), &evb[0]);
	   clEnqueueWriteBuffer(cl->clcmdq[0], cl->buffers[0][2], CL_TRUE, 0, sizeof(size_t), &offset, ctx->ev == NULL? 0:1, &(ctx->ev), &evb[1]);
	   clEnqueueWriteBuffer(cl->clcmdq[0], cl->buffers[0][3], CL_TRUE, 0, sizeof(size_t), &length, ctx->ev == NULL? 0:1, &(ctx->ev), &evb[2]);
	   clEnqueueNDRangeKernel (cl->clcmdq[0], cl->clkernel[0][1], 1, &global_work_offset, &global_work_size,&local_work_size, 3, evb, &ev);
	   if(ctx->ev != NULL) {
	   clWaitForEvents(1, &(ctx->ev));
	   }
	   */
	clEnqueueWriteBuffer(cl->clcmdq[0], cl->buffers[0][1], CL_TRUE,  0, sizeof(trit_t) * trit_len, trits, ctx->ev != NULL? 1:0, ctx->ev != NULL?&(ctx->ev):NULL, NULL);
	clEnqueueWriteBuffer(cl->clcmdq[0], cl->buffers[0][2], CL_TRUE, 0, sizeof(size_t), &offset, 0,NULL, NULL);
	clEnqueueWriteBuffer(cl->clcmdq[0], cl->buffers[0][3], CL_TRUE, 0, sizeof(size_t), &top, 0,NULL, NULL);

	clEnqueueNDRangeKernel (cl->clcmdq[0], cl->clkernel[0][1], 1, &global_work_offset, &global_work_size,&local_work_size, 0, NULL, &ev);

	clEnqueueReadBuffer(cl->clcmdq[0], cl->buffers[0][1], CL_TRUE, 0, sizeof(trit_t) * trit_len,trits, 1, &ev, NULL);
	ctx->ev = NULL;//evr;
}

void cl_reset(CLCurl *ctx) {
	size_t i;
	trit_t state[STATE_LENGTH], trit[TRANSACTION_LENGTH];
	memset(state,0,STATE_LENGTH*sizeof(trit_t));
	memset(trit,0,STATE_LENGTH*sizeof(trit_t));
	if(ctx->ev != NULL) {
		clWaitForEvents(1, &(ctx->ev));
	}
	for(i = 0; i < ctx->clctx.num_devices; i++) {
		clEnqueueWriteBuffer(ctx->clctx.clcmdq[i], ctx->clctx.buffers[i][1], CL_TRUE, 0, sizeof(trit_t)*TRANSACTION_LENGTH, trit, 0, NULL, NULL);
		clEnqueueWriteBuffer(ctx->clctx.clcmdq[i], ctx->clctx.buffers[i][0], CL_TRUE, 0, STATE_LENGTH*sizeof(trit_t), state, 0, NULL, NULL);
	}
	ctx->ev = NULL;
}
