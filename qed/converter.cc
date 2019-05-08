/*!
 * \file converter.cc
 * \brief Converter impls
 */
#include "qed/converter.h"

#include <memory.h>
#include <immintrin.h>

namespace qed {

bool ConvertHelper<fp32_t, fp32_t>::Assign(fp32_t& dst, const fp32_t& src) {
  dst = src;
  return true;
}
bool ConvertHelper<fp32_t, fp32_t>::Assign(fp32_t* dst,
                                           const fp32_t* src,
                                           size_t length) {
  memcpy(dst, src, sizeof(fp32_t) * length);
  return true;
}
bool ConvertHelper<fp32_t, fp32_t>::AddMA(fp32_t* dst,
                                          const fp32_t* src,
                                          fp32_t a,
                                          size_t length) {
  auto a128 = _mm_load_ps1(&a);
  auto a256 = _mm256_broadcast_ps(&a128);
  size_t i = 0;
  for (; (i + 8) <= length; i += 8) {
    auto s256 = _mm256_loadu_ps(src + i);
    auto mul = _mm256_mul_ps(a256, s256);
    auto d256 = _mm256_loadu_ps(dst + i);
    _mm256_storeu_ps(dst + i, _mm256_add_ps(d256, mul));
  }
  for (; i < length; i++) {
    dst[i] += src[i] * a;
  }
  return true;
}
bool ConvertHelper<fp32_t, fp32_t>::Add(fp32_t* dst,
                                        const fp32_t* src,
                                        size_t length) {
  size_t i = 0;
  for (; (i + 8) <= length; i += 8) {
    auto s256 = _mm256_loadu_ps(src + i);
    auto d256 = _mm256_loadu_ps(dst + i);
    _mm256_storeu_ps(dst + i, _mm256_add_ps(d256, s256));
  }
  for (; i < length; i++) {
    dst[i] += src[i];
  }
  return true;
}

bool ConvertHelper<fp16_t, fp32_t>::Assign(fp16_t* dst,
                                           const fp32_t* src,
                                           size_t length) {
  size_t i = 0;
  for (; (i + 8) <= length; i += 8) {
    _mm_storeu_si128((__m128i*) (dst + i),
                     _mm256_cvtps_ph(_mm256_loadu_ps(src + i), 0));
  }
  for (; i < length; i++) {
    dst[i] = _cvtss_sh(src[i], 0);
  }
  return true;
}
bool ConvertHelper<fp32_t, fp16_t>::Assign(fp32_t* dst,
                                           const fp16_t* src,
                                           size_t length) {
  size_t i = 0;
  for (; (i + 8) <= length; i += 8) {
    _mm256_storeu_ps((dst + i),
                     _mm256_cvtph_ps(_mm_loadu_si128((__m128i*) (src + i))));
  }
  for (; i < length; i++) {
    dst[i] = _cvtsh_ss(src[i]);
  }
  return true;
}
bool ConvertHelper<fp32_t, fp16_t>::AddMA(fp32_t* dst,
                                          const fp16_t* src,
                                          fp32_t a,
                                          size_t length) {
  auto a128 = _mm_load_ps1(&a);
  auto a256 = _mm256_broadcast_ps(&a128);
  size_t i = 0;
  for (; (i + 8) <= length; i += 8) {
    auto s256 = _mm256_cvtph_ps(_mm_loadu_si128((__m128i*) (src + i)));
    auto mul = _mm256_mul_ps(a256, s256);
    auto d256 = _mm256_loadu_ps(dst + i);
    _mm256_storeu_ps(dst + i, _mm256_add_ps(d256, mul));
  }
  for (; i < length; i++) {
    dst[i] += _cvtsh_ss(src[i]) * a;
  }
  return true;
}
bool ConvertHelper<fp32_t, fp16_t>::Add(fp32_t* dst,
                                        const fp16_t* src,
                                        size_t length) {
  size_t i = 0;
  for (; (i + 8) <= length; i += 8) {
    auto s256 = _mm256_cvtph_ps(_mm_loadu_si128((__m128i*) (src + i)));
    auto d256 = _mm256_loadu_ps(dst + i);
    _mm256_storeu_ps(dst + i, _mm256_add_ps(d256, s256));
  }
  for (; i < length; i++) {
    dst[i] += _cvtsh_ss(src[i]);
  }
  return true;
}

}  // namespace qed

