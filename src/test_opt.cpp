#include <vector>
#include <optional>
#include <iostream>

#include <corobatch_opt.h>

void float_2_int(const std::vector<float>& args, std::vector<std::optional<int>*>& ret){
    for(std::size_t i = 0; i < args.size(); i++) {
        ret[i]->emplace(args[i]);
    }
    std::cout << "\nRun f2i with " << args.size();

}

void int_2_float(const std::vector<int>& args, std::vector<std::optional<float>*>& ret){
    for(std::size_t i = 0; i < args.size(); i++) {
        ret[i]->emplace(args[i] + 0.5f);
    }
    std::cout << "\nRun i2f with " << args.size();
}

struct GT {
    template<typename T>
    bool operator()(const T& v) const {
        return v.size() >= num_;
    }

    std::size_t num_;
};

int main() {
    using namespace cb2;
    Executor e;
    Batcher<float, int, decltype((float_2_int)), GT> f2i{e, float_2_int, GT{7}};
    Batcher<int, float, decltype((int_2_float)), GT> i2f{e, int_2_float, GT{5}};
    auto task_generator = [&](int i) -> task {
        std::cout << "\nWith " << i;
        int new_i = co_await f2i(0.4f + i);
        std::cout << "\nGot " << new_i;
        float new_f = co_await i2f(new_i);
        std::cout << "\nGot " << new_f;
        co_return;
    };
    for(int i = 0; i < 12; i++) {
        e.submit(task_generator(i));
    }
    while(e.run_available()) {
        std::cout << "\nForce f2i";
        if(f2i.maybe_execute(true)) { continue; }
        std::cout << "\nForce i2f";
        if(i2f.maybe_execute(true)) { continue; }
        std::cout << "This should never be called";
    }
}
