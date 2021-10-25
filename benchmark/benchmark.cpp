#include <benchmark/benchmark.h>

#include <common.h>
#include <comparison.h>
#include <chrono>

using namespace std::literals::chrono_literals;

class TimedDelay : public benchmark::Fixture {
public:
  void SetUp(const ::benchmark::State& state) {
    setPerCallLatency(0us);
    setPerItemLatency(0us);
  }
};

BENCHMARK_F(TimedDelay, no_batching)(benchmark::State& s)
{
    auto users = getUsers();
    for ([[maybe_unused]] auto _ : s)
    {
        sendEmails_NoBatching(users);
    }
}

BENCHMARK_F(TimedDelay, hand_batching)(benchmark::State& s)
{
    auto users = getUsers();
    for ([[maybe_unused]] auto _ : s)
    {
        sendEmails_HandBatching(users);
    }
}

BENCHMARK_F(TimedDelay, range_batching)(benchmark::State& s)
{
    auto users = getUsers();
    for ([[maybe_unused]] auto _ : s)
    {
        sendEmails_RangeBatching(users);
    }
}

BENCHMARK_F(TimedDelay, coro_batching)(benchmark::State& s)
{
    auto users = getUsers();
    for ([[maybe_unused]] auto _ : s)
    {
        sendEmails_CoroBatching(users);
    }
}

BENCHMARK_F(TimedDelay, coro_batching2)(benchmark::State& s)
{
    auto users = getUsers();
    for ([[maybe_unused]] auto _ : s)
    {
        sendEmails_CoroBatching2(users);
    }
}

BENCHMARK_F(TimedDelay, coro_batching2_customalloc)(benchmark::State& s)
{
    auto users = getUsers();
    for ([[maybe_unused]] auto _ : s)
    {
        sendEmails_CoroBatching2CustomAlloc(users);
    }
}

// TODO pass arguments to the test for num users, dealay in the calls
BENCHMARK_MAIN();
