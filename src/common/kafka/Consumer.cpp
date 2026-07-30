#include "common/kafka/Consumer.h"

#include <utility>

#include <kafka/Properties.h>
#include <kafka/Types.h>

namespace oms {

namespace {

::kafka::Properties buildConsumerProps(const std::string& brokers,
                                       const std::string& groupId,
                                       bool fromBeginning) {
  ::kafka::Properties props;
  props.put("bootstrap.servers", brokers);
  props.put("group.id", groupId);
  props.put("enable.auto.commit", "true");
  props.put("auto.offset.reset", fromBeginning ? "earliest" : "latest");
  return props;
}

}

Consumer::Consumer(const std::string& brokers, const std::string& groupId,
                   const std::vector<std::string>& topics, bool fromBeginning)
    : consumer_(buildConsumerProps(brokers, groupId, fromBeginning)) {
  const ::kafka::Topics topicSet(topics.begin(), topics.end());
  consumer_.subscribe(topicSet);
}

std::vector<Consumer::Record> Consumer::poll(std::chrono::milliseconds timeout) {
  std::vector<Record> out;
  const auto records = consumer_.poll(timeout);
  out.reserve(records.size());

  for (const auto& record : records) {
    if (record.error()) {
      continue;
    }
    Record rec;
    rec.topic = record.topic();
    rec.key.assign(static_cast<const char*>(record.key().data()),
                   record.key().size());
    rec.payload.assign(static_cast<const char*>(record.value().data()),
                       record.value().size());
    out.push_back(std::move(rec));
  }
  return out;
}

}
