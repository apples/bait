#ifndef BEHAVIORTREEPROJ_BAIT_COMMON_HPP
#define BEHAVIORTREEPROJ_BAIT_COMMON_HPP

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

template <Optimization...>
struct OptHolder {};

} // namespace bait

#endif //BEHAVIORTREEPROJ_BAIT_COMMON_HPP
