#ifndef BEHAVIORTREEPROJ_BAIT_STATIC_HPP
#define BEHAVIORTREEPROJ_BAIT_STATIC_HPP

#include <chaiscript/chaiscript.hpp>

#include <algorithm>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

namespace bait {

namespace _detail_bait_chai_module {

using namespace std;
using chaiscript::Boxed_Value;
using chaiscript::Module;
using chaiscript::ModulePtr;
using chaiscript::boxed_cast;
using chaiscript::fun;
using chaiscript::type_conversion;

template <typename BT>
struct bootstrap_helper;

template<typename... Args>
struct bootstrap_helper<DynamicBT<Args...>> {
    static void bootstrap(const ModulePtr& m) {
        using BT = DynamicBT<Args...>;

        auto to_vector_btfunc = [](const vector<Boxed_Value>& bvs) {
            vector <typename BT::Func> btfuncs;
            auto cast = [](const Boxed_Value& bv){return boxed_cast<typename BT::Func>(bv);};
            btfuncs.reserve(bvs.size());
            transform(bvs.begin(), bvs.end(), back_inserter(btfuncs), cast);
            return btfuncs;
        };

        m->add(type_conversion<vector<Boxed_Value>, vector<typename BT::Func>>(to_vector_btfunc));

        m->add(fun<typename BT::Func(vector<typename BT::Func>)>(BT::sequence), "sequence");
        m->add(fun<typename BT::Func(vector<typename BT::Func>)>(BT::selector), "selector");
        m->add(fun<typename BT::Func(typename BT::Func)>(BT::inverter), "inverter");
        m->add(fun<typename BT::Func(typename BT::Func)>(BT::until_fail), "until_fail");
    }
};

template<typename BT>
ModulePtr bootstrap(ModulePtr m = make_shared<Module>()) {
    bootstrap_helper<BT>::bootstrap(m);
    return m;
}

} // namespace _detail_bait_chai_module

using _detail_bait_chai_module::bootstrap;

} // namespace bait

#endif //BEHAVIORTREEPROJ_BAIT_STATIC_HPP
