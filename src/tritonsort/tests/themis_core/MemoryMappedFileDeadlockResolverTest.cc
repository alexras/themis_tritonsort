#include <string.h>

#include "MemoryMappedFileDeadlockResolverTest.h"
#include "core/MemoryMappedFileDeadlockResolver.h"
#include "core/TritonSortAssert.h"

extern const char* TEST_WRITE_ROOT;

TEST_F(MemoryMappedFileDeadlockResolverTest, testResolve) {
  // Create a resolver with 1 disk in the write root.
  StringList diskList;
  diskList.push_back(TEST_WRITE_ROOT);
  MemoryMappedFileDeadlockResolver* resolver = NULL;
  ASSERT_NO_THROW(resolver = new MemoryMappedFileDeadlockResolver(diskList));

  // Try to mmap a file that is 100 bytes long.
  uint64_t requestSize = 100;
  void* memory = NULL;
  ASSERT_NO_THROW(memory = resolver->resolveRequest(requestSize));

  // Write 0..requestSize-1 to all the bytes.
  for (uint8_t i = 0; i < requestSize; ++i) {
    memset(static_cast<uint8_t*>(memory) + i, i, sizeof(i));
  }

  // Check that 0..requestSize-1 was written to all bytes.
  uint8_t* currentByte = static_cast<uint8_t*>(memory);
  uint8_t* memoryEnd = currentByte + requestSize;
  uint8_t i = 0;
  while (currentByte < memoryEnd) {
    EXPECT_EQ(i, *currentByte);
    ++currentByte;
    ++i;
  }

  // Clean up.
  ASSERT_NO_THROW(resolver->deallocate(memory));
  ASSERT_NO_THROW(delete resolver);
}

TEST_F(MemoryMappedFileDeadlockResolverTest, testBadDeallocate) {
  // Create a resolver with 1 disk in the write root.
  StringList diskList;
  diskList.push_back(TEST_WRITE_ROOT);
  MemoryMappedFileDeadlockResolver* resolver = NULL;
  ASSERT_NO_THROW(resolver = new MemoryMappedFileDeadlockResolver(diskList));

  // Try to deallocate garbage.
  void* memory = NULL;
  ASSERT_THROW(resolver->deallocate(memory), AssertionFailedException);

  // Clean up.
  ASSERT_NO_THROW(delete resolver);
}
