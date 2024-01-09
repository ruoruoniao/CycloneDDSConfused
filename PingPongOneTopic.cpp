//
// Created by MikuSoft on 2024/1/5.
//

#include "dds/dds.hpp"
#include "HelloWorldData.hpp"
#include "DataReaderListener.h"
#include "thread"
#include "DoubleBufferedQueue.h"

#include "mutex"

std::mutex w_m{};
DoubleBufferedQueue<HelloWorldData::Msg> que{};


DataReaderListener<HelloWorldData::Msg> *listener;

int recordPublishIndex = 0;
int recordRecvRequestIndex = 0;

int recordSubscribeIndex = 0;
int recordRequestIndex = 0;

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
            if (allZeroTime > 30 * ms && allZeroTime < 32 * ms) {
                std::cout << "Stuck: \n"
                          << "    Publish index: " << recordPublishIndex << "\n"
                          << "    RecvRequest index: " << recordRecvRequestIndex << "\n"
                          << "    Subscribe index: " << recordSubscribeIndex << "\n"
                          << "    Request index: " << recordRequestIndex << "\n"
                          << "    Last sample compute: " << listener->sampleCount << "\n"
                          << "    IsReceive: " << receive.load() << "\n"
                          << "    Is Sent Complete: " << sentComplete.load() << "\n" << std::endl;
            }
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

#include "memory"

std::atomic_bool readyToRequest{false};
std::string requestTo{};
int requestIndex = 0;

[[noreturn]]
int main(int argc, char **argv) {
    if (argc < 5) {
        std::cout << "PingPong.exe selfName toName size first" << std::endl;
    }

    std::string selfName = argv[1];
    std::string toName = argv[2];
    int64_t size = std::stoi(std::string(argv[3]));
    if (std::string(argv[4]) == "true" || std::string(argv[4]) == "true ") {
        receive = true;
    }
    char *data = (char *) malloc(size);
    memset(data, '6', size);
    HelloWorldData::Msg msg(0, selfName, toName, data, false);

    dds::domain::DomainParticipant participant(org::eclipse::cyclonedds::domain::default_id());
    dds::topic::Topic<HelloWorldData::Msg> topic(participant, "HelloWorldTest");


    std::cout << "Create writer." << std::endl;

    dds::pub::Publisher publisher(participant);
    auto w_qos = publisher.default_datawriter_qos();
    w_qos->policy(dds::core::policy::Reliability::Reliable(dds::core::Duration::from_secs(30)));
    w_qos->policy(dds::core::policy::History::KeepAll());
    dds::pub::DataWriter<HelloWorldData::Msg> writer(publisher, topic, w_qos);

    dds::sub::Subscriber subscriber(participant);
    auto r_qos = subscriber.default_datareader_qos();
    r_qos->policy(dds::core::policy::Reliability::Reliable(dds::core::Duration::from_secs(30)));
    r_qos->policy(dds::core::policy::History::KeepAll());
    listener = new DataReaderListener<HelloWorldData::Msg>([&selfName, &writer](const HelloWorldData::Msg &data) {
        if (data.to() != selfName) {
            return;
        }
        if (!data.request()) {
            //Subscribe
            requestIndex = data.index();
            requestTo = data.from();
            subscribeNum.fetch_add(1);
            readyToRequest = true;
            recordSubscribeIndex = data.index();
            que.push(HelloWorldData::Msg{data.index(), selfName, data.from(), "", true});
            receive = true;
        } else {
            //Request
            if (data.index() + 1 == sentIndex) {
                recvRequestNum.fetch_add(1);
                recordRecvRequestIndex = data.index();
                sentComplete = true;
            }
        }
    });

    std::thread t([&writer]() {
        while (true) {
            if (!que.empty()) {
                auto o_m = que.pop();
                if (!o_m) {
                    continue;
                }
                {
                    std::lock_guard lock(w_m);
                    writer.write(o_m.value());
                }
                recordRequestIndex = requestIndex;
                readyToRequest = false;
            }
        }
    });
    t.detach();
    dds::sub::DataReader<HelloWorldData::Msg> reader(subscriber, topic,
                                                     r_qos,
                                                     listener,
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
            {
                std::lock_guard lock(w_m);
                writer.write(msg);
            }
            recordPublishIndex = msg.index();
        }
        while (!sentComplete) {
            print_up_to(1000);
        }
        print_up_to(1000);
    }
}