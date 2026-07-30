#include "common/kafka/Producer.h"

#include <string>

#include <kafka/Properties.h>
#include <kafka/Types.h>

namespace oms {

namespace {

::kafka::Properties buildProducerProps(const std::string& brokers) {
  ::kafka::Properties props;
  props.put("bootstrap.servers", brokers);
  props.put("acks", "all");
  return props;
}

}

Producer::Producer(const std::string& brokers)
    : producer_(buildProducerProps(brokers)) {}

void Producer::publish(const std::string& topic, const std::string& key,
                       const google::protobuf::Message& message) {
  std::string payload;
  message.SerializeToString(&payload);

  const ::kafka::clients::producer::ProducerRecord record(
      topic,
      ::kafka::Key(key.data(), key.size()),
      ::kafka::Value(payload.data(), payload.size()));

  producer_.syncSend(record);
}

}
