#pragma once
#include "IReadableStream.h"
#include "ReadableStreamBase.h"
#include "IWritableStream.h"
#include "../LogOperation.h"
#include "../RefCounted.h"
#include <vector>
#include <utility>

#define QoSAlgorithm_RoundRobin 0
#define QoSAlgorithm_AverageThroughput 1
#define QoSAlgorithm_ConsistentInterval 2
#define QoSAlgorithm QoSAlgorithm_AverageThroughput

template <auto GetTimeFn, typename TimeVal>
class QoSFilterOperationStream final :
  public IWritableStream<RefCounted<const LogOperation>>,
  public ReadableStreamBase<RefCounted<const LogOperation>>
{
public:
  QoSFilterOperationStream() {}

  void setFilterRate(TimeVal minWaitPeriod)
  {
    period = minWaitPeriod;
  }

  bool write(const RefCounted<const LogOperation> & data) override
  {
    if (data->op.type == OperationType::ValuePreviewOperation)
    {
      auto op = reinterpret_cast<const ValueSetOperation *>(&data->op);

      if (op->length == 0)
      {
        lastPreviewOpSentTimestamps.clear();
      }
      else
      {
        std::pair<NodeId, TimeVal> * currentItem = nullptr;
        for (int i = 0; i < lastPreviewOpSentTimestamps.size(); i++)
        {
          if (lastPreviewOpSentTimestamps[i].first == op->nodeId)
          {
            currentItem = &lastPreviewOpSentTimestamps[i];
            break;
          }
        }
        if (currentItem == nullptr)
        {
          lastPreviewOpSentTimestamps.push_back(std::make_pair(op->nodeId, 0));
          currentItem = &lastPreviewOpSentTimestamps.back();
        }

        auto time = GetTimeFn();

        if (QoSAlgorithm == QoSAlgorithm_RoundRobin)
        {
          if (time < lastPreviewOpSent + period)
          {
            return true;
          }
          else
          {
            std::pair<NodeId, TimeVal> * oldestItem = nullptr;
            for (int i = 0; i < lastPreviewOpSentTimestamps.size(); i++)
            {
              if (oldestItem == nullptr ||
                lastPreviewOpSentTimestamps[i].second < oldestItem->second)
              {
                oldestItem = &lastPreviewOpSentTimestamps[i];
              }
            }
            if (oldestItem != currentItem)
            {
              return true;
            }

            lastPreviewOpSent = time;
            currentItem->second = time;
          }
        }
        else if (QoSAlgorithm == QoSAlgorithm_AverageThroughput)
        {
          if (time < currentItem->second + period * lastPreviewOpSentTimestamps.size())
          {
            return true;
          }
          else
          {
            lastPreviewOpSent = time;
            currentItem->second = time;
          }
        }
        else if (QoSAlgorithm == QoSAlgorithm_ConsistentInterval)
        {
          if (time < currentItem->second + period)
          {
            return true;
          }
          else
          {
            lastPreviewOpSent = time;
            currentItem->second = time;
          }
        }
      }
    }

    return writeToDestination(data);
  }

  void close() override {}

private:
  TimeVal period = 0;
  TimeVal lastPreviewOpSent = 0;
  std::vector<std::pair<NodeId, TimeVal>> lastPreviewOpSentTimestamps;
};