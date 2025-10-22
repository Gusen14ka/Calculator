#include "plugin_helpers/platform.hpp"
#include "plugin_helpers/PluginHandle.hpp"
#include "logger/Logger.hpp"
#include <libloaderapi.h>

#define LOG Logger::instance()

std::string last_dll_error() {
#ifdef _WIN32
    DWORD code = GetLastError();
    if (code == 0) return {};
    LPSTR msg = nullptr;
    DWORD n = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&msg, 0, nullptr);
    std::string s;
    if (n && msg) {
        s.assign(msg, msg + n);
        LocalFree(msg);
    }
    return s;
#else
    const char* e = dlerror();
    return e ? std::string(e) : std::string();
#endif
}

LibHandle platform_load_dll(std::filesystem::path const & path) noexcept{
    LibHandle handle;
    #ifdef _WIN32
        handle = LoadLibraryA(path.string().c_str());
    #else
        dlerror();
        handle = dlopen(path.string().c_str(), RTLD_NOW);
    #endif

    if(!handle){
        LOG.error(last_dll_error() + ": " + path.string(), "platform::platform_load_dll");
    }
    return handle;
}

void platform_free_library(LibHandle handle) noexcept{
    if(!handle){
        return;
    }
    #ifdef _WIN32
        BOOL ok = FreeLibrary(handle);
        if(!ok){
            LOG.error(last_dll_error(), "platform::platform_free_library");
        }
    #else
        dlerror();
        int ok = dlclose(handle);
        if(ok != 0){
            LOG.error(last_dll_error(), "platform::platform_free_library");
        }
    #endif
}

void* platform_get_symbol(LibHandle handle, char const * name, std::string* out_err) noexcept{
    if(!handle){
        if(out_err) *out_err = "null library handle";
        LOG.error("LibHandle is nullptr", "platform::platform_get_symbol");
        return nullptr;
    }
    void * sym = nullptr;
    #ifdef _WIN32
        sym = reinterpret_cast<void*>(GetProcAddress(handle, name));
    #else
        dlerror();
        void * sym = dlsym(handle, name);
    #endif
    if(!sym){
        auto err = last_dll_error();
        if(out_err) *out_err = err;
        LOG.error(err, "platform::platform_get_symbol");
    }
    return sym;
}