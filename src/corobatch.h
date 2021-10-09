#pragma once

#include <coroutine>
#include <optional>
#include <memory>
#include <deque>
#include <functional>
#include <cassert>

namespace corobatch {

struct task {
    struct promise_type {
        task get_return_object() {
            return task(std::coroutine_handle<promise_type>::from_promise(*this));
        }
        std::suspend_always initial_suspend() { return {}; }
        void unhandled_exception() noexcept { std::terminate(); }
        std::suspend_never final_suspend() noexcept { return {}; }
        void return_void() {
            (*counter_)--;
        }

        static inline std::size_t placeholder = 0;
        std::size_t* counter_ = &placeholder;
    };
    using Handle = std::coroutine_handle<promise_type>;

    explicit task(Handle handle) : handle_(handle) {}
    task(const task&) = delete;
    task(task&& other) noexcept : handle_(std::exchange(other.handle_, nullptr)) {}
    ~task() {
        if (handle_) {
            handle_.destroy();
        }
    }

    Handle release() &&
    {
        return std::exchange(handle_, nullptr);
    }

private:
    Handle handle_;
};

struct Executor {
    void submit(std::vector<std::coroutine_handle<>> v) {
        pending_.insert(pending_.end(), v.begin(), v.end());
    }

    void submit(task t) {
        non_completed_++;
        auto handle = std::move(t).release();
        handle.promise().counter_ = &non_completed_;
        pending_.push_back(handle);
    }

    std::optional<std::coroutine_handle<>> pop_next_coro() {
        // std::cout << "\nPop next. Current: " << pending_.size() << " Non completed: " << non_completed_;
        if(pending_.empty()) {
            return std::nullopt;
        }
        auto first = pending_.front();
        pending_.pop_front();
        return first;
    }

    //  True if not all submitted tasks on the executor have completed
    bool run_available() {
        for(auto next_coro = pop_next_coro(); next_coro; next_coro = pop_next_coro()) {
            next_coro->resume();
        }
        return non_completed_ != 0;
    }

    std::deque<std::coroutine_handle<>> pending_;
    std::size_t non_completed_ = 0;
};


template<typename T, typename R>
struct Batcher {
    struct Batch {
        std::vector<T> args_;
        std::vector<R> returns_;
        std::vector<std::coroutine_handle<>> pending_;
    };

    struct Awaitable
    {
        bool await_ready() {
            return storage_.maybe_execute();
        }

        R await_resume()
        {
            return batch_->returns_.at(index_);
        }

        std::coroutine_handle<> await_suspend(std::coroutine_handle<> h)
        {
            batch_->pending_.push_back(h);
            return storage_.executor_.pop_next_coro().value_or(std::noop_coroutine());
        }

        Batcher& storage_;
        std::shared_ptr<Batch> batch_;
        std::size_t index_ = -1;
    };

    Awaitable operator()(T arg) {
        std::size_t index = current_batch_->args_.size();
        current_batch_->args_.push_back(arg);
        return Awaitable{*this, current_batch_, index};
    }

    // Return true if the execution resulted in some tasks being unblocked
    bool maybe_execute(bool force = false) {
        bool should_execute = should_execute_op_ && should_execute_op_(current_batch_->args_);
        if( force or should_execute ) {
            if(current_batch_->args_.empty()) {
                return false;
            }
            current_batch_->returns_ = op_(std::move(current_batch_->args_));
            executor_.submit(std::move(current_batch_->pending_));
            current_batch_->pending_.clear();
            current_batch_ = std::make_shared<Batch>();
            return true;
        }
        return false;
    }

    Executor& executor_;
    std::function<std::vector<R>(std::vector<T>)> op_;
    std::function<bool(const std::vector<T>&)> should_execute_op_;
    std::shared_ptr<Batch> current_batch_ = std::make_shared<Batch>();
};

template<typename... Ts, typename... Rs>
void run_to_completion(Executor& e, Batcher<Ts, Rs>&... batchers) {
    while(e.run_available()) {
        [[maybe_unused]] bool something_executed =
            (batchers.maybe_execute(true) || ...);
        assert(something_executed);
    }
}

}
