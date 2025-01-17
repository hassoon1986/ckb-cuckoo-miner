#include "blake2b.h"

static const uint64_t blake2b_IV[8] =
{
  0x6a09e667f3bcc908ULL, 0xbb67ae8584caa73bULL,
  0x3c6ef372fe94f82bULL, 0xa54ff53a5f1d36f1ULL,
  0x510e527fade682d1ULL, 0x9b05688c2b3e6c1fULL,
  0x1f83d9abfb41bd6bULL, 0x5be0cd19137e2179ULL
};

/* Some helper functions */
static void _blake2b_set_lastnode( blake2b_state *S )
{
  S->f[1] = (uint64_t)-1;
}

static int _blake2b_is_lastblock( const blake2b_state *S )
{
  return S->f[0] != 0;
}

static void _blake2b_set_lastblock( blake2b_state *S )
{
  if( S->last_node ) _blake2b_set_lastnode( S );

  S->f[0] = (uint64_t)-1;
}

static void _blake2b_increment_counter( blake2b_state *S, const uint64_t inc )
{
  S->t[0] += inc;
  S->t[1] += ( S->t[0] < inc );
}

/* init xors IV with input parameter block */
int _blake2b_init_param( blake2b_state *S, const blake2b_param *P )
{
  size_t i;
  /*blake2b_init0( S ); */
  const unsigned char * v = ( const unsigned char * )( blake2b_IV );
  const unsigned char * p = ( const unsigned char * )( P );
  unsigned char * h = ( unsigned char * )( S->h );
  /* IV XOR ParamBlock */
  memset( S, 0, sizeof( blake2b_state ) );

  for( i = 0; i < BLAKE2B_OUTBYTES; ++i ) h[i] = v[i] ^ p[i];

  S->outlen = P->digest_length;
  return 0;
}


static void _blake2b_compress( blake2b_state *S, const uint8_t block[BLAKE2B_BLOCKBYTES] )
{
  __m128i row1l, row1h;
  __m128i row2l, row2h;
  __m128i row3l, row3h;
  __m128i row4l, row4h;
  __m128i b0, b1;
  __m128i t0, t1;
#if defined(HAVE_SSSE3) && !defined(HAVE_XOP)
  const __m128i r16 = _mm_setr_epi8( 2, 3, 4, 5, 6, 7, 0, 1, 10, 11, 12, 13, 14, 15, 8, 9 );
  const __m128i r24 = _mm_setr_epi8( 3, 4, 5, 6, 7, 0, 1, 2, 11, 12, 13, 14, 15, 8, 9, 10 );
#endif
#if defined(HAVE_SSE41)
  const __m128i m0 = LOADU( block + 00 );
  const __m128i m1 = LOADU( block + 16 );
  const __m128i m2 = LOADU( block + 32 );
  const __m128i m3 = LOADU( block + 48 );
  const __m128i m4 = LOADU( block + 64 );
  const __m128i m5 = LOADU( block + 80 );
  const __m128i m6 = LOADU( block + 96 );
  const __m128i m7 = LOADU( block + 112 );
#else
  const uint64_t  m0 = load64(block +  0 * sizeof(uint64_t));
  const uint64_t  m1 = load64(block +  1 * sizeof(uint64_t));
  const uint64_t  m2 = load64(block +  2 * sizeof(uint64_t));
  const uint64_t  m3 = load64(block +  3 * sizeof(uint64_t));
  const uint64_t  m4 = load64(block +  4 * sizeof(uint64_t));
  const uint64_t  m5 = load64(block +  5 * sizeof(uint64_t));
  const uint64_t  m6 = load64(block +  6 * sizeof(uint64_t));
  const uint64_t  m7 = load64(block +  7 * sizeof(uint64_t));
  const uint64_t  m8 = load64(block +  8 * sizeof(uint64_t));
  const uint64_t  m9 = load64(block +  9 * sizeof(uint64_t));
  const uint64_t m10 = load64(block + 10 * sizeof(uint64_t));
  const uint64_t m11 = load64(block + 11 * sizeof(uint64_t));
  const uint64_t m12 = load64(block + 12 * sizeof(uint64_t));
  const uint64_t m13 = load64(block + 13 * sizeof(uint64_t));
  const uint64_t m14 = load64(block + 14 * sizeof(uint64_t));
  const uint64_t m15 = load64(block + 15 * sizeof(uint64_t));
#endif
  row1l = LOADU( &S->h[0] );
  row1h = LOADU( &S->h[2] );
  row2l = LOADU( &S->h[4] );
  row2h = LOADU( &S->h[6] );
  row3l = LOADU( &blake2b_IV[0] );
  row3h = LOADU( &blake2b_IV[2] );
  row4l = _mm_xor_si128( LOADU( &blake2b_IV[4] ), LOADU( &S->t[0] ) );
  row4h = _mm_xor_si128( LOADU( &blake2b_IV[6] ), LOADU( &S->f[0] ) );
  ROUND( 0 );
  ROUND( 1 );
  ROUND( 2 );
  ROUND( 3 );
  ROUND( 4 );
  ROUND( 5 );
  ROUND( 6 );
  ROUND( 7 );
  ROUND( 8 );
  ROUND( 9 );
  ROUND( 10 );
  ROUND( 11 );
  row1l = _mm_xor_si128( row3l, row1l );
  row1h = _mm_xor_si128( row3h, row1h );
  STOREU( &S->h[0], _mm_xor_si128( LOADU( &S->h[0] ), row1l ) );
  STOREU( &S->h[2], _mm_xor_si128( LOADU( &S->h[2] ), row1h ) );
  row2l = _mm_xor_si128( row4l, row2l );
  row2h = _mm_xor_si128( row4h, row2h );
  STOREU( &S->h[4], _mm_xor_si128( LOADU( &S->h[4] ), row2l ) );
  STOREU( &S->h[6], _mm_xor_si128( LOADU( &S->h[6] ), row2h ) );
}


int b2b_update( blake2b_state *S, const void *pin, size_t inlen )
{
  const unsigned char * in = (const unsigned char *)pin;
  if( inlen > 0 )
  {
    size_t left = S->buflen;
    size_t fill = BLAKE2B_BLOCKBYTES - left;
    if( inlen > fill )
    {
      S->buflen = 0;
      memcpy( S->buf + left, in, fill ); /* Fill buffer */
      _blake2b_increment_counter( S, BLAKE2B_BLOCKBYTES );
      _blake2b_compress( S, S->buf ); /* Compress */
      in += fill; inlen -= fill;
      while(inlen > BLAKE2B_BLOCKBYTES) {
        _blake2b_increment_counter(S, BLAKE2B_BLOCKBYTES);
        _blake2b_compress( S, in );
        in += BLAKE2B_BLOCKBYTES;
        inlen -= BLAKE2B_BLOCKBYTES;
      }
    }
    memcpy( S->buf + S->buflen, in, inlen );
    S->buflen += inlen;
  }
  return 0;
}


int b2b_final( blake2b_state *S, void *out, size_t outlen )
{
  if( out == NULL || outlen < S->outlen )
    return -1;

  if( _blake2b_is_lastblock( S ) )
    return -1;

  _blake2b_increment_counter( S, S->buflen );
  _blake2b_set_lastblock( S );
  memset( S->buf + S->buflen, 0, BLAKE2B_BLOCKBYTES - S->buflen ); /* Padding */
  _blake2b_compress( S, S->buf );

  memcpy( out, &S->h[0], S->outlen );
  return 0;
}


void b2b_setup(blake2b_state *S) {
    int i;

    blake2b_param P;

    P.digest_length = 32;
    P.key_length = 0;
    P.fanout = 1;
    P.depth = 1;
    P.leaf_length = 0;
    P.xof_length = 0;
    P.node_offset = 0;
    P.inner_length = 0;
    P.node_depth = 0;
    
    for(i=0; i<14; ++i) {
        P.reserved[i] = 0;
    }

    for(i=0; i<BLAKE2B_SALTBYTES; ++i) {
        P.salt[i] = 0;
    }

    uint8_t personal[] = {99, 107, 98, 45, 100, 101, 102, 97, 117, 108, 116, 45, 104, 97, 115, 104};

    for(i=0; i<16; ++i) {
        P.personal[i] = personal[i];
    }

    _blake2b_init_param(S, &P);
}