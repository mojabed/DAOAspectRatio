#pragma once

#include <windows.h>
#include <d3d9.h>
#include <mutex>
#include "logger.h"

template<typename T>
class HookManager {
public:
    HookManager(void** vtable, int index, T hookFunc);
    ~HookManager();

    bool apply();
    bool remove();
    void* trampoline(void* target, void* hook);

    T getOriginal() const;

private:
    void** vtable;
    int index;
    T hookFunc;
    T originalFunc;
    DWORD oldProtect;
    std::mutex hookMutex;
};

template<typename T>
HookManager<T>::HookManager(void** vtable, int index, T hookFunc)
    : vtable(vtable), index(index), hookFunc(hookFunc), originalFunc(nullptr), oldProtect(0) {}

template<typename T>
HookManager<T>::~HookManager() {
    //remove();
}

template<typename T>
bool HookManager<T>::apply() {
    //std::lock_guard<std::mutex> lock(hookMutex);
    bool success = true;
	Logger::getInstance().log("Hooking function");

    if (!vtable || !hookFunc) {
        Logger::getInstance().logError("Invalid vtable or hook function");
        return false;
    }

    originalFunc = reinterpret_cast<T>(vtable[index]);
    if (!originalFunc) {
        Logger::getInstance().logError("Failed to get original function pointer");
        return false;
    }

    if (!VirtualProtect(&vtable[index], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect)) {
        Logger::getInstance().logError("Failed to change memory protection for vtable");
        return false;
    }

    vtable[index] = reinterpret_cast<void*>(hookFunc);
    if (!VirtualProtect(&vtable[index], sizeof(void*), oldProtect, &oldProtect)) {
        Logger::getInstance().logError("Failed to restore memory protection for vtable");
        return false;
    }

    Logger::getInstance().log("Function hooked successfully");
    return success;
}

template<typename T>
bool HookManager<T>::remove() {
    //std::lock_guard<std::mutex> lock(hookMutex);

    bool success = true;

    if (!vtable || !originalFunc) {
        Logger::getInstance().logError("Failed to get original function pointer");
        return false;
    }

    if (!VirtualProtect(&vtable[index], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect)) {
        Logger::getInstance().logError("Failed to change memory protection for vtable");
        return false;
    }

    vtable[index] = reinterpret_cast<void*>(&originalFunc);
    if (!VirtualProtect(&vtable[index], sizeof(void*), oldProtect, &oldProtect)) {
        Logger::getInstance().logError("Failed to restore memory protection for vtable");
        return false;
    }

    Logger::getInstance().log("Function unhooked successfully");
    return success;
}

template<typename T>
void* HookManager<T>::trampoline(void* target, void* hook) {
    DWORD oldProtect;
    BYTE* trampoline = (BYTE*)VirtualAlloc(nullptr, 16, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!trampoline) {
        Logger::getInstance().logError("Failed to allocate memory for trampoline");
        return nullptr;
    }

    Logger::getInstance().log(std::format("Trampoline allocated at address: {}", static_cast<void*>(trampoline)));

    std::memcpy(trampoline, target, 5); // original bytes of the target function

    uintptr_t jumpBack = (uintptr_t)target + 5;
    trampoline[5] = 0xE9; // JMP instruction
    *(uintptr_t*)(trampoline + 6) = jumpBack - (uintptr_t)(trampoline + 10); // relative offset from trampoline to target + 5

    // Write hook jump to target function
    if (!VirtualProtect(target, 5, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        Logger::getInstance().logError("Failed to change memory protection for target function");
        VirtualFree(trampoline, 0, MEM_RELEASE);
        return nullptr;
    }

    ((BYTE*)target)[0] = 0xE9; // JMP instruction
    *(uintptr_t*)((BYTE*)target + 1) = (uintptr_t)hook - (uintptr_t)target - 5; // relative offset from target to hook

    if (!VirtualProtect(target, 5, oldProtect, &oldProtect)) {
        Logger::getInstance().logError("Failed to restore memory protection for target function");
        VirtualFree(trampoline, 0, MEM_RELEASE);
        return nullptr;
    }

    return trampoline;
}

template<typename T>
T HookManager<T>::getOriginal() const {
    return originalFunc;
}