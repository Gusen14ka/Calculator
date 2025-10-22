#pragma once

#ifdef _WIN32
    #ifdef BUILD_PLUGIN
        #define PLUGIN_EXPORT extern "C" __declspec(dllexport)
    #else
        #define PLUGIN_EXPORT extern "C" __declspec(dllimport)
    #endif
#else
    #define PLUGIN_EXPORT extern "C"
#endif

constexpr unsigned ABI_VERSION = 1;
constexpr unsigned ERROR_BUF_SIZE = 256;

// Тип функции плагина - возвращает double
using plugin_func_t =  double (*)(unsigned argc, double const* argv,
    int* err, char* err_msg, int err_msg_size);

struct HostApi{
    // Функция логирования
    void (*log)(char const* msg, unsigned msg_size);

    // Сообщение об ошибке
    void (*report_error)(char const* msg, unsigned msg_size);
};

struct PluginInfo{
    // Имя для функции в плагине
    char const * name;

    // Длина имени (для точной валидации)
    unsigned name_len;

    // Псевдонимы функции
    char const * const * aliases;

    // Длины псевдонимов
    unsigned const * alias_lens;

    // Кол-во псевдонимов
    unsigned alias_count;

    // Минимальное число аргументов
    unsigned min_args;
    
    // Максимальное число аргументов; если -1, то переменное
    unsigned max_args;

    // Указатель на функцию
    plugin_func_t func;

    // Версия ABI
    unsigned abi_version;

    // Описание
    char const* description;

    // Является ли оператором
    bool is_oper;

    // Является ли правоассоциативным оператором
    bool is_right_assoc_oper;
};

// Экспортируемые функции в плагине:
// 1) вернуть info
using plugin_get_info_t =  int (*)(PluginInfo** const out_info);

// 2) инициализация плагина: передаём HostApi
using plugin_init_t = int (*)(HostApi const* host, char* err_msg, int err_msg_size);

// 3) выгрузка
using plugin_shutdown_t =  int (*)();