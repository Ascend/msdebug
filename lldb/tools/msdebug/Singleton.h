// Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.

#ifndef __SINGLETON_H__
#define __SINGLETON_H__


template <typename T, bool threadLocal, typename... Args>
class Singleton {
public:
    inline static T &Instance(Args &&... args);
    Singleton(Singleton const &) = delete;
    Singleton &operator=(Singleton const &) = delete;

protected:
    Singleton(void) = default;
    ~Singleton(void) = default;
};

template <typename T, bool threadLocal, typename... Args>
T &Singleton<T, threadLocal, Args...>::Instance(Args &&... args)
{
    static T instance(std::forward<Args>(args)...);
    return instance;
}

template <typename T, typename... Args>
class Singleton<T, true, Args...> {
public:
    inline static T &Instance(Args &&... args);
    Singleton(Singleton const &) = delete;
    Singleton &operator=(Singleton const &) = delete;

protected:
    Singleton(void) = default;
    ~Singleton(void) = default;
};

template <typename T, typename... Args>
T &Singleton<T, true, Args...>::Instance(Args &&... args)
{
    thread_local static T instance(std::forward<Args>(args)...);
    return instance;
}

#endif // __SINGLETON_H__
