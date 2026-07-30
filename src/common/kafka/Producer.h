#pragma once

#include <string>

#include <google/protobuf/message.h>
#include <kafka/KafkaProducer.h>

namespace oms {

class Producer {
 public:
  explicit Producer(const std::string& brokers);

  void publish(const std::string& topic, const std::string& key,
               const google::protobuf::Message& message);

 private:
  ::kafka::clients::producer::KafkaProducer producer_;
};

}
