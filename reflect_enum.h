#pragma once
#include <array>
#include <utility>

size_t constexpr str_length(const char * str) { return *str ? 1 + str_length(str + 1) : 0; }

template <size_t O, size_t... Is>
std::index_sequence<(O + Is)...> add_offset(std::index_sequence<Is...>)
{
  return {};
}

template <size_t O, size_t N>
auto make_index_sequence_with_offset()
{
  return add_offset<O>(std::make_index_sequence<N>{});
}

constexpr size_t func_sig_prefix_len = 29;
constexpr size_t func_sig_suffix_len = 7;
template <int test_val>
constexpr auto enum_value_name()
{
  return std::array<const char *, 2>{__FUNCSIG__ + func_sig_prefix_len,
                                     __FUNCSIG__ + func_sig_prefix_len + str_length(__FUNCSIG__ + func_sig_prefix_len) -
                                       func_sig_suffix_len};
}

template <typename EnumType, size_t... enum_vals, size_t enum_length = sizeof...(enum_vals)>
constexpr auto enum_value_names_impl(std::index_sequence<enum_vals...>)
{
  using enum_name_t = std::array<const char *, 2>;
  return std::array<enum_name_t, enum_length>{enum_value_name<static_cast<EnumType>(enum_vals)>()...};
}

template <typename EnumType, EnumType first_value, EnumType last_value>
constexpr auto enum_value_names()
{
  return enum_value_names_impl<EnumType>(make_index_sequence_with_offset<first_value, last_value>());
}
