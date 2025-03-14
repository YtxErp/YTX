/*
 * Copyright (C) 2023 YTX
 *
 * This file is part of YTX.
 *
 * YTX is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * YTX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with YTX. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef RESOURCEPOOL_H
#define RESOURCEPOOL_H

#include <QMutex>
#include <deque>

template <typename T>
concept Resettable = requires(T t) {
    { t.Reset() } -> std::same_as<void>;
};

template <typename Container>
concept Iterable = requires(Container c) {
    { c.begin() } -> std::same_as<typename Container::iterator>;
    { c.end() } -> std::same_as<typename Container::iterator>;
};

template <Resettable T> class ResourcePool {
public:
    static ResourcePool& Instance();
    T* Allocate();
    void Recycle(T* resource);
    template <Iterable Container> void Recycle(Container& resource_list);

private:
    ResourcePool();
    ~ResourcePool();

    ResourcePool(const ResourcePool&) = delete;
    ResourcePool& operator=(const ResourcePool&) = delete;
    ResourcePool(ResourcePool&&) = delete;
    ResourcePool& operator=(ResourcePool&&) = delete;

    void ExpandCapacity(int size);

private:
    std::deque<T*> pool_;
    QMutex mutex_;

    static constexpr qsizetype kSize { 100 };
    static constexpr qsizetype kExpandThreshold { 20 };
    static constexpr qsizetype kShrinkThreshold { 1001 };
};

template <Resettable T> ResourcePool<T>& ResourcePool<T>::Instance()
{
    static ResourcePool<T> instance {};
    return instance;
}

template <Resettable T> T* ResourcePool<T>::Allocate()
{
    QMutexLocker locker(&mutex_);

    if (pool_.size() <= kExpandThreshold)
        ExpandCapacity(kSize);

    if (pool_.empty()) {
        return new T();
    }

    T* resource { pool_.front() };
    pool_.pop_front();
    return resource;
}

template <Resettable T> void ResourcePool<T>::Recycle(T* resource)
{
    if (!resource)
        return;

    if (pool_.size() >= kShrinkThreshold) {
        delete resource;
        return;
    }

    QMutexLocker locker(&mutex_);
    resource->Reset();
    pool_.push_back(resource);
}

template <Resettable T> template <Iterable Container> void ResourcePool<T>::Recycle(Container& container)
{
    if (container.isEmpty())
        return;

    if (pool_.size() + container.size() >= kShrinkThreshold) {
        qDeleteAll(container);
    } else {
        QMutexLocker locker(&mutex_);

        for (T* resource : container) {
            if (resource) {
                resource->Reset();
                pool_.push_back(resource);
            }
        }
    }

    container.clear();
}

template <Resettable T> ResourcePool<T>::ResourcePool() { ExpandCapacity(kSize); }

template <Resettable T> void ResourcePool<T>::ExpandCapacity(int size)
{
    for (int i = 0; i < size; ++i) {
        pool_.push_back(new T());
    }
}

template <Resettable T> ResourcePool<T>::~ResourcePool()
{
    QMutexLocker locker(&mutex_);
    qDeleteAll(pool_);
    pool_.clear();
}

#endif // RESOURCEPOOL_H
