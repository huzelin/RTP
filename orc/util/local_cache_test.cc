#include <memory>
#include <string>

#include "gtest/gtest.h"
#include "orc/util/local_cache.h"

namespace orc {

TEST(LocalCache, GetPut) {
  std::unique_ptr<LocalCache<std::string, std::string>> cache{
    new LocalCache<std::string, std::string>(10, 10000)
  };

  uint64_t now = 1234567000;

  cache->Put("aaaa", "11111", now);

  std::string val;
  EXPECT_TRUE(cache->Get("aaaa", &val, now));
  EXPECT_EQ("11111", val);
  EXPECT_EQ(1, cache->size());

  EXPECT_FALSE(cache->Get("aaaa", &val, now+20000));
  EXPECT_EQ(0, cache->size());

  const char* key = "abcdefghijklmnopqrstuvwxyz";
  for (size_t i = 0; i < 10; ++i) {
    cache->Put(key+i, "11111", now);
  }
  EXPECT_EQ(10, cache->size());
  EXPECT_TRUE(cache->Get(key, &val, now));

  cache->Put(key+10, "11111", now);
  EXPECT_EQ(10, cache->size());
  EXPECT_TRUE(cache->Get(key, &val, now));
  EXPECT_FALSE(cache->Get(key+1, &val, now));

  for (size_t i = 0; i <= 10; ++i) {
    cache->Get(key+i, &val, now+20000);
  }

  EXPECT_EQ(0, cache->size());
}

}  // namespace orc
