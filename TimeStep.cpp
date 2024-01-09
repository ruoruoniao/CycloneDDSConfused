//
// Created by MikuSoft on 2024/1/5.
//

#include "dds/dds.hpp"
#include "Time.hpp"
#include "DataReaderListener.h"

bool server = false;

uint16_t step = 1;
uint64_t currentLogicTime = 0;
std::unordered_map<std::string, uint64_t> timeSaver{};

auto lastTime = std::chrono::system_clock::now();
uint64_t lastLogicTime = 0;
int64_t allZeroTime = 0;

void print_up_to(int64_t ms) {
    auto changeLogicTime = currentLogicTime - lastLogicTime;
    auto currentTime = std::chrono::system_clock::now();
    if (currentTime - lastTime > std::chrono::milliseconds(ms)) {
        lastTime = currentTime;
        bool zero = changeLogicTime == 0;
        if (zero) {
            allZeroTime += ms;
            return;
        }
        if (allZeroTime != 0) {
            ms = allZeroTime;
            allZeroTime = 0;
        }
        lastLogicTime = currentLogicTime;
        std::cout <<
                  "In " << ms << "ms: \n" <<
                  "    Step: " << changeLogicTime <<
                  "    Time: " << currentLogicTime << std::endl;
    }
}


[[noreturn]]
int main(int argc, char **argv) {
    if (argc < 3) {
        std::cout << "TimeStep.exe name nodeCount " << std::endl;
    }
    std::string selfName = argv[1];
    int64_t size = std::stoi(std::string(argv[2]));

    dds::domain::DomainParticipant participant(org::eclipse::cyclonedds::domain::default_id());

    dds::pub::Publisher publisher(participant);
    auto w_qos = publisher.default_datawriter_qos();
    w_qos->policy(dds::core::policy::Reliability::Reliable());

    dds::pub::DataWriter<Time::Boardcast> writer =
            dds::pub::DataWriter<Time::Boardcast>(publisher, dds::topic::Topic<Time::Boardcast>{participant, "time"},
                                                  w_qos);

    dds::sub::Subscriber subscriber(participant);
    auto r_qos = subscriber.default_datareader_qos();
    r_qos->policy(dds::core::policy::Reliability::Reliable());
    auto listener = new DataReaderListener<Time::Boardcast>([&selfName](const Time::Boardcast &data) {
        if (data.from() == selfName) {
            return;
        }
        timeSaver[data.from()] = data.time();
    });
    std::vector<dds::sub::DataReader<Time::Boardcast>> readerList{};
    readerList.emplace_back(subscriber,
                            dds::topic::Topic<Time::Boardcast>{participant, "time"},
                            r_qos, listener,
                            dds::core::status::StatusMask::all());


#if _WIN32
    Sleep(2 * 1000);
#else
    usleep(2 * 1000 * 1000);
#endif

    while (true) {
        currentLogicTime += step;
        Time::Boardcast time{currentLogicTime, selfName};
        writer.write(time);
        bool canRun = false;
        while (!canRun) {
            print_up_to(1000);
            if (timeSaver.size() + 1 < size) {
                continue;
            }
            bool result = true;
            for (auto &[n, t]: timeSaver) {
                if (t < currentLogicTime) {
                    result = false;
                    continue;
                }
            }
            canRun = result;
        }

        print_up_to(1000);
    }
}