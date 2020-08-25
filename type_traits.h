#pragma once

#include <type_traits>
#include <tuple>

template<typename T, typename = void>
struct is_zero_arity_callable : std::false_type
{
};

template<typename F>
struct is_zero_arity_callable<F, std::void_t<decltype(std::declval<F>()())>> : std::true_type { };

namespace detail {
template <typename ReturnT, typename... Args>
struct function_traits
{
  using args_t     = std::tuple<Args...>;
  using return_t   = ReturnT;
  using func_ptr_t = ReturnT (*)(Args...);
  template <size_t arg_idx>
  using arg_t                          = std::tuple_element_t<arg_idx, args_t>;
  static constexpr inline size_t arity = sizeof...(Args);

  static inline return_t empty(Args...)
  {
    static_assert(std::is_default_constructible_v<return_t> || std::is_same_v<return_t, void>,
                  "can't use empty(): return type must be default constructible!");
    if constexpr(!std::is_same_v<return_t, void>)
    {
      return {};
    }
  }
};
}

template <typename Callable>
struct function_traits : public function_traits<decltype(&Callable::operator())>
{
  // functor proxy
};

template <typename ReturnT, typename... Args>
struct function_traits<ReturnT (*)(Args...)> : detail::function_traits<ReturnT, Args...>
{
  // free-function pointer specialiazation
};

template <typename ReturnT, typename... Args>
struct function_traits<ReturnT(Args...)> : detail::function_traits<ReturnT, Args...>
{
  // free-function specialiazation
};

template <typename ClassT, typename ReturnT, typename... Args>
struct function_traits<ReturnT (ClassT::*)(Args...) const> : detail::function_traits<ReturnT, Args...>
{
  // lambda specialization
};

template <typename ClassT, typename ReturnT, typename... Args>
struct function_traits<ReturnT (ClassT::*)(Args...)> : detail::function_traits<ReturnT, Args...>
{
  // method specialization
};