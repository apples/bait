#include <functional>
#include <tuple>
#include <utility>
#include <type_traits>

using namespace std;

struct entity {
};

template<typename T>
struct EBCO {
    T value;
    constexpr EBCO() = default;
    constexpr EBCO(T t) : value(move(t)) { }
};

namespace BTStatic {

enum class status {
    SUCCESS,
    FAILURE,
    RUNNING
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

template<status Mode, typename... Ts>
struct sequence_t : EBCO<tuple<Ts...>> {
    size_t current = 0;

    constexpr sequence_t(tuple<Ts...> children) : EBCO<tuple<Ts...>>{move(children)} { }

    template<typename... Args>
    status operator()(Args&& ... args) {
        for (; current != sizeof...(Ts); ++current) {
            status result = apply(current, EBCO<tuple<Ts...>>::value,
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
constexpr auto sequence(Ts... ts) {
    return sequence_t<status::SUCCESS, Ts...>(make_tuple(move(ts)...));
}

template<typename... Ts>
constexpr auto sequence(tuple<Ts...> tup) {
    return sequence_t<status::SUCCESS, Ts...>(move(tup));
}

template<typename... Ts>
constexpr auto selector(Ts... ts) {
    return sequence_t<status::FAILURE, Ts...>(make_tuple(move(ts)...));
}

template<typename... Ts>
constexpr auto selector(tuple<Ts...> tup) {
    return sequence_t<status::FAILURE, Ts...>(move(tup));
}

template<typename T>
constexpr auto inverter(T t) {
    return inverter_t<T>(t);
}

template<typename T>
constexpr auto inverter(inverter_t<T> t) {
    return t.EBCO<T>::value;
}

template<typename T>
constexpr auto until_fail(T t) {
    return until_fail_t<T>(t);
}

static constexpr auto succeed = sequence();
static constexpr auto fail = selector();

template<typename Head, typename... Tail, size_t... Is>
tuple<Tail...> _tuple_tail_impl(tuple<Head, Tail...>&& tup, integer_sequence<size_t, Is...>) {
    return make_tuple(move(get<Is + 1>(tup))...);
}

template<typename Head, typename... Tail>
tuple<Tail...> tuple_tail(tuple<Head, Tail...>&& tup) {
    return _tuple_tail_impl(move(tup), make_integer_sequence<size_t, sizeof...(Tail)>());
}

template<typename Head, typename... Tail>
Head tuple_head(tuple<Head, Tail...>&& tup) {
    return get<0>(move(tup));
}

template<typename T>
auto simplify(T t) {
    return t;
}

template<status Mode, typename... Ts, typename... Us>
auto sequence_cat(sequence_t<Mode, Ts...> s1, sequence_t<Mode, Us...> s2) {
    return sequence_t<Mode, Ts..., Us...>(tuple_cat(s1.value, s2.value));
}

template<status Mode, typename... Ts, typename U>
auto sequence_cat(sequence_t<Mode, Ts...> s1, U s2) {
    return sequence_t<Mode, Ts..., U>(tuple_cat(s1.value, make_tuple(s2)));
}

template<status Mode, typename T, typename... Us>
auto sequence_cat(T s1, sequence_t<Mode, Us...> s2) {
    return sequence_t<Mode, T, Us...>(tuple_cat(make_tuple(s1), s2.value));
}

template<status Mode, typename T, typename U>
auto sequence_cat(T s1, U s2) {
    return sequence_t<Mode, T, U>(make_tuple(move(s1), move(s2)));
}

template<status Mode, typename Head, typename... Tail>
auto simplify(sequence_t<Mode, Head, Tail...> seq) {
    auto simp_head = simplify(tuple_head(move(seq.value)));
    auto simp_tail = simplify(sequence_t<Mode, Tail...>(tuple_tail(move(seq.value))));
    return sequence_cat<Mode>(simp_head, simp_tail);
}

template<status Mode, typename... Tail>
auto simplify(sequence_t<Mode, sequence_t<Mode>, Tail...> seq) {
    return simplify(sequence_t<Mode, Tail...>(tuple_tail(move(seq.value))));
}

template<status Mode, typename T>
auto simplify(sequence_t<Mode, T> seq) {
    return simplify(tuple_head(move(seq.value)));
}

}

template<typename T, T Ptr>
struct static_function;

template<typename R, typename... Args, R(* func)(Args...)>
struct static_function<R(*)(Args...), func> {
    constexpr R operator()(Args... args) const {
        return func(forward<Args>(args)...);
    }
};


BTStatic::status find_player();
BTStatic::status player_in_range();
BTStatic::status player_out_range();
BTStatic::status attack();
BTStatic::status walk_toward_player();
BTStatic::status walk_randomly();
BTStatic::status stand();
using namespace BTStatic;
auto behavior =
        sequence(
                sequence(
                        until_fail(
                                static_function<decltype(&find_player), find_player>()
                        ),
                        selector(
                                sequence(
                                        static_function<decltype(&player_in_range), player_in_range>(),
                                        inverter(static_function<decltype(&attack), attack>())
                                ),
                                static_function<decltype(&walk_toward_player), walk_toward_player>(),
                                selector(
                                        selector(
                                                static_function<decltype(&player_in_range), player_out_range>()
                                        )
                                )
                        )
                ),
                until_fail(
                        selector(
                                inverter(
                                        sequence(
                                                static_function<decltype(&walk_randomly), walk_randomly>()
                                        )
                                ),
                                static_function<decltype(&stand), stand>()
                        )
                )
        );
auto behavior2 = simplify(behavior);

#if 1
BTStatic::status find_player() { return BTStatic::status::SUCCESS; }
BTStatic::status player_in_range() { return BTStatic::status::SUCCESS; }
BTStatic::status player_out_range() { return BTStatic::status::SUCCESS; }
BTStatic::status attack() { return BTStatic::status::SUCCESS; }
BTStatic::status walk_toward_player() { return BTStatic::status::SUCCESS; }
BTStatic::status walk_randomly() { return BTStatic::status::SUCCESS; }
BTStatic::status stand() { return BTStatic::status::SUCCESS; }

#include <iostream>
#include <vector>
#include <random>
#include <iterator>


template<typename T>
void print(const T&, string indent) {
    cout << indent << "LEAF" << endl;
}

template<typename T>
void print(const inverter_t<T>& iv, string indent) {
    cout << indent << "inverter(" << endl;
    print(iv.value, indent + "    ");
    cout << indent << ")," << endl;
}

template<typename T>
void print(const until_fail_t<T>& iv, string indent) {
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
void print(const sequence_t<status::SUCCESS, Ts...>& iv, string indent) {
    cout << indent << "sequence(" << endl;
    print(iv.value, indent + "    ", make_integer_sequence<size_t, sizeof...(Ts)>());
    cout << indent << ")," << endl;
}

template<typename... Ts>
void print(const sequence_t<status::FAILURE, Ts...>& iv, string indent) {
    cout << indent << "selector(" << endl;
    print(iv.value, indent + "    ", make_integer_sequence<size_t, sizeof...(Ts)>());
    cout << indent << ")," << endl;
}


int main() {
    using namespace BTStatic;

    print(behavior, "");
    print(simplify(behavior), "");

    using strings = string[3];
    cout << (strings{"SUCCESS", "FAILURE", "RUNNING"}[(int) simplify(behavior)()]) << endl;

    cout << sizeof(behavior) << endl;
    cout << sizeof(behavior2) << endl;
}
#else

// behavior()  = 226 asm instructions
// behavior2() = 170 asm instructions

// sizeof(behavior)  = 14 words
// sizeof(behavior2) =  8 words

BTStatic::status func() {
    return behavior();
}
BTStatic::status func2() {
    return behavior2();
}
#endif
