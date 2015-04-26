#ifndef BEHAVIORTREEPROJ_BAIT_PRINT_STATIC_HPP
#define BEHAVIORTREEPROJ_BAIT_PRINT_STATIC_HPP

#include "bait_static.hpp"

#include <string>
#include <tuple>
#include <utility>

namespace bait {

namespace _detail_bait_print_static {

using std::make_integer_sequence;
using std::integer_sequence;
using std::string;
using std::tuple;
using std::get;

template<typename Stream, typename T>
void print_static(Stream& out, const T&, string indent) {
    out << indent << "LEAF,\n";
}

template<typename Stream, typename T>
void print_static(Stream& out, const StaticBT::inverter_t<T>& iv, string indent) {
    out << indent << "inverter(\n";
    print_static(out, iv.value, indent + "    ");
    out << indent << "),\n";
}

template<typename Stream, typename T>
void print_static(Stream& out, const StaticBT::until_fail_t<T>& iv, string indent) {
    out << indent << "until_fail(\n";
    print_static(out, iv.value, indent + "    ");
    out << indent << "),\n";
}

template<typename Stream, typename... Ts, size_t... Is>
void print_static(Stream& out, const tuple<Ts...>& tup, string indent, integer_sequence<size_t, Is...>) {
    using intarr = int[sizeof...(Ts)];
    intarr{(print_static(out, get<Is>(tup), indent), 1)...};
}

template<typename Stream, typename... Ts>
void print_static(Stream& out, const StaticBT::sequence_t<status::SUCCESS, Ts...>& iv, string indent) {
    out << indent << "sequence(\n";
    print_static(out, iv.value, indent + "    ", make_integer_sequence<size_t, sizeof...(Ts)>());
    out << indent << "),\n";
}

template<typename Stream, typename... Ts>
void print_static(Stream& out, const StaticBT::sequence_t<status::FAILURE, Ts...>& iv, string indent) {
    out << indent << "selector(\n";
    print_static(out, iv.value, indent + "    ", make_integer_sequence<size_t, sizeof...(Ts)>());
    out << indent << "),\n";
}

} // namespace _detail_bait_print_static

using _detail_bait_print_static::print_static;

} // namespace bait

#endif //BEHAVIORTREEPROJ_BAIT_PRINT_STATIC_HPP
