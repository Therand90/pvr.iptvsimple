/*
 * Therand TV Recorder integration for IPTV Simple Omega.
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <cstdint>
#include <ctime>
#include <memory>
#include <string>
#include <vector>

namespace iptvsimple
{
class InstanceSettings;

struct RecorderTimer
{
  std::string id;
  unsigned int clientIndex{0};
  std::string state;
  unsigned int timerType{1};
  unsigned int epgUid{0};
  int channelUid{0};
  std::string channelName;
  std::string tvgId;
  std::string title;
  time_t startAt{0};
  time_t stopAt{0};
  unsigned int marginBeforeSeconds{0};
  unsigned int marginAfterSeconds{0};
  std::string relativeOutputPath;
  uint64_t outputSizeBytes{0};
  std::string error;
};

struct RecorderRecording
{
  std::string id;
  int channelUid{0};
  unsigned int epgUid{0};
  std::string title;
  std::string channelName;
  std::string tvgId;
  time_t startAt{0};
  time_t stopAt{0};
  std::string relativeOutputPath;
  uint64_t outputSizeBytes{0};
};

struct RecorderTimerRequest
{
  int channelUid{0};
  std::string channelName;
  std::string tvgId;
  std::string streamUrl;
  std::string title;
  time_t startAt{0};
  time_t stopAt{0};
  unsigned int marginBeforeSeconds{0};
  unsigned int marginAfterSeconds{0};
  unsigned int timerType{1};
  unsigned int epgUid{0};
};

class RecorderClient
{
public:
  explicit RecorderClient(std::shared_ptr<InstanceSettings> settings);

  bool ReloadConfig();
  bool IsEnabled() const { return m_enabled && !m_backendUrl.empty() && !m_token.empty(); }
  unsigned int GetDefaultMarginBeforeSeconds() const { return m_defaultMarginBeforeSeconds; }
  unsigned int GetDefaultMarginAfterSeconds() const { return m_defaultMarginAfterSeconds; }

  bool GetTimers(std::vector<RecorderTimer>& timers);
  bool CreateTimer(const RecorderTimerRequest& request, RecorderTimer& timer);
  bool UpdateTimer(const std::string& id, const RecorderTimerRequest& request, RecorderTimer& timer);
  bool DeleteTimer(const std::string& id);

  bool GetRecordings(std::vector<RecorderRecording>& recordings);
  bool DeleteRecording(const std::string& id);
  std::string GetLocalRecordingPath(const std::string& relativePath) const;

private:
  bool Request(const std::string& path,
               const std::string& method,
               const std::string& requestBody,
               std::string& responseBody);
  std::string BuildTimerXml(const RecorderTimerRequest& request) const;

  std::shared_ptr<InstanceSettings> m_settings;
  bool m_enabled{false};
  std::string m_backendUrl;
  std::string m_token;
  std::string m_recordingsRoot;
  unsigned int m_defaultMarginBeforeSeconds{120};
  unsigned int m_defaultMarginAfterSeconds{300};
};
} // namespace iptvsimple
