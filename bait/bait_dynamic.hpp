#ifndef BEHAVIORTREEPROJ_BAIT_DYNAMIC_HPP
#define BEHAVIORTREEPROJ_BAIT_DYNAMIC_HPP

#include "bait_common.hpp"

#include <algorithm>
#include <vector>
#include <functional>
#include <iterator>
#include <utility>

namespace bait {

namespace _detail_bait_dynamic {

using namespace std;

template<typename T>
static auto vector_cat(vector<T>&& s1, vector<T>&& s2) {
    vector<T> funcs;
    funcs.reserve(s1.size() + s2.size());
    move(s1.begin(), s1.end(), back_inserter(funcs));
    move(s2.begin(), s2.end(), back_inserter(funcs));
    return funcs;
}

template<typename... Args>
struct DynamicBT {
    using Func = function<status(Args...)>;

    template<status Mode>
    struct RawSerial {
        vector<Func> children;
        size_t current = 0;

        constexpr RawSerial(vector<Func> children) : children(move(children)) { }

        status operator()(Args&& ... args) {
            auto sz = children.size();
            for (; current != sz; ++current) {
                status result = children[current](forward<Args>(args)...);
                switch (result) {
                    case status::RUNNING:
                        return status::RUNNING;
                    case Mode:
                        break;
                    default:
                        current = 0;
                        return result;
                }
                ++current;
            }
            current = 0;
            return Mode;
        }
    };

    using RawSequence = RawSerial<status::SUCCESS>;
    using RawSelector = RawSerial<status::FAILURE>;

    struct RawInverter {
        Func child;

        constexpr RawInverter() = default;

        constexpr RawInverter(Func child) : child(move(child)) { }

        status operator()(Args&& ... args) {
            status result = child(forward<Args>(args)...);
            switch (result) {
                case status::SUCCESS:
                    return status::FAILURE;
                case status::FAILURE:
                    return status::SUCCESS;
                default:
                    return result;
            }
        }
    };

    struct RawUntilFail {
        Func child;

        constexpr RawUntilFail(Func child) : child(move(child)) { }

        status operator()(Args&& ... args) {
            status result = child(forward<Args>(args)...);
            switch (result) {
                case status::FAILURE:
                    return status::SUCCESS;
                default:
                    return status::RUNNING;
            }
        }
    };

    template<typename... Ts>
    static constexpr Func sequence(Ts&& ... ts) {
        return RawSequence({forward<Ts>(ts)...});
    }

    static constexpr Func sequence(vector<Func> funcs) {
        return RawSequence(move(funcs));
    }

    template<typename... Ts>
    static constexpr Func selector(Ts&& ... ts) {
        return RawSelector({forward<Ts>(ts)...});
    }

    static constexpr Func selector(vector<Func> funcs) {
        return RawSelector(move(funcs));
    }

    static constexpr Func inverter(Func t) {
        return RawInverter(move(t));
    }

    static constexpr Func until_fail(Func t) {
        return RawUntilFail(move(t));
    }

    template<status Mode>
    struct constant_t {
        constexpr status operator()(Args...) const {
            return Mode;
        }
    };

    using succeed = constant_t<status::SUCCESS>;
    using fail = constant_t<status::FAILURE>;
};

} // namespace _detail_bait_dynamic

using _detail_bait_dynamic::DynamicBT;

template<typename... Args, Optimization... Opts>
struct Simplifier<DynamicBT<Args...>, Opts...> {
    using BT = DynamicBT<Args...>;

    template<status Mode>
    typename BT::Func simplify(typename BT::template RawSerial<Mode> seq) const {
        using namespace std;
        vector<typename BT::Func> finalvec = move(seq.children);
        vector<typename BT::Func> tmpvec;

        // Simplify children
        for (auto& f : finalvec) {
            f = simplify(move(f));
        }

        // Flatten series
        if (is_in<Optimization::FLATTEN_SERIES,Opts...>()) {
            tmpvec.reserve(finalvec.size());
            for (auto& f : finalvec) {
                if (auto ptr = f.template target<typename BT::template RawSerial < Mode>>()) {
                    move(ptr->children.begin(), ptr->children.end(), back_inserter(tmpvec));
                } else {
                    tmpvec.push_back(move(f));
                }
            }
            swap(tmpvec, finalvec);
            tmpvec.clear();
        }

        // Remove unreachable children
        if (is_in<Optimization::REMOVE_UNREACHABLE,Opts...>()) {
            auto is_instant_success = [](auto& f) {
                return bool(f.template target<typename BT::template constant_t < Mode>>
                        ());
            };
            auto is_instant_fail = [](auto& f) {
                return bool(f.template target<typename BT::template constant_t < flip(Mode)>>
                        ());
            };
            auto success_iter = find_if(finalvec.begin(), finalvec.end(), is_instant_success);
            if (success_iter != finalvec.end()) {
                finalvec.erase(next(success_iter), finalvec.end());
            }
            auto fail_iter = find_if(finalvec.begin(), finalvec.end(), is_instant_fail);
            finalvec.erase(fail_iter, finalvec.end());
        }

        // Unwrap singular or empty series
        if (is_in<Optimization::UNWRAP_SERIES,Opts...>()) {
            if (finalvec.size() == 0) {
                return typename BT::succeed();
            } else if (finalvec.size() == 1) {
                return move(finalvec.front());
            }
        }

        // Minimize inverters
        if (is_in<Optimization::MINIMIZE_SERIES_INVERSION,Opts...>()) {
            auto is_inverter = [](auto& f) { return bool(f.template target<typename BT::RawInverter>()); };
            auto inverted_children = count_if(finalvec.begin(), finalvec.end(), is_inverter);
            if (inverted_children > finalvec.size() - inverted_children + 1) {
                for (auto& f : finalvec) {
                    if (auto ptr = f.template target<typename BT::RawInverter>()) {
                        auto tmp = move(ptr->child);
                        f = move(tmp);
                    } else {
                        f = BT::inverter(move(f));
                    }
                }
                return BT::inverter(typename BT::template RawSerial<flip(Mode)>(move(finalvec)));
            }
        }

        return typename BT::template RawSerial<Mode>(move(finalvec));
    }

    typename BT::Func simplify(typename BT::RawInverter inv) const {
        using namespace std;
        inv.child = simplify(move(inv.child));
        if (is_in<Optimization::UNWRAP_INVERTERS,Opts...>()) {
            if (auto ptr = inv.child.template target<typename BT::RawInverter>()) {
                return move(ptr->child);
            }
        }
        return inv;
    }

    typename BT::Func simplify(typename BT::RawUntilFail uf) const {
        using namespace std;
        uf.child = simplify(move(uf.child));
        return uf;
    }

    // Dispatcher
    typename BT::Func simplify(typename BT::Func tree) const {
        using namespace std;
        if (auto branch = tree.template target<typename BT::RawSequence>()) {
            return simplify(move(*branch));
        } else if (auto branch = tree.template target<typename BT::RawSelector>()) {
            return simplify(move(*branch));
        } else if (auto branch = tree.template target<typename BT::RawInverter>()) {
            return simplify(move(*branch));
        } else if (auto branch = tree.template target<typename BT::RawUntilFail>()) {
            return simplify(move(*branch));
        } else if (auto branch = tree.template target<typename BT::Func>()) {
            return simplify(move(*branch));
        } else {
            return tree;
        }
    }

    typename BT::Func operator()(typename BT::Func tree) const {
        using namespace std;
        return simplify(move(tree));
    }
};

template<typename... Args>
struct Simplifier<DynamicBT<Args...>, Optimization::NONE> : Simplifier<DynamicBT<Args...>> {
};

template<typename... Args>
struct Simplifier<DynamicBT<Args...>, Optimization::QUICK>
        : Simplifier<DynamicBT<Args...>,
                Optimization::UNWRAP_INVERTERS,
                Optimization::UNWRAP_SERIES> {
};

template<typename... Args>
struct Simplifier<DynamicBT<Args...>, Optimization::ALL>
        : Simplifier<DynamicBT<Args...>,
                Optimization::UNWRAP_INVERTERS,
                Optimization::MINIMIZE_SERIES_INVERSION,
                Optimization::FLATTEN_SERIES,
                Optimization::UNWRAP_SERIES,
                Optimization::REMOVE_UNREACHABLE> {
};

} // namespace bait

#endif //BEHAVIORTREEPROJ_BAIT_DYNAMIC_HPP
