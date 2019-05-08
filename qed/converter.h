/*!
 * \file converter.h
 * \brief Data converter
 */
#ifndef QED_CONVERTER_H_
#define QED_CONVERTER_H_

#include "qed/basic.h"
#include <type_traits>

namespace qed {

template <typename TA, typename TB>
class ConvertHelper {
 public:
  /**
   * @brief do type conversion of two item, simple copy by default if two type are the same
   * @return true if success
   */
  static bool Assign(TA& dst, const TB& src) {
    static_assert(std::is_same<TA, TB>::value, "unsupported conversion");
    dst = src;
    return true;
  }

  /**
   * @brief do type conversion of two array, simple copy by default if two type are the same
   * @return true if success
   */
  static bool Assign(TA* dst, const TB* src, size_t length) {
    static_assert(std::is_same<TA, TB>::value, "unsupported conversion");
    for (int i = 0; i < length; i++) {
      dst[i] = src[i];
    }
    return true;
  }

  /**
   * @brief dst = dst + src * a
   * @return
   */
  static bool AddMA(TA* dst, const TB* src, TA a, size_t length) {
    static_assert(std::is_same<TA, TB>::value, "unsupported conversion");
    for (int i = 0; i < length; i++) {
      dst[i] = dst[i] + src[i] * a;
    }
    return true;
  }

  /**
   * @brief dst = dst + src
   * @return
   */
  static bool Add(TA* dst, const TB* src, size_t length) {
    static_assert(std::is_same<TA, TB>::value, "unsupported conversion");
    for (int i = 0; i < length; i++) {
      dst[i] += src[i];
    }
    return true;
  }
};

template<>
class ConvertHelper<fp32_t, fp32_t> {
 public:
  static bool Assign(fp32_t& dst, const fp32_t& src);
  static bool Assign(fp32_t* dst, const fp32_t* src, size_t length);
  static bool AddMA(fp32_t* dst, const fp32_t* src, fp32_t a, size_t length);
  static bool Add(fp32_t* dst, const fp32_t* src, size_t length);
};

template<>
class ConvertHelper<fp16_t, fp32_t> {
 public:
  //  static bool Assign(fp16_t& dst, const fp32_t& src);
  static bool Assign(fp16_t* dst, const fp32_t* src, size_t length);
  //  static bool AddMA(fp16_t* dst, const fp32_t* src, fp32_t a, size_t length);
  //  static bool Add(fp16_t* dst, const fp32_t* src, size_t length);
};

template<>
class ConvertHelper<fp32_t, fp16_t> {
 public:
  //  static bool Assign(fp32_t& dst, const fp16_t& src);
  static bool Assign(fp32_t* dst, const fp16_t* src, size_t length);
  static bool AddMA(fp32_t* dst, const fp16_t* src, fp32_t a, size_t length);
  static bool Add(fp32_t* dst, const fp16_t* src, size_t length);
};

}  // namespace qed

#endif  // QED_CONVERTER_H_
