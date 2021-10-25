#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <optional>

using UserId = int;
struct User {
    UserId id;
};

using EmailAddress = std::string;
struct UserPreferences {
    bool wantsEmailNotification;
    EmailAddress notificationEmailAddress;
};

void setPerCallLatency(std::chrono::nanoseconds val);
void setPerItemLatency(std::chrono::nanoseconds val);

std::vector<User> getUsers();
std::vector<UserPreferences> getUserPreferences(const std::vector<UserId>& userIds);
std::vector<bool> sendNotifications(const std::vector<EmailAddress>& emailAddresses);

void getUserPreferences2(const std::vector<UserId>& userIds, std::vector<std::optional<UserPreferences>*>&);
void sendNotifications2(const std::vector<EmailAddress>& emailAddresses, std::vector<std::optional<bool>*>&);
