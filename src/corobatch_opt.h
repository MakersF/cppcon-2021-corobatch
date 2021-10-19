#pragma once

#include <coroutine>
#include <optional>
#include <memory>
#include <deque>
#include <functional>
#include <cassert>

namespace cb2 {


/* ========================================================================
   There are 2 main changes that make this version more performant:
    - allocations of coroutines
    - storage of return value

   In this version the promise type is modified to provide a custom
   new and delete operations, which allocate the coroutine frame
   in a preallocated buffer.
   This removes the heap allocation done for each coroutine.

   The second change consists in storing the return value inside the
   Awaitable returned by the Batcher.
   This way we don't have to use a shared_ptr to keep the result
   vector alive, and we can completely remove the usage of the shared_ptr.
   This is done by keeping a vector of pointers to the storage inside
   the awaitables. When the function executes, it will place the return
   values directly in the right location. The memory for the return value
   will most commonly come from the coroutine frame of the coroutine which
   awaited the batch.
   ========================================================================
*/
inline std::size_t useless = 0;

// Implement custom allocation inside a buffer
extern std::array<char, 1'000'000> buffer;
extern std::size_t offset;
extern std::size_t allocations;

inline void* my_allocate(std::size_t size) {
    auto new_offset = offset + size;
    if(new_offset > buffer.size()) throw std::bad_alloc{};
    void* ptr = &buffer[offset];
    offset = new_offset;
    allocations++;
    return ptr;
}

inline void my_deallocate() {
    allocations--;
    if (allocations == 0) {
        offset = 0;
    }
}

template<bool custom_alloc = false>
struct task_impl {
    template<typename self>
    struct promise_type_base {
        task_impl get_return_object() {
            return task_impl(std::coroutine_handle<self>::from_promise(*static_cast<self*>(this)));
        }
        std::suspend_always initial_suspend() { return {}; }
        void unhandled_exception() noexcept { std::terminate(); }
        std::suspend_never final_suspend() noexcept { return {}; }
        void return_void() {
            (*counterPtr_)--;
        }

        std::size_t* counterPtr_ = &useless;

    };

    struct no_alloc_promise_type : promise_type_base<no_alloc_promise_type> {};

    struct custom_alloc_promise_type : promise_type_base<custom_alloc_promise_type> {
        void* operator new(std::size_t size)
        {
            return my_allocate(size);
        }

        void operator delete(void* ptr, std::size_t size)
        {
            my_deallocate();
        }
    };

    using promise_type = std::conditional_t<custom_alloc, custom_alloc_promise_type, no_alloc_promise_type>;
    using Handle = std::coroutine_handle<promise_type>;

    explicit task_impl(Handle handle) : handle_(handle) {}
    task_impl(const task_impl&) = delete;
    task_impl(task_impl&& other) noexcept : handle_(std::exchange(other.handle_, nullptr)) {}
    ~task_impl() {
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

using task = task_impl<false>;

struct Executor {
    void submit(std::vector<std::coroutine_handle<>> v) {
        pending_.insert(pending_.end(), v.begin(), v.end());
    }

    template<typename Task>
    void submit(Task t) {
        non_completed_++;
        auto handle = std::move(t).release();
        handle.promise().counterPtr_ = &non_completed_;
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


template<typename T, typename R, typename F, typename FF>
struct Batcher {

    struct Awaitable
    {
        bool await_ready() {
            return storage_.maybe_execute();
        }

        R await_resume()
        {
            return std::move(return_).value();
        }

        std::coroutine_handle<> await_suspend(std::coroutine_handle<> h)
        {
            storage_.pending_.push_back(h);
            return storage_.executor_.pop_next_coro().value_or(std::noop_coroutine());
        }

        Batcher& storage_;
        std::size_t index_;
        std::optional<R> return_;

        Awaitable(Batcher& b, std::size_t i)
        : storage_(b), index_(i) {
             storage_.returns_[index_] = &return_;
        }

        Awaitable(const Awaitable&) = delete;
        Awaitable(Awaitable&& o)
        : storage_(o.storage_), index_(o.index_), return_(std::move(o).return_)
        {
            storage_.returns_[index_] = &return_;
        }
    };

    Awaitable operator()(T arg) {
        std::size_t index = args_.size();
        args_.push_back(arg);
        returns_.push_back(nullptr);
        return Awaitable{*this, index};
    }

    // Return true if the execution resulted in some tasks being unblocked
    bool maybe_execute(bool force = false) {
        bool should_execute = should_execute_op_(args_);
        if( force or should_execute ) {
            if(args_.empty()) {
                return false;
            }
            op_(std::move(args_), returns_);
            executor_.submit(std::move(pending_));
            args_.clear();
            returns_.clear();
            pending_.clear();
            return true;
        }
        return false;
    }

    Batcher(Executor& e, F op, FF exec_op)
    : executor_(e), op_(op), should_execute_op_(exec_op) {}

    Executor& executor_;
    F op_;
    FF should_execute_op_;
    std::vector<T> args_;
    std::vector<std::optional<R>*> returns_;
    std::vector<std::coroutine_handle<>> pending_;
};

template<typename... Batchers>
void run_to_completion(Executor& e, Batchers&... batchers) {
    while(e.run_available()) {
        [[maybe_unused]] bool something_executed =
            (batchers.maybe_execute(true) || ...);
        assert(something_executed);
    }
}

}
