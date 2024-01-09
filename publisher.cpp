/*
 * Copyright(c) 2006 to 2020 ZettaScale Technology and others
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v. 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
 * v. 1.0 which is available at
 * http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 */
#include <cstdlib>
#include <iostream>
#include <chrono>
#include <thread>

/* Include the C++ DDS API. */
#include "dds/dds.hpp"

/* Include data type and specific traits to be used with the C++ DDS API. */
#include "HelloWorldData.hpp"

using namespace org::eclipse::cyclonedds;

uint64_t realTime = 0;
uint64_t times = 0;

auto lastTime = std::chrono::system_clock::now();

int64_t zeroTime = 0;

void print_up_to(int64_t ms) {
    auto currentTime = std::chrono::system_clock::now();
    if (currentTime - lastTime > std::chrono::milliseconds(ms)) {
        lastTime = currentTime;
        bool zero = times == 0;
        if (zero) {
            zeroTime += ms;
            return;
        }
        if (zeroTime != 0) {
            ms = zeroTime;
            zeroTime = 0;
        }
        std::cout <<
                  "In " << ms << "ms: \n" <<
                  "    Step: " << times << std::endl;
        times = 0;
    }
}

int main() {
    try {
        std::cout << "=== [Publisher] Create writer." << std::endl;

        /* First, a domain participant is needed.
         * Create one on the default domain. */
        dds::domain::DomainParticipant participant(domain::default_id());

        /* To publish something, a topic is needed. */
        std::vector<dds::topic::Topic<HelloWorldData::Msg>> topicList{};
        for (int i = 0; i < 2000; ++i) {
            topicList.emplace_back(participant, "HelloWorld_" + std::to_string(i));
        }

        /* A writer also needs a publisher. */
        dds::pub::Publisher publisher(participant);

        /* Now, the writer can be created to publish a HelloWorld message. */
        auto w_qos = publisher.default_datawriter_qos();
        w_qos.policy(dds::core::policy::Reliability::Reliable());
        std::vector<dds::pub::DataWriter<HelloWorldData::Msg>> writerList{};
        for (int i = 0; i < 2000; ++i) {
            writerList.emplace_back(publisher, topicList[i]);
        }

        /* For this example, we'd like to have a subscriber to actually read
         * our message. This is not always necessary. Also, the way it is
         * done here is just to illustrate the easiest way to do so. It isn't
         * really recommended to do a wait in a polling loop, however.
         * Please take a look at Listeners and WaitSets for much better
         * solutions, albeit somewhat more elaborate ones. */
#if _WIN32
        Sleep(2 * 1000);
#else
        usleep(2 * 1000 * 1000);
#endif

        /* Create a message to write. */
        HelloWorldData::Msg msg(1, "Hello World");

        /* Write the message. */
        std::cout << "=== [Publisher] Write sample." << std::endl;
        while (true) {
            writerList[(realTime + 0) % 1000].write(msg);
            realTime += 1;
            times += 1;
            print_up_to(1000);
        }
    }
    catch (const dds::core::Exception &e) {
        std::cerr << "=== [Publisher] Exception: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
