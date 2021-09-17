#include <experimental/coroutine>
#include <optional>
#include <memory>
#include <vector>
#include <deque>
#include <functional>

struct task {

    struct promise_type {
        task get_return_object() {
            return task(std::experimental::coroutine_handle<promise_type>::from_promise(*this));
        }
        std::experimental::suspend_always initial_suspend() { return {}; }
        void unhandled_exception() noexcept { std::terminate(); }
        std::experimental::suspend_never final_suspend() noexcept { return {}; }
        void return_void() {
            if(cb_) {
                cb_();
            }
        }

        std::function<void()> cb_;
    };

    explicit task(std::experimental::coroutine_handle<promise_type> handle) : handle_(handle) {}
    task(const task&) = delete;
    task(task&& other) noexcept : handle_(other.handle_) { other.handle_ = nullptr; }
     ~task() {
         if (handle_) {
             handle_.destroy();
         }
     }

    std::experimental::coroutine_handle<promise_type> handle_;
};

struct Executor {
    void schedule(std::vector<std::experimental::coroutine_handle<>> v) {
        pending_.insert(pending_.end(), v.begin(), v.end());
    }

    void schedule(const std::vector<task>& ts) {
        for(const task& t : ts) {
            pending_.push_back(t.handle_);
            non_completed_++;
            t.handle_.promise().cb_ = [this]() {
                non_completed_--;
            };
        }
    }
    std::optional<std::experimental::coroutine_handle<>> pop_next_coro() {
        if(pending_.empty()) {
            return {};
        }
        auto first = pending_.front();
        pending_.pop_front();
        return first;
    }

    //  True if not all scheduled tasks on the executor have completed
    bool run_available() {
        for(auto next_coro = pop_next_coro(); next_coro; next_coro = pop_next_coro()) {
            next_coro->resume();
        }
        return non_completed_ != 0;
    }

    std::deque<std::experimental::coroutine_handle<>> pending_;
    int non_completed_;
};


template<typename T, typename R>
struct Storage {
    struct Batch {
        std::vector<T> args_;
        std::vector<R> returns_;
        std::vector<std::experimental::coroutine_handle<>> pending_;
        bool has_executed = false;
    };

    struct Awaitable
    {
        bool await_ready() {
            index_ = batch_->args_.size();
            batch_->args_.push_back(arg_);
            return storage_.maybe_execute();
        }

        R await_resume()
        {
            return batch_->returns_.at(index_);
        }

        std::experimental::coroutine_handle<> await_suspend(std::experimental::coroutine_handle<> h)
        {
            batch_->pending_.push_back(h);
            return storage_.executor_.pop_next_coro().value_or(std::experimental::noop_coroutine());
        }

        Storage& storage_;
        T arg_;
        std::shared_ptr<Batch> batch_;
        std::size_t index_ = -1;
    };

    Awaitable operator()(T arg) {
        return Awaitable{*this, arg, current_batch_};
    }

    bool maybe_execute(bool force = false) {
        bool should_execute = false;
        if( not force and not should_execute ) {
            return false;
        }
        current_batch_->returns_ = op_(args_);
        current_batch_->has_executed = true;
        current_batch_->args_.clear();
        executor_.schedule(std::move(current_batch_->pending_));
        current_batch_->pending_.clear();
        current_batch_ = std::make_shared<Batch>();
        return true;
    }

    Executor& executor_;
    std::function<std::vector<R>(std::vector<T>)> op_;
    std::shared_ptr<Batch> current_batch_ = std::make_shared<Batch>();
};

int main() {
    Executor e;
    Storage<float, int> f2i{e, [](std::vector<float>){ return std::vector<int>{}; }};
    std::vector<task> tasks;
    auto task_generator = [&](int i) -> task {
        int new_i = co_await f2i(0.4f + i);
        co_return;
    };
    for(int i = 0; i < 10; i++) {
        tasks.push_back(task_generator(i));
    }
    e.schedule(tasks);
    while(e.run_available()) {
        f2i.maybe_execute(true);
    }
}
