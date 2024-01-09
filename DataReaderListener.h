//
// Created by MikuSoft on 2024/1/5.
// Copyright (c) 2024 SiYuanHongRui All rights reserved.
//
// region Include
// region STL
#include "dds/sub/DataReaderListener.hpp"
#include "dds/sub/TDataReader.hpp"
#include "HelloWorldData.hpp"
// endregion
// region Self
// endregion
// endregion

// region Define
// endregion

// region Using NameSpace
// endregion

template<typename T>
class DataReaderListener : public dds::sub::DataReaderListener<T> {
public:
    int lastLoan = 0;
    int sampleCount = 0;
    std::function<void(const T &)> _f = nullptr;

    explicit DataReaderListener(const std::function<void(const T &)> &function) {
        _f = function;
    }

protected:
    template<typename V>
    using DataReader = dds::sub::DataReader<V>;

    void on_requested_deadline_missed(
            dds::sub::DataReader<T> &reader,
            const dds::core::status::RequestedDeadlineMissedStatus &status) {

    }

    void on_requested_incompatible_qos(
            DataReader<T> &reader,
            const dds::core::status::RequestedIncompatibleQosStatus &status) {

    }


    void on_sample_rejected(
            DataReader<T> &reader,
            const dds::core::status::SampleRejectedStatus &status) {

    }

    void on_liveliness_changed(
            DataReader<T> &reader,
            const dds::core::status::LivelinessChangedStatus &status) {

    }

    void on_subscription_matched(
            DataReader<T> &reader,
            const dds::core::status::SubscriptionMatchedStatus &status) {

    }

    void on_sample_lost(
            DataReader<T> &reader,
            const dds::core::status::SampleLostStatus &status) {
        std::cout << "tmlgbdelost le yige " << std::endl;
    }

    void on_data_available(dds::sub::DataReader<T> &reader) {
        sampleCount = 0;
        auto samples = reader.take();
        for (auto sample = samples.begin(); sample < samples.end(); ++sample) {
            T data = sample->data();
            _f(data);
            ++sampleCount;
        }
    }
};