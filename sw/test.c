#include <cobsC.h>
#include <stdio.h>
#include <string.h>

#undef NDEBUG
#include <assert.h>

static void
setup(uint8_t *src, size_t size, int zeroIdx)
{
	memset(src, 0xff, size);
	if ( zeroIdx >= 0) {
		src[zeroIdx] = 0x00;
	}
}

static void testSingleRun(CobsCEncoderCtx *ctx, size_t sz, int zeroIdx)
{
	int i;
	if ( zeroIdx < 0 ) {
		zeroIdx = -1;
	}
	setup((uint8_t*)ctx->src, sz, zeroIdx);
	ctx->srcSize = sz;
	cobsCEncodeRewind(ctx);
	assert( cobsCEncode(ctx) );
	assert( ctx->srcIndex == 0 );
	assert( ctx->dstIndex == sz + 1 );
	if ( zeroIdx >= 0 ) {
		assert( ctx->srcRemain == ctx->dst[zeroIdx + 1] );
		assert( ctx->dst[0] == zeroIdx + 1 );
	} else {
		assert( ctx->srcRemain == 254 == sz ? 0 : sz + 1 );
		assert( ctx->dst[0] == sz + 1 );
	}
	for ( i = 0; i < sz; ++i ) {
		if ( i == zeroIdx ) {
			assert( ctx->dst[i + 1] + zeroIdx + 1 == sz + 1 );
	 	} else {
			assert( ctx->dst[i + 1] == 0xff );
		}
	}
}

int
main(int argc, char **argv)
{
	uint8_t dst[1024];
	uint8_t src[1024];
	CobsCEncoderCtx ctx;
	cobsCEncodeInit(&ctx);
	ctx.dst     = dst;
	ctx.dstSize = sizeof(dst);
	ctx.src     = src;
	ctx.srcSize = sizeof(src);
	testSingleRun(&ctx, 1, -1);
	testSingleRun(&ctx, 1,  0);
	testSingleRun(&ctx, 254, -1);
	testSingleRun(&ctx, 254, 254);
	testSingleRun(&ctx, 254, 0);
	testSingleRun(&ctx, 254, 10);
}
