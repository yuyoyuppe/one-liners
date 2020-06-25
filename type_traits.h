#pragma once

#include <type_traits>

template<typename T, typename = void>
struct is_zero_arity_callable : std::false_type
{
};

template<typename F>
struct is_zero_arity_callable<F, std::void_t<decltype(std::declval<F>()())>> : std::true_type
{
};