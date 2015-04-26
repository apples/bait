#ifndef BEHAVIORTREEPROJ_BAIT_DYNAMIC_HPP
#define BEHAVIORTREEPROJ_BAIT_DYNAMIC_HPP

#include "bait_common.hpp"

#include <vector>
#include <functional>
#include <iterator>
#include <utility>

namespace bait {

namespace _detail_bait_dynamic {

using namespace std;

template <typename T>
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

    template<status Mode>
    static Func simplify(RawSerial<Mode> seq) {
        if (seq.children.empty()) {
            return succeed();
        }
        vector<Func> simp_head;
        vector<Func> simp_tail;
        seq.children.front() = simplify(move(seq.children.front()));
        if (auto ptr = seq.children.front().template target<RawSerial<Mode>>()) {
            simp_head = move(ptr->children);
        } else if (auto ptr = seq.children.front().template target<constant_t<Mode>>()) {
        } else if (auto ptr = seq.children.front().template target<constant_t<flip(Mode)>>()) {
            return fail();
        } else {
            simp_head.push_back(move(seq.children.front()));
        }
        if (simp_head.size() > 1) {
            for (auto& f : simp_head) {
                f = simplify(move(f));
            }
        }
        seq.children.erase(seq.children.begin());
        auto newseq = simplify(move(seq));
        if (auto ptr = newseq.template target<RawSerial<Mode>>()) {
            simp_tail = move(ptr->children);
        } else {
            simp_tail.push_back(move(newseq));
        }
        auto catvec = vector_cat(move(simp_head), move(simp_tail));
        vector<Func> finalvec;
        finalvec.reserve(catvec.size());
        for (auto& f : catvec) {
            if (auto ptr = f.template target<constant_t<Mode>>()) {
            } else if (auto ptr = f.template target<constant_t<flip(Mode)>>()) {
                if (finalvec.empty()) {
                    return move(f);
                } else {
                    break;
                }
            } else {
                finalvec.push_back(move(f));
            }
        }
        if (finalvec.size() == 0) {
            return succeed();
        } else if (finalvec.size() == 1) {
            return move(finalvec.front());
        }
        return RawSerial<Mode>(move(finalvec));
    }

    static Func simplify(RawInverter inv) {
        if (auto ptr = inv.child.template target<RawInverter>()) {
            return simplify(move(ptr->child));
        } else {
            inv.child = simplify(move(inv.child));
            return inv;
        }
    }

    static Func simplify(RawUntilFail uf) {
        uf.child = simplify(move(uf.child));
        return uf;
    }

    // Dispatcher
    static Func simplify(Func tree) {
        if (auto branch = tree.template target<RawSequence>()) {
            return simplify(move(*branch));
        } else if (auto branch = tree.template target<RawSelector>()) {
            return simplify(move(*branch));
        } else if (auto branch = tree.template target<RawInverter>()) {
            return simplify(move(*branch));
        } else if (auto branch = tree.template target<RawUntilFail>()) {
            return simplify(move(*branch));
        } else {
            return tree;
        }
    }
};

} // namespace _detail_bait_dynamic

using _detail_bait_dynamic::DynamicBT;

} // namespace bait

#endif //BEHAVIORTREEPROJ_BAIT_DYNAMIC_HPP
