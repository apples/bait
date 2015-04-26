#ifndef BEHAVIORTREEPROJ_BAIT_COMMON_HPP
#define BEHAVIORTREEPROJ_BAIT_COMMON_HPP

#include <type_traits>

namespace bait {

enum class status {
    SUCCESS,
    FAILURE,
    RUNNING
};

static constexpr status flip(status s) {
    return (s == status::SUCCESS ? status::FAILURE : (s == status::FAILURE ? status::SUCCESS : s));
}

enum class Optimization {
    NONE,
    UNWRAP_INVERTERS,
    MINIMIZE_SERIES_INVERSION,
    FLATTEN_SERIES,
    UNWRAP_SERIES,
    REMOVE_UNREACHABLE,

    // Meta
            QUICK,
    ALL
};

template<Optimization...>
struct OptHolder {
};

template<Optimization...>
struct is_in_impl;

template<Optimization opt, Optimization Head, Optimization... Tail>
struct is_in_impl<opt, Head, Tail...> : std::conditional_t<opt == Head, std::true_type, is_in_impl<opt, Tail...>> {
};

template<Optimization opt>
struct is_in_impl<opt> : std::false_type {
};

template<Optimization opt, Optimization... Opts>
constexpr bool is_in() {
    return is_in_impl<opt, Opts...>::value;
}

template<typename BT, Optimization... Opts>
struct Simplifier;

} // namespace bait

#endif //BEHAVIORTREEPROJ_BAIT_COMMON_HPP
