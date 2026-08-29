#include <cobsC.h>

int
cobsCEncode(CobsCEncoderCtx *ctx)
{
	/* when a run is started the encoder ensures there is space for
	 * one max. segment + a terminating EOF, i.e., 256 bytes;
	 * thus, if there are remaining data there must be space
	 * in the buffer.
	 */
	const uint8_t *srcp;
	uint8_t       *dstp;
	const uint8_t *srcend;
	uint8_t       *dstend;
	uint8_t       *lenp;
	size_t         runLength;
	size_t         runMax;

	assert( ctx->dstSize >= SEG_MAX );

	srcp   = ctx->src + ctx->srcIndex;
	dstp   = ctx->dst + ctx->dstIndex;
	srcend = ctx->src + ctx->srcSize;
	dstend = ctx->dst + ctx->dstSize - SEG_MAX;

	if ( srcp == srcend ) {
		/* no work */
		return 1;
	}

	runLength  = ctx->srcRemain;

	while ( dstp <= dstend ) {
		/* remember where to store next link
		 *  - when a segment was ended by a EOF then this is the link field
		 *    but runLength + dstp have already been incremented past the EOF
		 *  - when a segment ended due to max-run then the link field is
		 *    past the end of the segment; dstp + runLength have not been
		 *    incremented
		 * in both cases destp-runLength points to the correct location
		 */
		lenp = dstp - runLength;
		if ( 0 == runLength ) {
			/* need header */
			runLength = 1;
			++dstp;
		}
		runMax = (srcend - srcp) + runLength;
		if ( runMax > RUN_MAX ) {
			runMax = RUN_MAX;
		}
		/* initially, runMax > runLength because srcp < srcend was tested above and before hitting 'again' */
		while ( COBSC_EOF != (*dstp++ = *srcp++) ) {
			if ( ++runLength >= runMax ) {
				/* source exhausted or max segment reached or both */
				/* store link */
				*lenp = runLength;
				if ( RUN_MAX == runLength ) {
					/* need a new header */
					runLength = 0;
					/* might be a max-length run */
					if ( srcp < srcend ) {
						goto continue_outer_loop;
					}
					/* else fall through and break outer loop */
				}
				/* if runLength < RUN_MAX then source is surely exhausted */
				goto break_outer_loop;
			}
		}
		*lenp     = runLength;
		runLength = 1;
continue_outer_loop:
	}
break_outer_loop:
	ctx->srcIndex  = srcp - ctx->src;
	ctx->dstIndex  = dstp - ctx->dst;
	ctx->srcRemain = runLength;
	if ( srcp < srcend ) {
		/* strip the space that was reserved for the next link header */
		ctx->dstIndex--;
		return 0;
	}
	/* srcp == srcend */
	ctx->srcIndex = 0; /* prepare for new source */
	return 1;
}


