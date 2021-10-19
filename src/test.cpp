#include <vector>
#include <iostream>

#include <corobatch.h>

std::vector<int> float_2_int(std::vector<float> args){
    std::vector<int> ret;
    for(auto arg : args) {
        ret.push_back(arg);
    }
    std::cout << "\nRun f2i with " << args.size();
    return ret;
}

std::vector<float> int_2_float(std::vector<int> args){
    std::vector<float> ret;
    for(auto arg : args) {
        ret.push_back(arg + 0.5f);
    }
    std::cout << "\nRun i2f with " << args.size();
    return ret;
}

int main() {
    using namespace corobatch;
    Executor e;
    Batcher<float, int> f2i{e, float_2_int,[](const std::vector<float>& args) {
        return args.size() >= 7;
    }};
    Batcher<int, float> i2f{e, int_2_float,[](const std::vector<int>& args) {
        return args.size() >= 5;
    }};
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
