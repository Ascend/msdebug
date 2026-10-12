/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
 */

#ifndef __SINGLETON_H__
#define __SINGLETON_H__

#include <utility>

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
