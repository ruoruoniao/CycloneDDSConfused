//
// Created by MikuSoft on 2022/10/13.
// Copyright (c) 2022 SiYuanHongRui All rights reserved.
//
#ifndef JTYJ_AlphaBase_BaseLibrary_DataStruct_DoubleBufferedQueue_H
#define JTYJ_AlphaBase_BaseLibrary_DataStruct_DoubleBufferedQueue_H

// region Include
// region STL
#include "list"
#include "mutex"
#include "atomic"
#include "optional"
// endregion
// region Self
// endregion
// endregion

// region Using NameSpace
// endregion


template<typename ElementType>
class DoubleBufferedQueue {
// region 构造函数
public:
    DoubleBufferedQueue() noexcept = default;
// endregion

// region 公有属性
public:
    using SizeType = typename std::list<ElementType>::size_type;
// endregion

// region 公有方法
public:
    [[nodiscard]]
    [[maybe_unused]]
    auto operator()() -> std::list<ElementType> {
        std::list<ElementType> list;
        this->pop(list);
        return list;
    }

    [[nodiscard]]
    [[maybe_unused]]
    inline auto size() const noexcept -> size_t {
        return _innerQueueSize;
    }

    [[nodiscard]]
    [[maybe_unused]]
    inline auto empty() const noexcept -> bool {
        return size() == 0;
    }
// endregion

// region Get/Set选择器
public:
    [[maybe_unused]]
    inline auto push(const ElementType &element) noexcept -> SizeType {
        std::lock_guard lock(_innerWriteLock);
        _innerListWrite.emplace_back(element);
        add(1);
        return _innerQueueSize;
    }

    [[maybe_unused]]
    inline auto push(std::list<ElementType> &elementList) noexcept -> SizeType {
        std::lock_guard lock(_innerWriteLock);
        SizeType size = elementList.size();
        _innerListWrite.splice(_innerListWrite.cend(), elementList);
        add(size);
        return _innerQueueSize;
    }

    [[maybe_unused]]
    inline auto push(ElementType &&element) noexcept -> SizeType {
        std::lock_guard lock(_innerWriteLock);
        _innerListWrite.emplace_back(element);
        add(1);
        return _innerQueueSize;
    }

    [[maybe_unused]]
    inline auto push(std::list<ElementType> &&elementList) noexcept -> SizeType {
        std::lock_guard lock(_innerWriteLock);
        SizeType size = elementList.size();
        _innerListWrite.splice(_innerListWrite.cend(), elementList);
        add(size);
        return _innerQueueSize;
    }

    [[nodiscard]]
    [[maybe_unused]]
    inline auto front() -> std::optional<ElementType> {
        std::lock_guard lock(_innerReadLock);
        if (size() == 0) {
            return std::nullopt;
        }
        if (_innerListRead.empty()) {
            std::lock_guard lock1(_innerWriteLock);
            _innerListWrite.swap(_innerListRead);
        }
        std::optional result = _innerListRead.front();
        return result;
    }

    [[nodiscard]]
    [[maybe_unused]]
    inline auto pop() -> std::optional<ElementType> {
        std::lock_guard lock(_innerReadLock);
        if (size() == 0) {
            return std::nullopt;
        }
        if (_innerListRead.empty()) {
            std::lock_guard lock1(_innerWriteLock);
            _innerListWrite.swap(_innerListRead);
        }
        std::optional result = _innerListRead.front();
        _innerListRead.pop_front();
        subtract(1);
        return result;
    }

    [[maybe_unused]]
    inline auto pop(std::list<ElementType> &elementList) noexcept -> bool {
        std::lock_guard lock(_innerReadLock);
        std::lock_guard lock1(_innerWriteLock);
        if (size() == 0) {
            return false;
        }
        if (!_innerListRead.empty()) {
            elementList.splice(elementList.cend(), _innerListRead);
        }
        if (!_innerListWrite.empty()) {
            elementList.splice(elementList.cend(), _innerListWrite);
        }
        _innerQueueSize = 0;
        return true;
    }

    inline void clear() {
        std::scoped_lock lock(_innerReadLock, _innerWriteLock);
        std::list<ElementType>().swap(_innerListRead);
        std::list<ElementType>().swap(_innerListWrite);
        _innerQueueSize = 0;
    }
// endregion

// region 私有变量
private:
    /*!
     * 写队列
     */
    std::list<ElementType> _innerListWrite{};
    /*!
     * 读队列
     */
    std::list<ElementType> _innerListRead{};
    /*!
     * 读取线程锁
     */
    std::mutex _innerWriteLock{};
    /*!
     * 写入线程锁
     */
    std::mutex _innerReadLock{};
    /*!
     * 队列长度
     */
    std::atomic<SizeType> _innerQueueSize = 0;
// endregion

// region 私有方法
private:
    auto add(size_t count) noexcept {
        return _innerQueueSize.fetch_add(count, std::memory_order_relaxed);
    }

    auto subtract(size_t count) noexcept {
        return _innerQueueSize.fetch_sub(count, std::memory_order_relaxed);
    }
// endregion
};

#endif //JTYJ_AlphaBase_BaseLibrary_DataStruct_DoubleBufferedQueue_H