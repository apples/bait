#ifndef BEHAVIORTREEPROJ_BAIT_STATIC_HPP
#define BEHAVIORTREEPROJ_BAIT_STATIC_HPP

#include "bait_common.hpp"

#include <tuple>
#include <utility>
#include <type_traits>

namespace bait {

namespace _detail_bait_static {

using namespace std;

template<typename T>
struct EBCO {
    T value;
    constexpr EBCO() = default;
    constexpr EBCO(T t) : value(move(t)) { }
};

template<size_t C = 0, typename... Ts, typename Func, enable_if_t<C < sizeof...(Ts) - 1, void>...>
status apply(size_t i, tuple<Ts...>& tup, Func&& func) {
    if (i == C) {
        return func(get<C>(tup));
    } else {
        return apply<C + 1>(i, tup, forward<Func>(func));
    }
}

template<size_t C = 0, typename... Ts, typename Func, enable_if_t<C == sizeof...(Ts) - 1, void>...>
status apply(size_t, tuple<Ts...>& tup, Func&& func) {
    return func(get<sizeof...(Ts) - 1>(tup));
}

struct StaticBT {
    template<status Mode, typename... Ts>
    struct sequence_t : EBCO<tuple<Ts...>> {
        size_t current = 0;

        constexpr sequence_t(tuple<Ts...> children) : EBCO<tuple<Ts...>>
                                                              {move(children)} { }

        template<typename... Args>
        status operator()(Args&& ... args) {
            for (; current != sizeof...(Ts); ++current) {
                status result = apply(current, EBCO<tuple < Ts...>>
                ::value,
                        [&](auto&& child) { return child(forward<Args>(args)...); });
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

    template<status Mode>
    struct sequence_t<Mode> : EBCO<tuple<>> {
        constexpr sequence_t(tuple<> t) : EBCO<tuple<>>(t) { };
        template<typename... Args>
        constexpr status operator()(Args&& ... args) {
            return Mode;
        }
    };

    template<typename T>
    struct inverter_t : EBCO<T> {
        constexpr inverter_t() = default;
        constexpr inverter_t(T t) : EBCO<T>(move(t)) { }
        template<typename... Args>
        status operator()(Args&& ... args) {
            status result = EBCO<T>::value(forward<Args>(args)...);
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

    template<typename T>
    struct until_fail_t : EBCO<T> {
        constexpr until_fail_t(T t) : EBCO<T>(move(t)) { }
        template<typename... Args>
        status operator()(Args&& ... args) {
            status result = EBCO<T>::value(forward<Args>(args)...);
            if (result == status::FAILURE) {
                return status::SUCCESS;
            } else {
                return status::RUNNING;
            }
        }
    };

    template<typename... Ts>
    static constexpr auto sequence(Ts... ts) {
        return sequence_t<status::SUCCESS, Ts...>(make_tuple(move(ts)...));
    }

    template<typename... Ts>
    static constexpr auto sequence(tuple<Ts...> tup) {
        return sequence_t<status::SUCCESS, Ts...>(move(tup));
    }

    template<typename... Ts>
    static constexpr auto selector(Ts... ts) {
        return sequence_t<status::FAILURE, Ts...>(make_tuple(move(ts)...));
    }

    template<typename... Ts>
    static constexpr auto selector(tuple<Ts...> tup) {
        return sequence_t<status::FAILURE, Ts...>(move(tup));
    }

    template<typename T>
    static constexpr auto inverter(T t) {
        return inverter_t<T>(t);
    }

    template<typename T>
    static constexpr auto inverter(inverter_t<T> t) {
        return t.EBCO<T>::value;
    }

    template<typename T>
    static constexpr auto until_fail(T t) {
        return until_fail_t<T>(t);
    }

    static constexpr auto succeed() { return sequence(); }
    static constexpr auto fail() { return selector(); }

    template<typename Head, typename... Tail, size_t... Is>
    static tuple<Tail...> _tuple_tail_impl(tuple<Head, Tail...>&& tup, integer_sequence<size_t, Is...>) {
        return make_tuple(move(get<Is + 1>(tup))...);
    }

    template<typename Head, typename... Tail>
    static tuple<Tail...> tuple_tail(tuple<Head, Tail...>&& tup) {
        return _tuple_tail_impl(move(tup), make_integer_sequence<size_t, sizeof...(Tail)>());
    }

    template<typename Head, typename... Tail>
    static Head tuple_head(tuple<Head, Tail...>&& tup) {
        return get<0>(move(tup));
    }

    template<status Mode, typename... Ts, typename... Us>
    static auto sequence_cat(sequence_t<Mode, Ts...> s1, sequence_t<Mode, Us...> s2) {
        return sequence_t < Mode, Ts..., Us...>(tuple_cat(s1.value, s2.value));
    }

    template<status Mode, typename... Ts, typename U>
    static auto sequence_cat(sequence_t<Mode, Ts...> s1, U s2) {
        return sequence_t < Mode, Ts..., U > (tuple_cat(s1.value, make_tuple(s2)));
    }

    template<status Mode, typename T, typename... Us>
    static auto sequence_cat(T s1, sequence_t<Mode, Us...> s2) {
        return sequence_t < Mode, T, Us...>(tuple_cat(make_tuple(s1), s2.value));
    }

    template<status Mode, typename T, typename U>
    static auto sequence_cat(T s1, U s2) {
        return sequence_t<Mode, T, U>(make_tuple(move(s1), move(s2)));
    }

    template<typename T>
    static auto simplify(T t) {
        return t;
    }

    template<status Mode, typename Head, typename... Tail>
    static auto simplify(sequence_t<Mode, Head, Tail...> seq) {
        auto simp_head = simplify(tuple_head(move(seq.value)));
        auto simp_tail = simplify(sequence_t<Mode, Tail...>(tuple_tail(move(seq.value))));
        return sequence_cat<Mode>(simp_head, simp_tail);
    }

    template<status Mode, typename... Tail>
    static auto simplify(sequence_t<Mode, sequence_t<Mode>, Tail...> seq) {
        return simplify(sequence_t<Mode, Tail...>(tuple_tail(move(seq.value))));
    }

    template<status Mode, typename T>
    static auto simplify(sequence_t<Mode, T> seq) {
        return simplify(tuple_head(move(seq.value)));
    }
};

template<typename T>
void print(const T&, string indent) {
    cout << indent << "LEAF" << endl;
}

template<typename T>
void print(const StaticBT::inverter_t<T>& iv, string indent) {
    cout << indent << "inverter(" << endl;
    print(iv.value, indent + "    ");
    cout << indent << ")," << endl;
}

template<typename T>
void print(const StaticBT::until_fail_t<T>& iv, string indent) {
    cout << indent << "until_fail(" << endl;
    print(iv.value, indent + "    ");
    cout << indent << ")," << endl;
}

template<typename... Ts, size_t... Is>
void print(const tuple<Ts...>& tup, string indent, integer_sequence<size_t, Is...>) {
    using intarr = int[sizeof...(Ts)];
    intarr{(print(get<Is>(tup), indent), 1)...};
}

template<typename... Ts>
void print(const StaticBT::sequence_t<status::SUCCESS, Ts...>& iv, string indent) {
    cout << indent << "sequence(" << endl;
    print(iv.value, indent + "    ", make_integer_sequence<size_t, sizeof...(Ts)>());
    cout << indent << ")," << endl;
}

template<typename... Ts>
void print(const StaticBT::sequence_t<status::FAILURE, Ts...>& iv, string indent) {
    cout << indent << "selector(" << endl;
    print(iv.value, indent + "    ", make_integer_sequence<size_t, sizeof...(Ts)>());
    cout << indent << ")," << endl;
}

} // namespace _detail_bait_static

using _detail_bait_static::StaticBT;
using _detail_bait_static::print;

} // namespace bait

#endif //BEHAVIORTREEPROJ_BAIT_STATIC_HPP
