#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <assert.h>

typedef struct CobsCEncoderCtx {
	uint8_t        *dst;
    size_t          dstSize;
    size_t          dstIndex;
	const uint8_t  *src;
    size_t          srcSize;
    size_t          srcIndex;
	size_t          srcRemain;
} CobsCEncoderCtx;

static inline void
cobsCEncodeContinue(CobsCEncoderCtx *ctx, int cont)
{
	assert( ctx->srcRemain < 2 );
	/* continue/reposition after encoded buffer was flushed but
	 * we want to keep encoding to the same frame; must have
	 * flushed and srcRemain was 1 or 0.
	 */
	if ( 1 == ctx->srcRemain ) {
		ctx->dst[0]   = 1;
	}
	ctx->dstIndex = ctx->srcRemain;
}

static inline void
cobsCEncodeRewind(CobsCEncoderCtx *ctx)
{
	ctx->srcIndex  = 0;
	ctx->dstIndex  = 0;
	ctx->srcRemain = 0;
}

static inline void
cobsCEncodeInit(CobsCEncoderCtx *ctx)
{
	/* buffers + sizes must be set by user */
	cobsCEncodeRewind(ctx);
}


#define COBSC_EOF 0x00

/* Encode the 'src' buffer into the 'dst' buffer.
 * Encoding may stop either because the source is exhausted
 * or the destination is full.
 * The reason is reflected in the return value: 1 if the
 * source is exhausted and 0 if the destination buffer is full.
 * Note that 'full' means that another segment migth not fit.
 * The encoder stops at a segment boundary and returns 0
 * leaving some of the source unconsumed.
 * In either case, encoding may proceed:
 *  1. if the destination was full then flush the destination
 *     buffer to the transmitter and 'continue' the encoder state:
 *
 *         while ( ! cobsCEncode(ctx)0 ) {
 *            flush(ctx->dst, cts->dstIndex);
 *            cobsCEncodeContinue(ctx);
 *         }
 *
 *  2. if the source is exhausted then
 *     a. the source buffer can be refilled or
 *        changed and encoding resumed.
 *     b. eventually, an EOF may be appended
 *        (the encoder ensures there is space in the
 *        destination buffer), the destination
 *        flushed and rewound.
 *
 *     Using an 'iovec'-style array:
 *
 *         cobsCEncodeInit(ctx);
 *         for ( n = 0; n < NUM_IOVS; ++n ) {
 *            ctx->src     = iov[n].buf;
 *            ctx->srcSize = iov[n].size;
 *            while ( ! cobsCEncode(ctx) ) {
 *               flush(ctx->dst, cts->dstIndex);
 *               cobsCEncodeContinue(ctx);
 *            }
 *         }
 *         ctx->dst[ctx->dstIndex++] = COBSC_EOF;
 *         flush(ctx->dst, ctx->dstIndex);
 *         cobsCEncodeRewind(ctx);
 *
 * NOTE: if a protocol is guaranteed to always have frames <= 254
 *       then the buffer may be encoded 'in place' provided that
 *       there is space for a header byte:
 *         ctx->dst     = inPlaceBuffer;
 *         ctx->dstSize = 256;
 *         ctx->src     = inPlaceBuffer + 1;
 *         ctx->srcSize = 254;
 */

int cobsCEncode(CobsCEncoderCtx *ctx);

#ifdef __cplusplus
}
#endif
