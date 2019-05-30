#include "orc/util/string_piece.h"

#include "gtest/gtest.h"

namespace orc {

TEST(StringPiece, Common) {
  {
    StringPiece sp;
    ASSERT_TRUE(sp.empty());
  }
  {
    StringPiece sp("");
    ASSERT_TRUE(sp.empty());
  }
  {
    StringPiece sp("a");
    ASSERT_FALSE(sp.empty());
    ASSERT_EQ(sp.size(), 1);
  }
  {
    std::string a = "a";
    StringPiece sp(a);
    ASSERT_FALSE(sp.empty());
    ASSERT_EQ(sp.size(), 1);
  }
  {
    const char* a = "a";
    StringPiece sp(a, 1);
    ASSERT_FALSE(sp.empty());
    ASSERT_EQ(sp.size(), 1);
  }
}

TEST(StringPiece, Equal) {
#define CMP_Y(op, x, y) \
  { \
    ASSERT_TRUE(StringPiece(x) op StringPiece(y)); \
  }

#define CMP_N(op, x, y) \
  { \
    ASSERT_FALSE(StringPiece(x) op StringPiece(y)); \
  }

  CMP_Y(==, "", "");
  CMP_Y(==, "a", "a");
  CMP_Y(==, "aa", "aa");
  CMP_N(==, "a", "");
  CMP_N(==, "", "a");
  CMP_N(==, "a", "b");
  CMP_N(==, "a", "aa");
  CMP_N(==, "aa", "a");

  CMP_N(!=, "", "");
  CMP_N(!=, "a", "a");
  CMP_N(!=, "aa", "aa");
  CMP_Y(!=, "a", "");
  CMP_Y(!=, "", "a");
  CMP_Y(!=, "a", "b");
  CMP_Y(!=, "a", "aa");
  CMP_Y(!=, "aa", "a");

  CMP_Y(<, "a", "b");
  CMP_Y(<, "a", "aa");
  CMP_Y(<, "aa", "b");
  CMP_Y(<, "aa", "bb");
  CMP_N(<, "a", "a");
  CMP_N(<, "b", "a");
  CMP_N(<, "aa", "a");
  CMP_N(<, "b", "aa");
  CMP_N(<, "bb", "aa");

  CMP_Y(<=, "a", "a");
  CMP_Y(<=, "a", "b");
  CMP_Y(<=, "a", "aa");
  CMP_Y(<=, "aa", "b");
  CMP_Y(<=, "aa", "bb");
  CMP_N(<=, "b", "a");
  CMP_N(<=, "aa", "a");
  CMP_N(<=, "b", "aa");
  CMP_N(<=, "bb", "aa");
}

TEST(StringPiece, STL) {
  std::string str = "abcdef";
  StringPiece sp(str);

  ASSERT_FALSE(sp.empty());

  ASSERT_EQ(sp.size(), str.size());
  ASSERT_EQ(sp.data(), str.data());
  ASSERT_EQ(sp.length(), str.length());

  for (size_t i = 0; i < str.size(); ++i) {
    ASSERT_EQ(sp[i], str[i]);
  }

  ASSERT_EQ(*sp.begin(), *str.begin());
  ASSERT_EQ(*(sp.begin()+1), *(str.begin()+1));
  ASSERT_EQ(*(sp.begin()+3), *(str.begin()+3));

  ASSERT_EQ(*(sp.end()-1), *(str.end()-1));

  ASSERT_EQ(*(sp.rbegin()), *(str.end()-1));
  ASSERT_EQ(*(sp.rbegin()+1), *(str.end()-2));
  ASSERT_EQ(*(sp.rbegin()+3), *(str.end()-4));

  ASSERT_EQ(*(sp.rend()-1), *(str.begin()));
  ASSERT_EQ(*(sp.rbegin()-3), *(str.end()+2));

  ASSERT_EQ(sp.substr(3), str.substr(3));
  ASSERT_EQ(sp.substr(1, 3), str.substr(1,3));
  ASSERT_EQ(sp.substr(6, 3), str.substr(6,3));
  ASSERT_EQ(sp.substr(3, 9), str.substr(3,9));

  sp.clear();
  ASSERT_EQ(sp.data(), nullptr);
  ASSERT_EQ(sp.size(), 0);
  ASSERT_TRUE(sp.empty());
  ASSERT_TRUE(sp.begin() == sp.end());
}

TEST(StringPiece, Find) {
  {
    std::string str = "abcdefghijklmnopqrstuvwxyz";
    StringPiece sp(str);

    ASSERT_EQ(sp.find('a'), str.find('a'));
    ASSERT_EQ(sp.find('a', 10), str.find('a', 10));
    ASSERT_EQ(sp.find('1'), str.find('1'));
    ASSERT_EQ(sp.find('1', 10), str.find('1', 10));

    ASSERT_EQ(sp.find("abc"), str.find("abc"));
    ASSERT_EQ(sp.find("aa"), str.find("aa"));
    ASSERT_EQ(sp.find("st"), str.find("st"));
    ASSERT_EQ(sp.find("aa"), str.find("aa"));
  }
  {
    std::string str = "aaaccbbaaddccbbdd";
    StringPiece sp(str);

    ASSERT_EQ(sp.find_first_of('a'), str.find_first_of('a'));
    ASSERT_EQ(sp.find_first_of('b'), str.find_first_of('b'));
    ASSERT_EQ(sp.find_first_of('c'), str.find_first_of('c'));

    ASSERT_EQ(sp.find_first_of("def"), str.find_first_of("def"));
    ASSERT_EQ(sp.find_first_of("da"), str.find_first_of("da"));
    ASSERT_EQ(sp.find_first_of("cba"), str.find_first_of("cba"));
    ASSERT_EQ(sp.find_first_of("890"), str.find_first_of("890"));
    ASSERT_EQ(sp.find_first_of("cc"), str.find_first_of("cc"));

  }
}

}  // namespace orc
