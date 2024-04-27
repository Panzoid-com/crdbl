#include <gtest/gtest.h>
#include <algorithm>
#include <random>
#include <unordered_map>
#include <memory>
#include <Core.h>
#include <Serialization/LogOperationSerialization.h>
#include <Streams/CallbackWritableStream.h>
#include <OperationType.h>
#include "helpers.h"
#include "ReverseStream.h"

struct RandomOpGenState
{
  bool isInGroupContext = false;
  std::unordered_map<PrimitiveNodeTypes::PrimitiveType, std::vector<NodeId>> nodes;
};

int randomInt(int min, int max)
{
  return min + (std::rand() % (max + 1 - min));
}

std::string randomString(size_t minLength, size_t maxLength)
{
  size_t length = randomInt(minLength, maxLength);
  std::string output = std::string(length, '\0');
  for (int i = 0; i < length; i++)
  {
    output[i] = static_cast<char>(std::rand() % 0xFF);
  }
  return output;
}

void applyRandomOperations(CoreTestWrapper & wrapper, int numOperations = 1000, std::shared_ptr<RandomOpGenState> state = nullptr)
{
  if (!state)
  {
    state = std::make_shared<RandomOpGenState>();
  }

  for (int i = 0; i < numOperations; i++)
  {
    OperationType opType = static_cast<OperationType>(
      std::rand() % static_cast<int>(OperationType::RedoBlockValueDeleteAfterOperation));

    switch (opType)
    {
      case OperationType::GroupOperation:
      {
        if (!state->isInGroupContext)
        {
          int numToApply = 10;
          numToApply = (numToApply > numOperations - i) ? numOperations - i : numToApply;
          state->isInGroupContext = true;
          wrapper.builder.startGroup(OperationType::GroupOperation);
          applyRandomOperations(wrapper, numToApply, state);
          wrapper.builder.commitGroup();
          state->isInGroupContext = false;
          i += numToApply;
          continue;
        }
        break;
      }
      case OperationType::NodeCreateOperation:
      {
        auto nodeType = static_cast<PrimitiveNodeTypes::PrimitiveType>(
          std::rand() % static_cast<int>(PrimitiveNodeTypes::PrimitiveType::StringValue));

        NodeId nodeId = NodeId::Null;

        switch (nodeType)
        {
          case PrimitiveNodeTypes::PrimitiveType::Abstract:
            nodeId = wrapper.builder.createNode(PrimitiveNodeTypes::Abstract());
            break;
          case PrimitiveNodeTypes::PrimitiveType::List:
            nodeId = wrapper.builder.createNode(PrimitiveNodeTypes::List());
            break;
          case PrimitiveNodeTypes::PrimitiveType::Map:
            nodeId = wrapper.builder.createNode(PrimitiveNodeTypes::Map());
            break;
          case PrimitiveNodeTypes::PrimitiveType::Reference:
            nodeId = wrapper.builder.createNode(PrimitiveNodeTypes::Reference());
            break;
          case PrimitiveNodeTypes::PrimitiveType::Int32Value:
            nodeId = wrapper.builder.createNode(PrimitiveNodeTypes::Int32Value());
            break;
          case PrimitiveNodeTypes::PrimitiveType::Int64Value:
            nodeId = wrapper.builder.createNode(PrimitiveNodeTypes::Int64Value());
            break;
          case PrimitiveNodeTypes::PrimitiveType::FloatValue:
            nodeId = wrapper.builder.createNode(PrimitiveNodeTypes::FloatValue());
            break;
          case PrimitiveNodeTypes::PrimitiveType::DoubleValue:
            nodeId = wrapper.builder.createNode(PrimitiveNodeTypes::DoubleValue());
            break;
          case PrimitiveNodeTypes::PrimitiveType::Int8Value:
            nodeId = wrapper.builder.createNode(PrimitiveNodeTypes::Int8Value());
            break;
          case PrimitiveNodeTypes::PrimitiveType::BoolValue:
            nodeId = wrapper.builder.createNode(PrimitiveNodeTypes::BoolValue());
            break;
          case PrimitiveNodeTypes::PrimitiveType::StringValue:
            nodeId = wrapper.builder.createNode(PrimitiveNodeTypes::StringValue());
            break;
          default:
            break;
        }

        if (nodeId != NodeId::Null)
        {
          state->nodes[nodeType].push_back(nodeId);
          continue;
        }

        break;
      }
      case OperationType::EdgeCreateOperation:
      {
        wrapper.builder.addChild(NodeId::SiteRoot, NodeId::SiteRoot, randomString(0, 50));
        continue;
      }
      case OperationType::EdgeDeleteOperation:
      {
        wrapper.builder.removeChild(NodeId::SiteRoot, EdgeId::Null);
        continue;
      }
      case OperationType::ValueSetOperation:
      {
        wrapper.builder.setValue(NodeId::SiteRoot, (int32_t)0);
        continue;
      }
      case OperationType::BlockValueInsertAfterOperation:
      {
        wrapper.builder.insertText(NodeId::SiteRoot, 0, randomString(0, 50));
        continue;
      }
      case OperationType::BlockValueDeleteAfterOperation:
      {
        wrapper.builder.deleteText(NodeId::SiteRoot, 0, randomInt(0, 100));
        continue;
      }
      default:
      {
        break;
      }
    }

    NoOpOperation op;
    op.type = OperationType::NoOpOperation;
    wrapper.builder.applyOperation(&op, op.getSize());
  }
}

TEST(SerializationTest, BasicSerializationWorks)
{
  CoreTestWrapper wrapper;

  std::srand(0);
  applyRandomOperations(wrapper);

  auto expectedIt = wrapper.log.begin();

  auto serializer = std::unique_ptr<ILogOperationSerializer>(
    LogOperationSerialization::CreateSerializer(
      LogOperationSerialization::DefaultFormat()));
  auto deserializer = std::unique_ptr<ILogOperationDeserializer>(
    LogOperationSerialization::CreateDeserializer(
      LogOperationSerialization::DefaultFormat(), DeserializeDirection::Forward));

  serializer->pipeTo(*deserializer);

  auto callbackStream = std::unique_ptr<CallbackWritableStream<RefCounted<const LogOperation>>>(
    new CallbackWritableStream<RefCounted<const LogOperation>>(
      [&](const RefCounted<const LogOperation> & op)
      {
        auto expectedOp = reinterpret_cast<const LogOperation *>((*expectedIt).data());
        ASSERT_EQ((*expectedIt).size(), op->getSize());
        ASSERT_EQ(*expectedOp, *op);
        ++expectedIt;
      },
      [&]()
      {
        ASSERT_EQ(expectedIt, wrapper.log.end());
      }
  ));

  deserializer->pipeTo(*callbackStream);

  for (auto & op : wrapper.log)
  {
    RefCounted<const LogOperation> rc(reinterpret_cast<const LogOperation *>(op.data()));
    serializer->write(rc);
    rc.release();
  }

  serializer->close();
  deserializer->close();
  callbackStream->close();
}

struct FormatTest
{
  const char * format;
  bool hasTs;
  bool hasTag;
  bool reverse;
};

const FormatTest formats[] =
{
  {
    "standard_v1_full",
    true,
    true,
    false
  },
  {
    "standard_v1_full", //reverse
    true,
    true,
    true
  },
  {
    "standard_v1_forward",
    true,
    true,
    false
  },
  {
    "standard_v1_untagged",
    true,
    false,
    false
  },
  {
    "standard_v1_untagged", //reverse
    true,
    false,
    true
  },
  {
    "standard_v1_type",
    false,
    false,
    false
  }
};

TEST(SerializationTest, SerializationBetweenSameFormatsWorks)
{
  CoreTestWrapper wrapper;

  std::srand(0);
  applyRandomOperations(wrapper);

  for (int i = 0; i < sizeof(formats) / sizeof(formats[0]); i++)
  {
    auto fwdIt = wrapper.log.begin();
    auto revIt = wrapper.log.rbegin();

    auto serializer = std::unique_ptr<ILogOperationSerializer>(
      LogOperationSerialization::CreateSerializer(formats[i].format));
    auto deserializer = std::unique_ptr<ILogOperationDeserializer>(
      LogOperationSerialization::CreateDeserializer(formats[i].format,
        (formats[i].reverse) ? DeserializeDirection::Reverse : DeserializeDirection::Forward));

    auto reverse = std::make_unique<ReverseStream>();

    if (formats[i].reverse)
    {
      serializer->pipeTo(*reverse);
      reverse->pipeTo(*deserializer);
    }
    else
    {
      serializer->pipeTo(*deserializer);
    }

    auto callbackStream = std::unique_ptr<CallbackWritableStream<RefCounted<const LogOperation>>>(
      new CallbackWritableStream<RefCounted<const LogOperation>>(
        [&](const RefCounted<const LogOperation> & op)
        {
          auto expected = (formats[i].reverse)
            ? std::basic_string<char>(*revIt) : std::basic_string<char>(*fwdIt);
          ASSERT_EQ(expected.size(), op->getSize());
          auto expectedOp = reinterpret_cast<LogOperation *>(expected.data());
          if (!formats[i].hasTag)
          {
            expectedOp->tag = Tag::Default();
          }
          if (!formats[i].hasTs)
          {
            expectedOp->ts = Timestamp::Null;
          }
          ASSERT_EQ(*expectedOp, *op);

          if (formats[i].reverse)
          {
            ++revIt;
          }
          else
          {
            ++fwdIt;
          }
        },
        [&]()
        {
          if (formats[i].reverse)
          {
            ASSERT_EQ(revIt, wrapper.log.rend());
          }
          else
          {
            ASSERT_EQ(revIt, wrapper.log.rbegin());
          }
        }
    ));

    deserializer->pipeTo(*callbackStream);

    for (auto & op : wrapper.log)
    {
      RefCounted<const LogOperation> rc(reinterpret_cast<const LogOperation *>(op.data()));
      serializer->write(rc);
      rc.release();
    }

    serializer->close();
    reverse->close();
    deserializer->close();
    callbackStream->close();
  }
}

TEST(SerializationTest, DeserializationOfRandomDataIsWellBehaved)
{
  std::srand(0);

  for (int i = 0; i < sizeof(formats) / sizeof(formats[0]); i++)
  {
    auto deserializer = std::unique_ptr<ILogOperationDeserializer>(
      LogOperationSerialization::CreateDeserializer(formats[i].format,
        (formats[i].reverse) ? DeserializeDirection::Reverse : DeserializeDirection::Forward));

    const size_t numRandomBytes = 10240;
    size_t bytesWritten = 0;
    size_t unparsedBytes = 0;

    auto callbackStream = std::unique_ptr<CallbackWritableStream<RefCounted<const LogOperation>>>(
      new CallbackWritableStream<RefCounted<const LogOperation>>(
        [&](const RefCounted<const LogOperation> & op)
        {
          //TODO: any kind of verification?
          //  it's not easy to check the size since in general this op size does
          //  not necessarily correspond to the input data size
        },
        [&]()
        {

        }
    ));

    deserializer->pipeTo(*callbackStream);

    while (bytesWritten < numRandomBytes)
    {
      size_t maxLength = (numRandomBytes < 1024) ? numRandomBytes : 1024;
      auto randomBytes = randomString(1, maxLength);
      bytesWritten += randomBytes.size();
      unparsedBytes += randomBytes.size();
      deserializer->write(randomBytes);
    }

    deserializer->close();
    callbackStream->close();
  }
}