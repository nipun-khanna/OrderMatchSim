#include <gtest/gtest.h>

#include <set>
#include <string>

#include "common/utils/Ids.h"
#include "common/utils/Time.h"

TEST(NowMicros, IsPositiveAndNonDecreasing) {
  const int64_t a = oms::util::nowMicros();
  const int64_t b = oms::util::nowMicros();
  EXPECT_GT(a, 0);
  EXPECT_GE(b, a);
}

TEST(NewId, IsPrefixedAndUnique) {
  std::set<std::string> ids;
  for (int i = 0; i < 1000; ++i) {
    std::string id = oms::util::newId("ord");
    EXPECT_EQ(id.rfind("ord-", 0), 0u) << "id should start with the prefix";
    EXPECT_TRUE(ids.insert(id).second) << "ids must be unique: " << id;
  }
}
