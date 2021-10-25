#include <common.h>
#include <thread>
#include <emmintrin.h>

namespace {

const int kNumberOfUsers = 1000;

std::chrono::nanoseconds kPerCallLatency{0};
std::chrono::nanoseconds kPerItemLatency{0};

}


void setPerCallLatency(std::chrono::nanoseconds val) {
    kPerCallLatency = val;
}
void setPerItemLatency(std::chrono::nanoseconds val) {
    kPerItemLatency = val;
}

std::vector<User> getUsers() {
    std::vector<User> result;
    result.reserve(kNumberOfUsers);
    for(int i = 0; i < kNumberOfUsers; i++) {
        result.push_back({.id = i});
    }
    return result;
}

void wait_for(std::size_t num_items) {
    // For give me for the bestiality
    // we want accurate sleep, so we busy-loop. Calling sleep might yield the thread
    // and the switch might be too granular
    auto wait = kPerCallLatency + (kPerItemLatency * num_items);
    if(wait.count() == 0) {
        return;
    }
    auto target = std::chrono::steady_clock::now() + wait;
    while(std::chrono::steady_clock::now() < target) {
        _mm_pause();
    }
}

std::vector<UserPreferences> getUserPreferences(const std::vector<UserId>& userIds){
    std::vector<UserPreferences> preferences;
    preferences.reserve(userIds.size());
    for(const UserId& id : userIds) {
        preferences.push_back({
            .wantsEmailNotification = (id % 2 == 0),
            .notificationEmailAddress = "constant@email.address"});
    }

    wait_for(userIds.size());
    return preferences;
}

std::vector<bool> sendNotifications(const std::vector<EmailAddress>& emailAddresses) {
    wait_for(emailAddresses.size());
    return std::vector<bool>(emailAddresses.size(), true);
}

void getUserPreferences2(const std::vector<UserId>& userIds, std::vector<std::optional<UserPreferences>*>& ret) {
    for(std::size_t i = 0; i < userIds.size(); i++) {
        const UserId& id = userIds[i];
        ret[i]->emplace(UserPreferences{
            .wantsEmailNotification = (id % 2 == 0),
            .notificationEmailAddress = "constant@email.address"});
    }
    wait_for(userIds.size());
}

void sendNotifications2(const std::vector<EmailAddress>& emailAddresses, std::vector<std::optional<bool>*>& ret) {
    for(auto& opt : ret) {
        opt->emplace(true);
    }
    wait_for(emailAddresses.size());
}
