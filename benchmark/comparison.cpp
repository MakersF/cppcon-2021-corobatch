#include <comparison.h>

// Needed for the hand batching
#include <algorithm>
#include <iterator>
// Needed for range batching
#include <ranges>
// Needed for the coro batching
#include <corobatch.h>
// Needed for the coro batching
#include <corobatch_opt.h>

void sendEmails_NoBatching(const std::vector<User>& users) {
    for(const User& user : users) {
        const UserPreferences preferences = getUserPreferences({user.id}).at(0);
        if (preferences.wantsEmailNotification) {
            sendNotifications({preferences.notificationEmailAddress});
        }
    }
}

void sendEmails_HandBatching(const std::vector<User>& users) {
    std::vector<UserId> userIds;
    userIds.reserve(users.size());
    transform(users.begin(), users.end(), std::back_inserter(userIds),
        [](const User& user) { return user.id; });
    std::vector<UserPreferences> preferences = getUserPreferences(userIds);
    std::remove_if(preferences.begin(), preferences.end(),
        [](const UserPreferences& pref) { return !pref.wantsEmailNotification;});
    std::vector<EmailAddress> emails;
    emails.reserve(preferences.size());
    std::transform(preferences.begin(), preferences.end(), std::back_inserter(emails),
        [](const UserPreferences& pref) {return pref.notificationEmailAddress; });
    sendNotifications(emails);
}

void sendEmails_RangeBatching(const std::vector<User>& users) {
    using namespace std::ranges::views;
    auto ids = users
        | transform([](const User& user) { return user.id; });
    std::vector<UserId> userIds(ids.begin(), ids.end());
    std::vector<UserPreferences> preferences = getUserPreferences(userIds);
    auto emails = preferences
        | filter([](const UserPreferences& pref) { return pref.wantsEmailNotification; })
        | transform([](const UserPreferences& pref) { return pref.notificationEmailAddress; });
    std::vector<EmailAddress> emailAddresses(emails.begin(), emails.end());
    sendNotifications(emailAddresses);
}

void sendEmails_CoroBatching(const std::vector<User>& users) {
    using namespace corobatch;
    Executor e;
    auto no_max_batch_size = [](const auto&) { return false; };
    Batcher<UserId, UserPreferences> getPrefs{e, getUserPreferences, no_max_batch_size};
    Batcher<EmailAddress, bool> sendNotifs{e, sendNotifications, no_max_batch_size};

    auto notify_user = [&](const User& user) -> task {
        const UserPreferences preferences = co_await getPrefs(user.id);
        if (preferences.wantsEmailNotification) {
            co_await sendNotifs(preferences.notificationEmailAddress);
        }
    };

    for(const User& user : users) {
        e.submit(notify_user(user));
    }
    run_to_completion(e, getPrefs, sendNotifs);
}

template<bool custom_alloc>
void sendEmails_CoroBatching2Impl(const std::vector<User>& users) {
    using namespace cb2;
    Executor e;
    auto no_max_batch_size = [](const auto&) { return false; };
    using T = decltype(no_max_batch_size);
    Batcher<UserId, UserPreferences, decltype((getUserPreferences2)), T> getPrefs{e, getUserPreferences2, no_max_batch_size};
    Batcher<EmailAddress, bool, decltype((sendNotifications2)), T> sendNotifs{e, sendNotifications2, no_max_batch_size};

    auto notify_user = [&](const User& user) -> task_impl<custom_alloc> {
        const UserPreferences preferences = co_await getPrefs(user.id);
        if (preferences.wantsEmailNotification) {
            co_await sendNotifs(preferences.notificationEmailAddress);
        }
    };

    for(const User& user : users) {
        e.submit(notify_user(user));
    }
    run_to_completion(e, getPrefs, sendNotifs);
}

void sendEmails_CoroBatching2(const std::vector<User>& users) {
    sendEmails_CoroBatching2Impl<false>(users);
}

void sendEmails_CoroBatching2CustomAlloc(const std::vector<User>& users) {
    sendEmails_CoroBatching2Impl<true>(users);
}
