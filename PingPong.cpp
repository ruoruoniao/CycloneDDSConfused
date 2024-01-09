//
// Created by MikuSoft on 2024/1/5.
// Copyright (c) 2024 SiYuanHongRui All rights reserved.
//

#include "dds/dds.hpp"
#include "HelloWorldData.hpp"
#include "DataReaderListener.h"

std::atomic_bool sentComplete{true};
std::atomic_int32_t sentIndex{0};
std::atomic_bool receive{false};

std::atomic_int64_t publishNum{0};
std::atomic_int64_t recvRequestNum{0};

std::atomic_int64_t subscribeNum{0};
std::atomic_int64_t requestNum{0};

auto lastTime = std::chrono::system_clock::now();
int64_t allZeroTime = 0;

void print_up_to(int64_t ms) {
    auto currentTime = std::chrono::system_clock::now();
    if (currentTime - lastTime > std::chrono::milliseconds(ms)) {
        lastTime = currentTime;
        int64_t result[4] = {0, 0, 0, 0};
        result[0] = publishNum.load();
        result[1] = recvRequestNum.load();
        result[2] = subscribeNum.load();
        result[3] = subscribeNum.load();
        bool allZero = true;
        for (long long i: result) {
            if (i != 0) {
                allZero = false;
                break;
            }
        }
        if (allZero) {
            allZeroTime += ms;
            return;
        }
        if (allZeroTime != 0) {
            ms = allZeroTime;
            allZeroTime = 0;
        }
        std::cout <<
                  "In " << ms << "ms: \n" <<
                  "    Publish: " << result[0] << "\n" <<
                  "    Receive Request: " << result[1] << "\n" <<
                  "    Subscribe: " << result[2] << "\n" <<
                  "    Post Request: " << result[3] << std::endl;
        publishNum = 0;
        recvRequestNum = 0;
        subscribeNum = 0;
        requestNum = 0;
    }
}

[[noreturn]]
int main(int argc, char **argv) {
    if (argc < 5) {
        std::cout << "PingPong.exe publishTopic subscribeTopic size first" << std::endl;
    }
    std::string publishTopic = argv[1];
    std::string subscribeTopic = argv[2];
    int64_t size = std::stoi(std::string(argv[3]));
    if (std::string(argv[4]) == "true" || std::string(argv[4]) == "true ") {
        receive = true;
    }
    char *data = (char *) malloc(size);
    memset(data, '6', size);
    HelloWorldData::Msg msg(0, 0, 0, data, false);

    dds::domain::DomainParticipant participant(org::eclipse::cyclonedds::domain::default_id());

    dds::topic::Topic<HelloWorldData::Msg> topicP(participant, publishTopic);
    dds::topic::Topic<HelloWorldData::Msg> topicPR(participant, publishTopic + "_Request");
    dds::topic::Topic<HelloWorldData::Msg> topicS(participant, subscribeTopic);
    dds::topic::Topic<HelloWorldData::Msg> topicSR(participant, subscribeTopic + "_Request");

    std::cout << "Create writer." << std::endl;

    dds::pub::Publisher publisher(participant);
    auto w_qos = publisher.default_datawriter_qos();
    w_qos->policy(dds::core::policy::Reliability::Reliable());
    dds::pub::DataWriter<HelloWorldData::Msg> writerP(publisher, topicP, w_qos);
    dds::pub::DataWriter<HelloWorldData::Msg> writerSR(publisher, topicSR, w_qos);

    dds::sub::Subscriber subscriber(participant);
    auto r_qos = subscriber.default_datareader_qos();
    r_qos->policy(dds::core::policy::Reliability::Reliable());
    auto listenerPR = new DataReaderListener<HelloWorldData::Msg>([](const HelloWorldData::Msg &data) {
        if (data.index() + 1 == sentIndex) {
            sentComplete = true;
            recvRequestNum.fetch_add(1);
        }
    });
    dds::sub::DataReader<HelloWorldData::Msg> readerPR(subscriber, topicPR,
                                                       r_qos,
                                                       listenerPR,
                                                       dds::core::status::StatusMask::all());

    auto listenerS = new DataReaderListener<HelloWorldData::Msg>([&writerSR](const HelloWorldData::Msg &data) {
        receive = true;
        int id = data.index();
        subscribeNum.fetch_add(1);
        HelloWorldData::Msg request(id, 0, 0, "", false);
        writerSR.write(request);
        requestNum.fetch_add(1);
    });
    dds::sub::DataReader<HelloWorldData::Msg> readerS(subscriber, topicS,
                                                      r_qos,
                                                      listenerS,
                                                      dds::core::status::StatusMask::all());

#if _WIN32
    Sleep(2 * 1000);
#else
    usleep(2 * 1000 * 1000);
#endif

    while (true) {
        if (receive) {
            receive = false;
            sentComplete = false;
            publishNum.fetch_add(1);
            msg.index() = sentIndex.fetch_add(1);
            writerP.write(msg);
        }
        while (!sentComplete) {
            print_up_to(1000);
        }
        print_up_to(1000);
    }
}