#include <arm_neon.h>

typedef int32x4_t __m128i;
typedef float32x4_t __m128;

#define _MM_SHUFFLE(fp3,fp2,fp1,fp0) (((fp3) << 6) | ((fp2) << 4) | ((fp1) << 2) | ((fp0)))
#define __constrange(a,b) const

#define FORCE_INLINE static inline __attribute__((always_inline))

#define _mm_cvtsi128_si32(a) vgetq_lane_s32((a), 0)
#define _mm_cvtsi128_si64(a) vgetq_lane_s64(vreinterpretq_s64_s32(a), 0)

#define _mm_xor_si128(a, b) veorq_s32((a), (b))

#define _mm_slli_si128(a, imm) \
({ \
	__m128i ret; \
	if ((imm) <= 0) { \
		ret = a; \
	} \
	else if ((imm) > 15) { \
		ret = _mm_setzero_si128(); \
	} \
	else { \
		ret = vreinterpretq_s32_s8(vextq_s8(vdupq_n_s8(0), vreinterpretq_s8_s32(a), 16 - (imm))); \
	} \
	ret; \
})

#define _mm_prefetch(...)

FORCE_INLINE __m128i _mm_set_epi32(int i3, int i2, int i1, int i0)
{
	const int32_t __attribute__((aligned(16))) data[4] = { i0, i1, i2, i3 };
	return vld1q_s32(data);
}

FORCE_INLINE __m128i _mm_set_epi64x(int64_t i1, int64_t i0)
{
	const int64_t __attribute__((aligned(16))) data[2] = { i0, i1 };
	return vreinterpretq_s32_s64(vld1q_s64(data));
}

FORCE_INLINE __m128i _mm_setzero_si128()
{
	return vdupq_n_s32(0);
}

FORCE_INLINE __m128i _mm_load_si128(const __m128i *p)
{
	return vld1q_s32((int32_t *)p);
}

FORCE_INLINE void _mm_store_si128(__m128i *p, __m128i a)
{
	vst1q_s32((int32_t*) p, a);
}

#define _mm_aeskeygenassist_si128 soft_aeskeygenassist

FORCE_INLINE __m128i _mm_aesenc_si128(__m128i v, __m128i rkey)
{
    static const __attribute__((aligned(16))) uint8x16_t zero = {0};
    return vreinterpretq_s32_u8(veorq_u8(vaesmcq_u8(vaeseq_u8(vreinterpretq_u8_s32(v), zero)), vreinterpretq_u8_s32(rkey)));
}

// Takes the upper 64 bits of a and places it in the low end of the result
// Takes the lower 64 bits of a and places it into the high end of the result.
FORCE_INLINE __m128i _mm_shuffle_epi_1032(__m128i a)
{
	int32x2_t a32 = vget_high_s32(a);
	int32x2_t a10 = vget_low_s32(a);
	return vcombine_s32(a32, a10);
}

// takes the lower two 32-bit values from a and swaps them and places in low end of result
// takes the higher two 32 bit values from a and swaps them and places in high end of result.
FORCE_INLINE __m128i _mm_shuffle_epi_2301(__m128i a)
{
	int32x2_t a01 = vrev64_s32(vget_low_s32(a));
	int32x2_t a23 = vrev64_s32(vget_high_s32(a));
	return vcombine_s32(a01, a23);
}

// rotates the least significant 32 bits into the most signficant 32 bits, and shifts the rest down
FORCE_INLINE __m128i _mm_shuffle_epi_0321(__m128i a)
{
	return vextq_s32(a, a, 1);
}

// rotates the most significant 32 bits into the least signficant 32 bits, and shifts the rest up
FORCE_INLINE __m128i _mm_shuffle_epi_2103(__m128i a)
{
	return vextq_s32(a, a, 3);
}

// gets the lower 64 bits of a, and places it in the upper 64 bits
// gets the lower 64 bits of a and places it in the lower 64 bits
FORCE_INLINE __m128i _mm_shuffle_epi_1010(__m128i a)
{
	int32x2_t a10 = vget_low_s32(a);
	return vcombine_s32(a10, a10);
}

// gets the lower 64 bits of a, swaps the 0 and 1 elements, and places it in the lower 64 bits
// gets the lower 64 bits of a, and places it in the upper 64 bits
FORCE_INLINE __m128i _mm_shuffle_epi_1001(__m128i a)
{
	int32x2_t a01 = vrev64_s32(vget_low_s32(a));
	int32x2_t a10 = vget_low_s32(a);
	return vcombine_s32(a01, a10);
}

// gets the lower 64 bits of a, swaps the 0 and 1 elements and places it in the upper 64 bits
// gets the lower 64 bits of a, swaps the 0 and 1 elements, and places it in the lower 64 bits
FORCE_INLINE __m128i _mm_shuffle_epi_0101(__m128i a)
{
	int32x2_t a01 = vrev64_s32(vget_low_s32(a));
	return vcombine_s32(a01, a01);
}

FORCE_INLINE __m128i _mm_shuffle_epi_2211(__m128i a)
{
	int32x2_t a11 = vdup_lane_s32(vget_low_s32(a), 1);
	int32x2_t a22 = vdup_lane_s32(vget_high_s32(a), 0);
	return vcombine_s32(a11, a22);
}

FORCE_INLINE __m128i _mm_shuffle_epi_0122(__m128i a)
{
	int32x2_t a22 = vdup_lane_s32(vget_high_s32(a), 0);
	int32x2_t a01 = vrev64_s32(vget_low_s32(a));
	return vcombine_s32(a22, a01);
}

FORCE_INLINE __m128i _mm_shuffle_epi_3332(__m128i a)
{
	int32x2_t a32 = vget_high_s32(a);
	int32x2_t a33 = vdup_lane_s32(vget_high_s32(a), 1);
	return vcombine_s32(a32, a33);
}

FORCE_INLINE __m128i _mm_shuffle_epi32_default(__m128i a, __constrange(0,255) int imm)
{
	__m128i ret;
	ret[0] = a[imm & 0x3];
	ret[1] = a[(imm >> 2) & 0x3];
	ret[2] = a[(imm >> 4) & 0x03];
	ret[3] = a[(imm >> 6) & 0x03];
	return ret;
}

#define _mm_shuffle_epi32_splat(a, imm) \
({ \
	vdupq_laneq_s32(a, (imm)); \
})

#define _mm_shuffle_epi32(a, imm) \
({ \
	__m128i ret; \
	switch (imm) \
	{ \
		case _MM_SHUFFLE(1, 0, 3, 2): ret = _mm_shuffle_epi_1032((a)); break; \
		case _MM_SHUFFLE(2, 3, 0, 1): ret = _mm_shuffle_epi_2301((a)); break; \
		case _MM_SHUFFLE(0, 3, 2, 1): ret = _mm_shuffle_epi_0321((a)); break; \
		case _MM_SHUFFLE(2, 1, 0, 3): ret = _mm_shuffle_epi_2103((a)); break; \
		case _MM_SHUFFLE(1, 0, 1, 0): ret = _mm_shuffle_epi_1010((a)); break; \
		case _MM_SHUFFLE(1, 0, 0, 1): ret = _mm_shuffle_epi_1001((a)); break; \
		case _MM_SHUFFLE(0, 1, 0, 1): ret = _mm_shuffle_epi_0101((a)); break; \
		case _MM_SHUFFLE(2, 2, 1, 1): ret = _mm_shuffle_epi_2211((a)); break; \
		case _MM_SHUFFLE(0, 1, 2, 2): ret = _mm_shuffle_epi_0122((a)); break; \
		case _MM_SHUFFLE(3, 3, 3, 2): ret = _mm_shuffle_epi_3332((a)); break; \
		case _MM_SHUFFLE(0, 0, 0, 0): ret = _mm_shuffle_epi32_splat((a),0); break; \
		case _MM_SHUFFLE(1, 1, 1, 1): ret = _mm_shuffle_epi32_splat((a),1); break; \
		case _MM_SHUFFLE(2, 2, 2, 2): ret = _mm_shuffle_epi32_splat((a),2); break; \
		case _MM_SHUFFLE(3, 3, 3, 3): ret = _mm_shuffle_epi32_splat((a),3); break; \
		default: ret = _mm_shuffle_epi32_default((a), (imm)); break; \
	} \
	ret; \
})

FORCE_INLINE __m128i _mm_shufflelo_epi16(__m128i a, __constrange(0,255) int imm)
{
	int16x8_t ret = vreinterpretq_s16_s32(a);
	ret[0] = vreinterpretq_s16_s32(a)[imm & 0x3];
	ret[1] = vreinterpretq_s16_s32(a)[(imm >> 2) & 0x3];
	ret[2] = vreinterpretq_s16_s32(a)[(imm >> 4) & 0x03];
	ret[3] = vreinterpretq_s16_s32(a)[(imm >> 6) & 0x03];
	return vreinterpretq_s32_s16(ret);
}
