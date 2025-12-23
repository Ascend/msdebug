// Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 
#include "HijackedLayerManager.h"
 
#include <unordered_map>
#include <unordered_set>
 
namespace {
 
using LayerSet = std::unordered_set<std::string>;
 
const std::unordered_map<std::string, LayerSet> &GetLayerMap() {
    static const std::unordered_map<std::string, LayerSet> ACL_TO_RUNTIME_MAP = {
        { "aclrtSetDeviceImpl", { "rtSetDevice" } },
        { "aclrtCreateContextImpl", { "rtCtxCreateEx" } },
        { "aclrtSynchronizeStreamWithTimeoutImpl", { "rtStreamSynchronizeWithTimeout" } },
    };
    return ACL_TO_RUNTIME_MAP;
}
 
} // namespace
 
void HijackedLayerManager::Push(std::string const &func)
{
    this->layers_.push_front(func);
}
 
void HijackedLayerManager::Pop(void)
{
    if (!this->layers_.empty()) {
        this->layers_.pop_front();
    }
}
 
bool HijackedLayerManager::InCallStack(std::string const &func) const
{
    if (this->layers_.empty()) {
        return false;
    }
 
    for (auto const &p : this->layers_) {
        auto it = GetLayerMap().find(p);
        if (it == GetLayerMap().cend()) {
            continue;
        }
        if (it->second.find(func) != it->second.cend()) {
            return true;
        }
    }
    return false;
}
 
LayerGuard::LayerGuard(HijackedLayerManager &manager, std::string const &func)
    : manager_{manager}
{
    manager.Push(func);
}
 
LayerGuard::~LayerGuard(void)
{
    this->manager_.Pop();
}
