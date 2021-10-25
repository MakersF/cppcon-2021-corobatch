#pragma once

#include <common.h>

void sendEmails_NoBatching(const std::vector<User>& users);
void sendEmails_HandBatching(const std::vector<User>& users);
void sendEmails_RangeBatching(const std::vector<User>& users);
void sendEmails_CoroBatching(const std::vector<User>& users);
void sendEmails_CoroBatching2(const std::vector<User>& users);
void sendEmails_CoroBatching2CustomAlloc(const std::vector<User>& users);
