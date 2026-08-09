/*
 * Therand TV Recorder integration for IPTV Simple Omega.
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "RecorderClient.h"

#include "InstanceSettings.h"
#include "utilities/CurlUtils.h"
#include "utilities/FileUtils.h"
#include "utilities/Logger.h"

#include <algorithm>
#include <sstream>

#include <pugixml.hpp>

using namespace iptvsimple::utilities;

namespace
{
std::string ChildString(const pugi::xml_node& node, const char* name)
{
  return node.child(name).text().as_string();
}

unsigned int ChildUInt(const pugi::xml_node& node, const char* name)
{
  return node.child(name).text().as_uint();
}

uint64_t ChildUInt64(const pugi::xml_node& node, const char* name)
{
  const char* value = node.child(name).text().as_string("0");
  try
  {
    return static_cast<uint64_t>(std::stoull(value));
  }
  catch (...)
  {
    return 0;
  }
}

bool ParseTimerNode(const pugi::xml_node& node, iptvsimple::RecorderTimer& timer)
{
  if (!node || std::string(node.name()) != "timer")
    return false;

  timer.id = ChildString(node, "id");
  timer.clientIndex = ChildUInt(node, "client_index");
  timer.state = ChildString(node, "state");
  timer.timerType = ChildUInt(node, "timer_type");
  timer.epgUid = ChildUInt(node, "epg_uid");
  timer.channelUid = node.child("channel_uid").text().as_int();
  timer.channelName = ChildString(node, "channel_name");
  timer.tvgId = ChildString(node, "tvg_id");
  timer.title = ChildString(node, "title");
  timer.startAt = static_cast<time_t>(ChildUInt64(node, "start_at"));
  timer.stopAt = static_cast<time_t>(ChildUInt64(node, "stop_at"));
  timer.marginBeforeSeconds = ChildUInt(node, "margin_before_seconds");
  timer.marginAfterSeconds = ChildUInt(node, "margin_after_seconds");
  timer.relativeOutputPath = ChildString(node, "relative_output_path");
  timer.outputSizeBytes = ChildUInt64(node, "output_size_bytes");
  timer.error = ChildString(node, "error");

  return !timer.id.empty() && timer.clientIndex != 0;
}

bool ParseRecordingNode(const pugi::xml_node& node, iptvsimple::RecorderRecording& recording)
{
  if (!node || std::string(node.name()) != "recording")
    return false;

  recording.id = ChildString(node, "id");
  recording.title = ChildString(node, "title");
  recording.channelName = ChildString(node, "channel_name");
  recording.tvgId = ChildString(node, "tvg_id");
  recording.startAt = static_cast<time_t>(ChildUInt64(node, "start_at"));
  recording.stopAt = static_cast<time_t>(ChildUInt64(node, "stop_at"));
  recording.relativeOutputPath = ChildString(node, "relative_output_path");
  recording.outputSizeBytes = ChildUInt64(node, "output_size_bytes");

  return !recording.id.empty();
}
} // unnamed namespace

namespace iptvsimple
{

RecorderClient::RecorderClient(std::shared_ptr<InstanceSettings> settings) : m_settings(settings)
{
  ReloadConfig();
}

bool RecorderClient::ReloadConfig()
{
  m_enabled = false;
  m_backendUrl.clear();
  m_token.clear();
  m_defaultMarginBeforeSeconds = 120;
  m_defaultMarginAfterSeconds = 300;

  const std::string configPath =
      FileUtils::GetUserDataAddonFilePath(m_settings->GetUserPath(), "therand-recorder.xml");

  std::string contents;
  if (FileUtils::GetFileContents(configPath, contents) <= 0)
  {
    Logger::Log(LEVEL_INFO,
                "%s - Therand recorder config not found; recording backend disabled (%s)",
                __FUNCTION__, configPath.c_str());
    return false;
  }

  pugi::xml_document document;
  const pugi::xml_parse_result result = document.load_string(contents.c_str());
  if (!result)
  {
    Logger::Log(LEVEL_ERROR, "%s - Invalid Therand recorder XML config", __FUNCTION__);
    return false;
  }

  const pugi::xml_node root = document.child("therand-recorder");
  if (!root)
  {
    Logger::Log(LEVEL_ERROR, "%s - Missing <therand-recorder> root element", __FUNCTION__);
    return false;
  }

  m_enabled = root.child("enabled").text().as_bool(false);
  m_backendUrl = ChildString(root, "backend_url");
  m_token = ChildString(root, "token");
  m_defaultMarginBeforeSeconds =
      root.child("margin_before_seconds").text().as_uint(m_defaultMarginBeforeSeconds);
  m_defaultMarginAfterSeconds =
      root.child("margin_after_seconds").text().as_uint(m_defaultMarginAfterSeconds);

  while (!m_backendUrl.empty() && m_backendUrl.back() == '/')
    m_backendUrl.pop_back();

  if (!IsEnabled())
  {
    Logger::Log(LEVEL_INFO, "%s - Therand recorder backend is disabled or incomplete", __FUNCTION__);
    return false;
  }

  Logger::Log(LEVEL_INFO, "%s - Therand recorder backend enabled at %s", __FUNCTION__,
              m_backendUrl.c_str());
  return true;
}

bool RecorderClient::Request(const std::string& path,
                             const std::string& method,
                             const std::string& requestBody,
                             std::string& responseBody)
{
  responseBody.clear();
  if (!IsEnabled())
    return false;

  CUrl request(m_backendUrl + path);
  request.AddHeaders({{"Authorization", "Bearer " + m_token},
                      {"Accept", "application/xml"}});

  if (method == "POST")
  {
    request.AddHeaders({{"Content-Type", "application/xml"}});
    request.SetPostData(requestBody);
  }
  else if (method != "GET")
  {
    request.SetRequestMethod(method);
  }

  const int statusCode = request.Open();
  if (statusCode < 200 || statusCode >= 300)
  {
    Logger::Log(LEVEL_ERROR, "%s - Recorder backend %s %s returned HTTP %d", __FUNCTION__,
                method.c_str(), path.c_str(), statusCode);
    return false;
  }

  const ReadStatus readStatus = request.Read(responseBody);
  if (readStatus == ReadStatus::ERROR)
  {
    Logger::Log(LEVEL_ERROR, "%s - Failed reading recorder backend response for %s", __FUNCTION__,
                path.c_str());
    return false;
  }

  return true;
}

bool RecorderClient::GetTimers(std::vector<RecorderTimer>& timers)
{
  timers.clear();
  std::string response;
  if (!Request("/api/v1/pvr/timers", "GET", "", response))
    return false;

  pugi::xml_document document;
  if (!document.load_string(response.c_str()))
    return false;

  for (const pugi::xml_node& node : document.child("timers").children("timer"))
  {
    RecorderTimer timer;
    if (ParseTimerNode(node, timer))
      timers.emplace_back(std::move(timer));
  }
  return true;
}

bool RecorderClient::CreateTimer(const RecorderTimerRequest& timerRequest, RecorderTimer& timer)
{
  pugi::xml_document document;
  pugi::xml_node root = document.append_child("timer");
  root.append_child("channel_uid").text().set(timerRequest.channelUid);
  root.append_child("channel_name").text().set(timerRequest.channelName.c_str());
  root.append_child("tvg_id").text().set(timerRequest.tvgId.c_str());
  root.append_child("stream_url").text().set(timerRequest.streamUrl.c_str());
  root.append_child("title").text().set(timerRequest.title.c_str());
  root.append_child("start_at").text().set(static_cast<long long>(timerRequest.startAt));
  root.append_child("stop_at").text().set(static_cast<long long>(timerRequest.stopAt));
  root.append_child("margin_before_seconds").text().set(timerRequest.marginBeforeSeconds);
  root.append_child("margin_after_seconds").text().set(timerRequest.marginAfterSeconds);
  root.append_child("timer_type").text().set(timerRequest.timerType);
  root.append_child("epg_uid").text().set(timerRequest.epgUid);

  std::ostringstream stream;
  document.save(stream, "", pugi::format_raw);

  std::string response;
  if (!Request("/api/v1/pvr/timers", "POST", stream.str(), response))
    return false;

  pugi::xml_document responseDocument;
  if (!responseDocument.load_string(response.c_str()))
    return false;

  return ParseTimerNode(responseDocument.child("timer"), timer);
}

bool RecorderClient::DeleteTimer(const std::string& id)
{
  std::string response;
  return Request("/api/v1/pvr/timers/" + id, "DELETE", "", response);
}

bool RecorderClient::GetRecordings(std::vector<RecorderRecording>& recordings)
{
  recordings.clear();
  std::string response;
  if (!Request("/api/v1/pvr/recordings", "GET", "", response))
    return false;

  pugi::xml_document document;
  if (!document.load_string(response.c_str()))
    return false;

  for (const pugi::xml_node& node : document.child("recordings").children("recording"))
  {
    RecorderRecording recording;
    if (ParseRecordingNode(node, recording))
      recordings.emplace_back(std::move(recording));
  }
  return true;
}

bool RecorderClient::DeleteRecording(const std::string& id)
{
  std::string response;
  return Request("/api/v1/pvr/recordings/" + id, "DELETE", "", response);
}

} // namespace iptvsimple
