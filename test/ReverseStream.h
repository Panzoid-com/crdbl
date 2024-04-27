#pragma once
#include <Streams/IReadableStream.h>
#include <Streams/IWritableStream.h>
#include <Streams/ReadableStreamBase.h>
#include <string>

/*
 * This is mainly for testing serialization; it buffers a forward stream in its
 * entirety and then when complete, it writes the entire stream back in reverse
 * chunks.
 */

constexpr int bufferSize = 4096;

class ReverseStream : public IWritableStream<std::string_view>, public ReadableStreamBase<std::string_view>
{
public:
  bool write(const std::string_view & data)
  {
    buffer.append(data);
    return true;
  }

  void close()
  {
    if (closed)
    {
      return;
    }

    int numChunks = (buffer.size() + (bufferSize - 1)) / bufferSize;
    for (int i = 0; i < numChunks; i++)
    {
      int chunkSize = bufferSize;
      int chunkOffset = buffer.size() - ((i + 1) * bufferSize);
      if (chunkOffset < 0)
      {
        chunkSize += chunkOffset;
        chunkOffset = 0;
      }

      writeToDestination(std::string_view(buffer.data() + chunkOffset, chunkSize));
    }

    closed = true;
  }

private:
  std::basic_string<char> buffer;
  bool closed = false;
};