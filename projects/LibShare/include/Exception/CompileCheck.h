#pragma once

#include <type_traits>

#define CHECK_CLASS(className)                                                                     \
    static_assert((std::is_polymorphic<className>::value                                           \
                       ? std::has_virtual_destructor<className>::value                             \
                       : true),                                                                    \
                  "class " #className " has virtual function(s) but destructor is not virtual.")