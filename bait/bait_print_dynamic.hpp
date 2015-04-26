#ifndef BEHAVIORTREEPROJ_BAIT_PRINT_DYNAMIC_HPP
#define BEHAVIORTREEPROJ_BAIT_PRINT_DYNAMIC_HPP

#include "bait_dynamic.hpp"

#include <string>

namespace bait {

namespace _detail_bait_print_dynamic {

using std::string;

template <typename Stream, typename... Args>
void print_dynamic(Stream& out, const typename DynamicBT<Args...>::Func& func, string indent);

template <typename Stream, typename... Args>
void print_dynamic(Stream& out, const typename DynamicBT<Args...>::RawSequence& seq, string indent) {
    out << indent << "sequence(\n";
    for (auto& f : seq.children) {
        print_dynamic(out, f, indent + "    ");
    }
    out << indent << "),\n";
}

template <typename Stream, typename... Args>
void print_dynamic(Stream& out, const typename DynamicBT<Args...>::RawSelector& seq, string indent) {
    out << indent << "selector(\n";
    for (auto& f : seq.children) {
        print_dynamic(out, f, indent + "    ");
    }
    out << indent << "),\n";
}

template <typename Stream, typename... Args>
void print_dynamic(Stream& out, const typename DynamicBT<Args...>::RawInverter& inv, string indent) {
    out << indent << "inverter(\n";
    print_dynamic(out, inv.child, indent + "    ");
    out << indent << "),\n";
}

template <typename Stream, typename... Args>
void print_dynamic(Stream& out, const typename DynamicBT<Args...>::RawUntilFail& uf, string indent) {
    out << indent << "until_fail(\n";
    print_dynamic(out, uf.child, indent + "    ");
    out << indent << "),\n";
}

template <typename Stream, typename... Args>
void print_dynamic(Stream& out, const typename DynamicBT<Args...>::Func& tree, string indent) {
    if (auto branch = tree.template target<typename DynamicBT<Args...>::RawSequence>()) {
        return print_dynamic(out, *branch, indent);
    } else if (auto branch = tree.template target<typename DynamicBT<Args...>::RawSelector>()) {
        return print_dynamic(out, *branch, indent);
    } else if (auto branch = tree.template target<typename DynamicBT<Args...>::RawInverter>()) {
        return print_dynamic(out, *branch, indent);
    } else if (auto branch = tree.template target<typename DynamicBT<Args...>::RawUntilFail>()) {
        return print_dynamic(out, *branch, indent);
    } else {
        out << indent << "LEAF,\n";
    }
}

} // namespace _detail_bait_print_dynamic

using _detail_bait_print_dynamic::print_dynamic;

} // namespace bait

#endif //BEHAVIORTREEPROJ_BAIT_PRINT_DYNAMIC_HPP
