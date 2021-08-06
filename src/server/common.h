#pragma once

#include <memory>

#define FWD_DECL(Type)                                                                                                 \
    class Type;                                                                                                        \
    using Type##Ptr = std::shared_ptr<Type>;                                                                           \
    using Type##UPtr = std::unique_ptr<Type>;                                                                          \
    using Type##WPtr = std::weak_ptr<Type>;
