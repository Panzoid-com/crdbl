#include "BlockValue.h"
#include <iostream>

size_t getUtf16CodeUnitLength(char * str, size_t length)
{
  size_t utf16Length = 0;
  size_t i = 0;

  while (i < length)
  {
    char c = str[i];
    if (c >= 0 && c <= 127) i += 1;
    else if ((c & 0xE0) == 0xC0) i += 2;
    else if ((c & 0xF0) == 0xE0) i += 3;
    else if ((c & 0xF8) == 0xF0)
    {
      i += 4;
      utf16Length++;
    }

    utf16Length++;
  }

  return utf16Length;
}

template <class T>
uint32_t BlockData<T>::getDataLength() const
{
  const BlockData<T> * currentSplit = this;
  uint32_t length = 0;

  while (currentSplit)
  {
    length += currentSplit->length;
    currentSplit = currentSplit->nextSplit;
  }

  return length;
}

template struct BlockData<char>;

template <class T>
BlockValue<T>::~BlockValue()
{
  deleteAllBlocks();
}

template <class T>
BlockData<T> * BlockValue<T>::splitAt(BlockData<T> * start, uint32_t pos)
{
  while (start->offset + start->length < pos)
  {
    start = start->nextSplit;

    //past the end of data (bad argument)
    if (start == nullptr)
    {
      return nullptr;
    }
  }

  if (start->offset != pos && start->offset + start->length != pos)
  {
    BlockData<T> * next = new BlockData<T>();
    uint32_t delta = pos - start->offset;

    //split the block at offset
    next->id = start->id;
    next->offset = pos;
    next->length = start->length - delta;
    next->effect = start->effect;
    next->nextSibling = start->nextSibling;
    next->nextSplit = start->nextSplit;
    if (start->value != nullptr)
    {
      next->value = start->value + delta;
    }

    start->length = pos - start->offset;
    start->nextSibling = next;
    start->nextSplit = next;
  }

  return start;
}

template <class T>
BlockData<T> * BlockValue<T>::getBlockData(const Timestamp & blockId)
{
  BlockData<T> * block = nullptr;

  auto it = blocks.find(blockId);
  if (it != blocks.end())
  {
    block = it->second;
  }

  if (block == nullptr)
  {
    block = new BlockData<T>();
    block->id = blockId;
    block->offset = 0;
    block->length = BlockData<T>::maxLength;
    blocks[blockId] = block;
  }

  return block;
}

template <class T>
const BlockData<T> * BlockValue<T>::getExistingBlock(const Timestamp & blockId) const
{
  const BlockData<T> * block = nullptr;

  auto it = blocks.find(blockId);
  if (it != blocks.end())
  {
    block = it->second;
  }

  return block;
}

template <class T>
BlockData<T> * BlockValue<T>::getBlock(const Timestamp & blockId, uint32_t offset, uint32_t length)
{
  BlockData<T> * block = getBlockData(blockId);

  block = splitAt(block, offset);
  if (block == nullptr)
  {
    return nullptr;
  }
  else if (offset != block->offset)
  {
    block = block->nextSplit;
  }

  splitAt(block, offset + length);

  return block;
}

template <class T>
void BlockValue<T>::cleanUpFollowingSplits(BlockData<T> * block)
{
  while (block->nextSplit != nullptr)
  {
    //remove from the nextSibling list (walking through as necessary)
    BlockData<T> * prev = block;
    while (prev != nullptr)
    {
      if (prev->nextSibling == block->nextSplit)
      {
        prev->nextSibling = prev->nextSibling->nextSibling;
        break;
      }

      prev = prev->nextSibling;
    }

    //remove from the nextSplit list and delete
    BlockData<T> * tmp = block->nextSplit;
    block->nextSplit = tmp->nextSplit;
    delete tmp;
  }
}

template <class T>
void BlockValue<T>::deleteAllBlocks()
{
  for (auto it = blocks.begin(); it != blocks.end(); ++it)
  {
    BlockData<T> * block = it->second;
    BlockData<T> * split = block->nextSplit;

    while (split != nullptr)
    {
      block->nextSplit = split->nextSplit;
      delete split;
      split = block->nextSplit;
    }

    delete block;
  }

  children = nullptr;
}

template <class T>
std::pair<bool, size_t> BlockValue<T>::findBlockOffset(BlockData<T> * blockPtr) const
{
  BlockData<T> * block = children;
  size_t offset = 0;

  while (block != nullptr)
  {
    if (block == blockPtr)
    {
      return std::make_pair(true, offset);
    }

    if (block->effect.isVisible())
    {
      offset += block->length;
    }

    block = block->nextSibling;
  }

  return std::make_pair(false, offset);
}

template <class T>
std::pair<bool, size_t> BlockValue<T>::findBlockOffsetUtf16(BlockData<T> * blockPtr) const
{
  BlockData<T> * block = children;
  size_t offset = 0;

  while (block != nullptr)
  {
    if (block == blockPtr)
    {
      return std::make_pair(true, offset);
    }

    if (block->effect.isVisible())
    {
      offset += getUtf16CodeUnitLength(block->value, block->length);
    }

    block = block->nextSibling;
  }

  return std::make_pair(false, offset);
}

template <class T>
Timestamp BlockValue<T>::findOffset(uint32_t & offset) const
{
  Timestamp ts = { 0, 0 };
  BlockData<T> * block = children;
  uint32_t pos = 0;

  if (offset == 0)
  {
    return ts;
  }

  while (true)
  {
    if (block->effect.isVisible())
    {
      if (pos + block->length >= offset)
      {
        ts = block->id;
        offset = block->offset + offset - pos;
        return ts;
      }

      pos += block->length;
    }

    //NOTE: this isn't ideal if the last block is invisible
    //  it will probably technically still work but really the last visible block
    //  should be returned
    //  also some callers may expect that only visible blocks will be returned
    if (block->nextSibling == nullptr)
    {
      ts = block->id;
      offset = block->offset + block->length;
      return ts;
    }

    block = block->nextSibling;
  }

  return ts;
}

template <class T>
std::pair<Timestamp, uint32_t> BlockValue<T>::findUtf16CodeUnitOffset(size_t offsetCodeUnits) const
{
  auto result = findUtf16CodeUnitOffsetPtr(offsetCodeUnits);
  if (result.first != nullptr && offsetCodeUnits > 0)
  {
    return std::make_pair(result.first->id, result.first->offset + result.second);
  }
  else
  {
    return std::make_pair(Timestamp(), 0);
  }
}

template <class T>
BlockData<T> * BlockValue<T>::findOffsetPtr(uint32_t & offset) const
{
  BlockData<T> * block = children;
  uint32_t pos = 0;

  if (offset == 0)
  {
    return block;
  }

  while (true)
  {
    if (block->effect.isVisible())
    {
      if (pos + block->length >= offset)
      {
        offset = offset - pos;
        return block;
      }

      pos += block->length;
    }

    if (block->nextSibling == nullptr)
    {
      offset = block->length;
      return block;
    }

    block = block->nextSibling;
  }
}

template <class T>
std::pair<BlockData<T> *, uint32_t> BlockValue<T>::findUtf16CodeUnitOffsetPtr(size_t offsetCodeUnits) const
{
  BlockData<T> * block = children;
  BlockData<T> * lastVisible = nullptr;
  uint32_t utf16Offset = 0;

  if (offsetCodeUnits == 0 || block == nullptr)
  {
    //return the first visible block (not before the first block) if possible
      return std::make_pair(block, 0);
  }

  while (block != nullptr)
  {
    if (block->effect.isVisible())
    {
      lastVisible = block;
      uint32_t utf8BlockOffset = 0;
      uint32_t end = block->length;
      while (utf8BlockOffset < end)
      {
        char c = block->value[utf8BlockOffset];
        if (c >= 0 && c <= 127) utf8BlockOffset += 1;
        else if ((c & 0xE0) == 0xC0) utf8BlockOffset += 2;
        else if ((c & 0xF0) == 0xE0) utf8BlockOffset += 3;
        else if ((c & 0xF8) == 0xF0)
        {
          // Surrogate pairs, the only case where we have 2 utf16 code units
          utf8BlockOffset += 4;
          utf16Offset++;
        }

        utf16Offset++;

        if (utf16Offset >= offsetCodeUnits)
        {
          return std::make_pair(block, utf8BlockOffset);
        }
      }
    }
    block = block->nextSibling;
  }

  if (lastVisible != nullptr)
  {
    return std::make_pair(lastVisible, lastVisible->length);
  }

  return std::make_pair(nullptr, 0);
}

template <class T>
BlockData<T> * BlockValue<T>::getChildren()
{
  return children;
}

template <class T>
const BlockData<T> * BlockValue<T>::getChildren() const
{
  return const_cast<const BlockData<T> *>(children);
}

template <class T>
void BlockValue<T>::insertAfter(const Timestamp & blockId, uint32_t offset,
  const Timestamp & ts, uint32_t length, const T * data, ChangedCallback callback)
{
  BlockData<T> * block = getBlock(ts, 0, length);

  //if the block is already initialized it must already be inserted
  if (block->effect.isInitialized())
  {
    return;
  }

  //initialize all split blocks
  BlockData<T> * split = block;
  while (split != nullptr)
  {
    if (split->offset < length)
    {
      split->value = const_cast<T *>(data) + split->offset;
      // split->effect.initialize();

      if (split->offset + split->length == length)
      {
        //this is to account for any potential extra split blocks that have
        //been created due to previous invalid ops
        cleanUpFollowingSplits(split);
        break;
      }

      split = split->nextSplit;
    }
    else
    {
      //this should never happen
    }
  }

  BlockData<T> ** insert = nullptr;
  if (blockId.isNull())
  {
    insert = &children;
  }
  else
  {
    BlockData<T> * prev = getBlockData(blockId);
    prev = splitAt(prev, offset);
    //TODO: prev could be nullptr here due to bad argument
    insert = &prev->nextSibling;
  }

  //insert after newer blocks
  while (*insert != nullptr)
  {
    if ((*insert)->id < block->id || (*insert)->id == block->id)
    {
      break;
    }

    insert = &(*insert)->nextSibling;
  }

  if ((*insert) != nullptr && (*insert)->id == block->id)
  {
    //block is already in the list
    initializeBlock(ts, callback);
    return;
  }

  //find the last sibling in this sibling list
  BlockData<T> ** nextSibling = &block->nextSibling;
  while (*nextSibling != nullptr)
  {
    nextSibling = &(*nextSibling)->nextSibling;
  }

  //insert block(s) in the main sibling list
  *nextSibling = *insert;
  *insert = block;

  initializeBlock(ts, callback);

  return;
}

template <class T>
void BlockValue<T>::initializeBlock(const Timestamp & blockId, ChangedCallback callback)
{
  BlockData<T> * block = const_cast<BlockData<T> *>(getExistingBlock(blockId));
  auto blockOffset = findBlockOffsetUtf16(block);

  while (block != nullptr)
  {
    bool prevVisibility = false;

    block->effect.initialize();

    bool newVisibility = block->effect.isVisible();

    if (blockOffset.first && prevVisibility != newVisibility)
    {
      if (newVisibility)
      {
        // we return the actual data size here, not the utf16 code unit length
        callback(blockOffset.second, block->value, block->length);
      }
      else
      {
        callback(blockOffset.second, nullptr, getUtf16CodeUnitLength(block->value, block->length));
      }
    }

    BlockData<T> * nextBlock = block->nextSplit;
    while (block != nextBlock)
    {
      if (block->effect.isVisible())
      {
        blockOffset.second += getUtf16CodeUnitLength(block->value, block->length);
      }
      block = block->nextSibling;
    }
  }
}

template <class T>
void BlockValue<T>::updateEffect(const Timestamp & blockId, uint32_t offset,
  uint32_t length, int delta, ChangedCallback callback)
{
  BlockData<T> * block = getBlock(blockId, offset, length);
  auto blockOffset = findBlockOffsetUtf16(block);

  if (length > BlockData<T>::maxLength - offset)
  {
    length = BlockData<T>::maxLength - offset;
  }

  while (block != nullptr && block->offset < offset + length)
  {
    bool prevVisibility = block->effect.isVisible();

    if (delta == 0)
    {
      //special case: 0 delta indicates that the effect should be deinitialized
      block->effect.deinitialize();
    }
    else
    {
      block->effect += delta;
    }

    bool newVisibility = block->effect.isVisible();

    if (blockOffset.first && prevVisibility != newVisibility)
    {
      if (newVisibility)
      {
        // we return the actual data size here, not the utf16 code unit length
        callback(blockOffset.second, block->value, block->length);
      }
      else
      {
        callback(blockOffset.second, nullptr, getUtf16CodeUnitLength(block->value, block->length));
      }
    }

    BlockData<T> * nextBlock = block->nextSplit;
    while (block != nextBlock)
    {
      if (block->effect.isVisible())
      {
        blockOffset.second += getUtf16CodeUnitLength(block->value, block->length);
      }
      block = block->nextSibling;
    }
  }
}

template <class T>
void BlockValue<T>::deinitializeBlock(const Timestamp & blockId, ChangedCallback callback)
{
  updateEffect(blockId, 0, BlockData<T>::maxLength, 0, callback);
}

template <class T>
void BlockValue<T>::deleteAfter(const Timestamp & blockId,
  const Timestamp & deleteBlockId, ChangedCallback callback)
{
  BlockData<T> ** cursor;
  // if (blockId.isNULL())
  // {
    cursor = &children;
  // }
  // else
  // {
  //   const BlockData<T> * prevBlock = getExistingBlock(blockId);
  //   if (prevBlock == nullptr)
  //   {
  //     return;
  //   }
  //   cursor = &prevBlock->nextSibling;
  // }

  const BlockData<T> * searchBlock = getExistingBlock(deleteBlockId);
  if (searchBlock == nullptr)
  {
    return;
  }

  T * blockValue = searchBlock->value;

  size_t offset = 0;
  while (*cursor != nullptr)
  {
    if (*cursor == searchBlock)
    {
      *cursor = searchBlock->nextSibling;
      BlockData<T> * deleteBlock = const_cast<BlockData<T> *>(searchBlock);
      searchBlock = searchBlock->nextSplit;

      if (deleteBlock->effect.isVisible())
      {
        callback(offset, nullptr, deleteBlock->length);
      }
      delete deleteBlock;

      if (searchBlock == nullptr)
      {
        break;
      }

      continue;
    }

    if ((*cursor)->effect.isVisible())
    {
      offset += (*cursor)->length;
    }

    cursor = &(*cursor)->nextSibling;
  }

  blocks.erase(deleteBlockId);

  if (blockValue != nullptr)
  {
    delete blockValue;
  }
}

template <class T>
size_t BlockValue<T>::getLength() const
{
  size_t length = 0;
  BlockData<T> * block = children;

  while (block != nullptr)
  {
    if (block->effect.isVisible())
    {
      length += block->length;
    }

    block = block->nextSibling;
  }

  return length;
}

template <class T>
std::string BlockValue<T>::toString() const
{
  std::string outString;
  BlockData<T> * block = children;

  while (block != nullptr)
  {
    if (block->effect.isVisible())
    {
      outString.append(reinterpret_cast<const char *>(block->value), block->length);
    }

    block = block->nextSibling;
  }

  return outString;
}

template <class T>
void BlockValue<T>::computeInsertions(size_t offset, const uint8_t * data,
  size_t length,
  std::function<void(const Timestamp &, uint32_t, const uint8_t *, uint32_t)> callback) const
{
  auto insert = findUtf16CodeUnitOffsetPtr(offset);
  if (insert.first != nullptr && offset > 0)
  {
    callback(insert.first->id, insert.first->offset + insert.second, data, length);
  }
  else
  {
    //insert at front (i.e. the end of the empty list)
    callback(Timestamp(), 0, data, length);
  }
}

template <class T>
void BlockValue<T>::computeDeletions(size_t offset, size_t length,
  std::function<void(const Timestamp &, uint32_t, uint32_t)> callback) const
{
  size_t endOffset = offset + length;
  auto start = findUtf16CodeUnitOffsetPtr(offset);
  auto end = findUtf16CodeUnitOffsetPtr(endOffset);

  BlockData<T> * data = start.first;
  uint32_t dataOffset = start.second;

  while (data != nullptr)
  {
    if (data->effect.isVisible() == false)
    {
      //don't need to set offset because findOffsetPtr will never return an invisible block
      data = data->nextSibling;
      continue;
    }

    if (data == end.first)
    {
      if (end.second > dataOffset)
      {
        callback(data->id, data->offset + dataOffset, end.second - dataOffset);
      }

      break;
    }

    if (data->length > dataOffset)
    {
      callback(data->id, data->offset + dataOffset, data->length - dataOffset);
    }

    data = data->nextSibling;
    dataOffset = 0;
  }
}

template <class T>
void BlockValue<T>::printList() const
{
  BlockData<T> * block = children;

  while (block != nullptr)
  {
    std::cout << "b " << block << " " << block->effect.isVisible() << " " <<
      std::string(block->value, block->length) << " " <<
      block->nextSibling << " " << block->nextSplit << " | ";

    if (block->nextSibling == block)
    {
      std::cout << "infinite loop detected " << std::endl;
      break;
    }

    block = block->nextSibling;
  }

  std::cout << std::endl;
}

template class BlockValue<char>;