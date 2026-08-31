#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cobsC.h>

#undef   NDEBUG
#include <assert.h>

using V = std::vector<uint8_t>;

int
decodeSeg(CobsCDecoderCtx *dctx, size_t *premain)
{
	int rv = 0;
	assert( *premain > 0 );

	if ( *premain < dctx->dstSize ) {
		dctx->dstSize = *premain;
	}

	while ( dctx->srcIndex < dctx->srcSize ) {
		if ( cobsCDecode( dctx ) ) {
			rv = 1;
			break;
		}
		*premain      -= dctx->dstIndex;
		dctx->dst     += dctx->dstIndex;
		dctx->dstIndex = 0;
	}
	return 0;
}

void
vecCodecTest(const V &src, size_t encWin, size_t bufsz, size_t decWin, const V *eexp)
{
	V dst;
	V buf;
	V enc;
	dst.resize(src.size());
	buf.resize(bufsz);

	CobsCEncoderCtx ectx;
	CobsCDecoderCtx dctx;
	cobsCEncodeInit(&ectx);
	cobsCDecodeInit(&dctx);

	ectx.src     = &src[0];
	ectx.srcSize = encWin;
	ectx.dst     = &buf[0];
	ectx.dstSize = bufsz;

	dctx.src     = ectx.dst;
	dctx.srcSize = ectx.dstSize;
	dctx.dst     = &dst[0];
	dctx.dstSize = decWin;

	size_t eremain = src.size();
	size_t dremain = dst.size();
	bool   edone;
	while ( eremain > 0 ) {
		size_t lidx = 0;
		while ( !  cobsCEncode( &ectx ) ) {
			if ( eexp ) {
				for ( auto k = 0; k < ectx.dstIndex; ++k  ) {
					enc.push_back( ectx.dst[k] );
				}
			}
			dctx.srcSize = ectx.dstIndex;

			if ( decodeSeg( &dctx, &dremain ) ) {
				goto all_done;
			}

			dctx.srcIndex = 0;
			cobsCEncodeContinue(&ectx);
		}
		ectx.src += ectx.srcSize;
		eremain  -= ectx.srcSize;
		if ( eremain < ectx.srcSize ) {
			ectx.srcSize = eremain;
		}
	}
	if ( eexp ) {
		for ( auto k = 0; k < ectx.dstIndex; ++k  ) {
			enc.push_back( ectx.dst[k] );
		}
		printf("Total encoded length %zd, expected %zd\n", enc.size(), eexp->size());
		assert( enc.size() == eexp->size() );
		for ( int k = 0; k < enc.size(); ++k ) {
			assert(enc[k] == (*eexp)[k]);
		}
	}

	dctx.srcSize = ectx.dstIndex;
	decodeSeg( &dctx, &dremain );
	assert( dremain == 0 );
all_done:
	for ( int i = 0; i < src.size(); ++i ) {
		assert(dst[i] == src[i]);
	}
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

	vecCodecTest(src, 100, 300, 100, &enc);

	return 0;
}

