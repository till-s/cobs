#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cobsC.h>

#undef   NDEBUG
#include <assert.h>

using V = std::vector<uint8_t>;

int
vecCodecTest(const V &src, size_t encWin, size_t decWin)
{
	V dst;
	V ebuf;
	dst.resize(src.size());
	ebuf.resize(encWin);

	CobsCEncoderCtx ectx;
	CobsCDecoderCtx dctx;
	cobsCEncodeInit(&ectx);
	cobsCDecodeInit(&dctx);

	ectx.src     = &src[0];
	ectx.srcSize = 

}

int
main(int argc, char **argv)
{
	V src;
	V dst;
	V buf;
	V enc;

	src.resize(1000);
	dst.resize(1000);
	for (auto it = src.begin(); it != src.end(); ++it) {
		*it = (uint8_t)lrand48();
	}
	buf.resize(300);
	CobsCEncoderCtx ectx;
	CobsCDecoderCtx dctx;
	cobsCEncodeInit(&ectx);
	cobsCDecodeInit(&dctx);
	ectx.src     = &src[0];
	ectx.srcSize = src.size();
	ectx.dst     = &buf[0];
	ectx.dstSize = buf.size();

	dctx.src     = &buf[0];
	dctx.dst     = &dst[0];

	bool done;
	do {
		done = cobsCEncode( &ectx );
		for ( auto i = 0; i < ectx.dstIndex; ++i ) {
			enc.push_back(buf[i]);
		}
		dctx.srcSize = ectx.dstIndex;
		dctx.dstSize = dst.size() - (dctx.dst - &dst[0]);
		cobsCDecode(&dctx);
		assert( done || dctx.srcIndex == dctx.srcSize );

		dctx.dst    += dctx.dstIndex;
		if ( ! done ) {
			cobsCEncodeContinue( &ectx );
			cobsCDecodeContinue( &dctx );
		}
	} while ( ! done );
	int e = src.size();
	for ( int i = 0; i < e; ++i ) {
		printf("0x%04x 0x%02x - 0x%02x - 0x%02x\n", i, src[i], dst[i], enc[i]);
		if ( src[i] != dst[i] ) {
			printf("Mismatch @ %d\n", i);
			if ( e == src.size() ) {
				e = i + 5;
			}
		}
	}

	return 0;
}

