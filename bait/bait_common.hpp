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

} // namespace bait

#endif //BEHAVIORTREEPROJ_BAIT_COMMON_HPP
