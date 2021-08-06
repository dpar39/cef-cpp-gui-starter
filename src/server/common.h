#pragma once

#include <memory>
#include <string>

#define FWD_DECL(Type)                                                                                                 \
    class Type;                                                                                                        \
    using Type##Ptr = std::shared_ptr<Type>;                                                                           \
    using Type##UPtr = std::unique_ptr<Type>;                                                                          \
    using Type##WPtr = std::weak_ptr<Type>;
