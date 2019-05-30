#include "gtest/gtest.h"
#include "orc/util/str_util.h"

namespace orc {

TEST(StrUtil, Split) {
  {
    std::string a = ";";
    std::vector<std::string> v;
    str::Split(a, ";", &v);
    ASSERT_EQ(v.size(), 2);
    ASSERT_EQ(v[0], std::string(""));
    ASSERT_EQ(v[1], std::string(""));

    v.clear();
    str::Split(a, ";", &v, true);
    ASSERT_EQ(v.size(), 0);

    std::vector<StringPiece> vsp;
    str::Split(a, ";", &vsp);
    ASSERT_EQ(vsp.size(), 2);
    ASSERT_EQ(vsp[0], std::string(""));
    ASSERT_EQ(vsp[1], std::string(""));

    vsp.clear();
    str::Split(a, ";", &vsp, true);
    ASSERT_EQ(vsp.size(), 0);
  }

  {
    std::string a = "a;b;c;d";
    std::vector<std::string> v;

    str::Split(a, ";", &v);
    ASSERT_EQ(v.size(), 4);

    ASSERT_EQ(v[0], "a");
    ASSERT_EQ(v[1], "b");
    ASSERT_EQ(v[2], "c");
    ASSERT_EQ(v[3], "d");

    std::vector<StringPiece> vsp;
    str::Split(a, ";", &vsp);
    ASSERT_EQ(vsp.size(), 4);

    ASSERT_EQ(vsp[0], "a");
    ASSERT_EQ(vsp[1], "b");
    ASSERT_EQ(vsp[2], "c");
    ASSERT_EQ(vsp[3], "d");
  }

  {
    StringPiece a("a;b;c;d;xx;yy;zz;", 7);
    std::vector<StringPiece> vsp;
    str::Split(a, ";", &vsp);
    ASSERT_EQ(vsp.size(), 4);

    ASSERT_EQ(vsp[0], "a");
    ASSERT_EQ(vsp[1], "b");
    ASSERT_EQ(vsp[2], "c");
    ASSERT_EQ(vsp[3], "d");
  }
}

TEST(StrUtil, Convert) {
#define EQ(src, method, num) \
  { \
    decltype(num) tmp; \
    ASSERT_TRUE(str::method(src, &tmp)); \
    ASSERT_EQ(tmp, num); \
  }

  EQ("123", ToInt32, 123);
  EQ("0", ToInt32, 0);
  EQ("4294967295", ToInt32, -1);
  EQ("2147483647", ToInt32, 2147483647);
  EQ("2147483648", ToInt32, (int32_t)-2147483648);

  EQ("123", ToUint32, (uint32_t)123);
  EQ("0", ToUint32, (uint32_t)0);
  EQ("2147483648", ToUint32, (uint32_t)2147483648);
  EQ("4294967295", ToUint32, (uint32_t)4294967295);
#undef EQ

#define DEQ(src, method, num) \
  { \
    decltype(num) tmp; \
    ASSERT_TRUE(str::method(src, &tmp)); \
    ASSERT_NEAR(tmp, num, 0.00000001); \
  }

  DEQ("1.23", ToFloat, 1.23f);
  DEQ("0.12", ToFloat, 0.12f);
  DEQ("1.234567891", ToFloat, 1.234567891f);

  DEQ("1.234567891", ToDouble, 1.234567891);
  DEQ("0.234567891", ToDouble, 0.234567891);
#undef DEQ

  int32_t n;
  ASSERT_TRUE(str::ToInt32("123\002123", 3, &n));
  ASSERT_EQ(123, n);
}

TEST(StrUtil, StrConvert) {
#define EQ(Type, src, dst) \
  { \
    Type s(src); \
    decltype(dst) tmp; \
    ASSERT_TRUE(str::StringConvert(s, &tmp)); \
    ASSERT_EQ(tmp, dst); \
  }

  EQ(StringPiece, "123", 123);
  EQ(std::string, "123", 123);

  EQ(StringPiece, "2147483647", 2147483647);
  EQ(std::string, "2147483647", 2147483647);

  EQ(StringPiece, "2147483648", (int32_t)-2147483648);
  EQ(std::string, "2147483648", (int32_t)-2147483648);

#undef EQ

  {
    std::string str = "aaa";
    std::string dst;
    ASSERT_TRUE(str::StringConvert(str, &dst));
    ASSERT_EQ("aaa", dst);
  }
}

TEST(StrUtil, RightTrim) {
  {
    std::string a = "a   ";
    str::RightTrim(&a);
    ASSERT_EQ("a", a);
  }

  {
    std::string a = "ab  b  ";
    str::RightTrim(&a);
    ASSERT_EQ("ab  b", a);
  }

  {
    std::string a = "ab";
    str::RightTrim(&a);
    ASSERT_EQ("ab", a);
  }


}

}  // namespace orc
