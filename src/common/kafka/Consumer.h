#pragma once

#include <chrono>
#include <string>
#include <vector>

#include <kafka/KafkaConsumer.h>

namespace oms {

class Consumer {
 public:
  struct Record {
    std::string topic;
    std::string key;
    std::string payload;
  };

  Consumer(const std::string& brokers, const std::string& groupId,
           const std::vector<std::string>& topics, bool fromBeginning = true);

  std::vector<Record> poll(std::chrono::milliseconds timeout);

 private:
  ::kafka::clients::consumer::KafkaConsumer consumer_;
};

}
