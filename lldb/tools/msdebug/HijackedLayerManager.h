// Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 
#ifndef CORE_HIJACKED_LAYER_MANAGER_H
#define CORE_HIJACKED_LAYER_MANAGER_H
 
#include <list>
#include <string>
 
#include "Singleton.h"
 
/** 劫持接口层级管理
 * CANN 软件栈中存在不同接口的层级调用，如 aclrtSetDevice -> rtSetDevice，劫持桩
 * 当前会同时对不同层级的接口做劫持。为了防止不同层级的接口上报相同的信息，
 * 需要对层级之间的 调用关系进行处理。本类中以栈的形式保存劫持接口的调用上
 * 下文，高层接口和低层接口之间的 调用关系以 map 保存。当低层接口被劫持时，
 * 需要检查上下文中所有的高层接口是否会 掩盖掉当前的劫持接口
 *
 * 限制：HijackedLayerManager 类当前仅能处理单层接口调用
 */
class HijackedLayerManager : public Singleton<HijackedLayerManager, true> {
public:
    /** 将当前劫持的函数入栈，在该上下文中，被 func 调用的低层函数会被掩掉
     * @param func - [in] 要管理的高层函数
     */
    void Push(std::string const &func);
 
    /** 将栈顶的函数出栈
     */
    void Pop(void);
 
    /** 判断一个函数是否应该被掩掉
     * @param func - [in] 要校验的函数
     */
    bool InCallStack(std::string const &func) const;
 
private:
    std::list<std::string> layers_;
};
 
/** 利用 RAII 实现自动出入栈
 */
class LayerGuard {
public:
    LayerGuard(HijackedLayerManager &manager, std::string const &func);
    ~LayerGuard(void);
 
private:
    HijackedLayerManager &manager_;
};
 
#endif // CORE_HIJACKED_LAYER_MANAGER_H
