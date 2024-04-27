#pragma once
#include <cstdint>
#include <cstring>
#include <string>
#include "Timestamp.h"
#include "Nodes/Node.h"
#include <unordered_map>

//Linked list structure could be augmented with a tree lookup structure for O(log(n)) lookup/insert
//  Incoming operations are fast because of the block map, but local inserts need to linear search for a string position

template <class T>
struct BlockData
{
  static const uint32_t maxLength = 0xFFFFFFFF;

  Timestamp id;
  uint32_t offset = 0;
  uint32_t length = 0;
  Effect effect;
  T * value = nullptr;

  BlockData<T> * nextSibling = nullptr;
  BlockData<T> * nextSplit = nullptr;

  uint32_t getDataLength() const;
};

template <class T>
class BlockValue
{
public:
  using ChangedCallback = std::function<void(size_t, char *, uint32_t)>;

  ~BlockValue();

  std::pair<bool, size_t> findBlockOffset(BlockData<T> * block) const;
  std::pair<bool, size_t> findBlockOffsetUtf16(BlockData<T> * blockPtr) const;

  BlockData<T> * findOffsetPtr(uint32_t & offset) const;
  std::pair<BlockData<T> *, uint32_t> findUtf16CodeUnitOffsetPtr(size_t offsetCodeUnits) const;

  BlockData<T> * getChildren();
  const BlockData<T> * getChildren() const;

  Timestamp findOffset(uint32_t & offset) const;
  std::pair<Timestamp, uint32_t> findUtf16CodeUnitOffset(size_t offsetCodeUnits) const;

  void insertAfter(const Timestamp & blockId, uint32_t offset, const Timestamp & ts, uint32_t length, const T * data, ChangedCallback callback);
  void updateEffect(const Timestamp & blockId, uint32_t offset, uint32_t length, int delta, ChangedCallback callback);
  void deinitializeBlock(const Timestamp & blockId, ChangedCallback callback);
  void deleteAfter(const Timestamp & blockId, const Timestamp & deleteBlockId, ChangedCallback callback);

  size_t getLength() const;

  void cleanUpFollowingSplits(BlockData<T> * block);
  void deleteAllBlocks();

  BlockData<T> * getBlockData(const Timestamp & timestamp);
  BlockData<T> * getBlock(const Timestamp & timestamp, uint32_t offset, uint32_t length);
  const BlockData<T> * getExistingBlock(const Timestamp & timestamp) const;
  BlockData<T> * splitAt(BlockData<T> * start, uint32_t pos);

  void computeInsertions(size_t offset, const uint8_t * data,
    size_t length, std::function<void(const Timestamp & blockId, uint32_t offset, const uint8_t * data, uint32_t length)> callback) const;
  void computeDeletions(size_t offset, size_t length,
    std::function<void(const Timestamp & blockId, uint32_t offset, uint32_t length)> callback) const;

  std::string toString() const;

  BlockData<T> * children = nullptr;
private:
  std::unordered_map<Timestamp, BlockData<T> *> blocks;

  void initializeBlock(const Timestamp & blockId, ChangedCallback callback);
  void printList() const;
};