#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <assert.h>

#define COBSC_EOF 0x00

typedef struct CobsCEncoderCtx {
	uint8_t        *dst;
    size_t          dstSize;
    size_t          dstIndex;
	const uint8_t  *src;
    size_t          srcSize;
    size_t          srcIndex;
	size_t          runLength;
} CobsCEncoderCtx;

static inline void
cobsCEncodeContinue(CobsCEncoderCtx *ctx)
{
	assert( ctx->runLength < 2 );
	/* continue/reposition after encoded buffer was flushed but
	 * we want to keep encoding to the same frame; must have
	 * flushed and runLength was 1 or 0.
	 */
	if ( 1 == ctx->runLength ) {
		ctx->dst[0]   = 1;
	}
	ctx->dstIndex = ctx->runLength;
}

static inline void
cobsCEncodeRewind(CobsCEncoderCtx *ctx)
{
	ctx->srcIndex  = 0;
	ctx->dstIndex  = 0;
	ctx->runLength = 0;
}

static inline void
cobsCEncodeInit(CobsCEncoderCtx *ctx)
{
	/* buffers + sizes must be set by user */
	cobsCEncodeRewind(ctx);
}

/* The encoder ensures there is space to call this
 * routine *once*; after that either cobsCEncode()
 * must be called (to append more data) and/or the buffer
 * flushed if cobsCEncode() returns zero or right away.
 *
 * e.g.,:
 *
 *    cobsCEncodeAppendEOF(&ctx);
 *    flush( ctx.dst, ctx.dstIndex);
 *    cobsCEncodeRewind(&ctx);
 */
static inline void
cobsCEncodeAppendEOF(CobsCEncoderCtx *ctx)
{
	ctx->dst[ctx->dstIndex] = COBSC_EOF;
	++ctx->dstIndex;
	ctx->runLength = 0;
}


/* Convenience wrapper:
 *   1. append more data (data,size) to a frame; routime may be
 *      called multiple times (wrap == 0).
 *   2. wrapup an exising frame; append EOF and send.
 *
 * 1. and 2. may be combined (size > 0, wrap != 0) or used
 * separately (size or wrap may be zero).
 */
int
cobsCEncodeAddToFrame(
	CobsCEncoderCtx *ectx,
	const uint8_t *data,
	size_t size,
	int wrap,
	int (*flush)(const uint8_t *, size_t, void *closure),
	void *closure);

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
 */

int cobsCEncode(CobsCEncoderCtx *ctx);

typedef struct CobsCDecoderCtx {
	uint8_t        *dst;
    size_t          dstSize;
    size_t          dstIndex;
	const uint8_t  *src;
    size_t          srcSize;
    size_t          srcIndex;
	size_t          runLength;
	int             replace;
} CobsCDecoderCtx;

static inline void
cobsCDecodeContinue(CobsCDecoderCtx *ctx)
{
	ctx->srcIndex  = 0;
	ctx->dstIndex  = 0;
}

static inline void
cobsCDecodeRewind(CobsCDecoderCtx *ctx)
{
	ctx->srcIndex  = 0;
	ctx->dstIndex  = 0;
	ctx->runLength = 0;
	ctx->replace   = 0;
}

static inline void
cobsCDecodeInit(CobsCDecoderCtx *ctx)
{
	cobsCDecodeRewind(ctx);
}

/* Returns nonzero when a frame ends;
 * otherwise the user has to check srcIndex and dstIndex
 * to find out whether the source or destination (or both)
 * are exhausted.
 */
int cobsCDecode(CobsCDecoderCtx *ctx);

#ifdef __cplusplus
}
#endif
