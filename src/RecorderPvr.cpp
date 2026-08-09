/*
 * Therand TV Recorder PVR timer integration.
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "IptvSimple.h"

#include "iptvsimple/data/EpgEntry.h"
#include "iptvsimple/utilities/Logger.h"

#include <algorithm>
#include <ctime>
#include <string>
#include <vector>

#include <kodi/gui/dialogs/Numeric.h>

using namespace iptvsimple;
using namespace iptvsimple::data;
using namespace iptvsimple::utilities;

namespace
{
constexpr unsigned int THERAND_TIMER_TYPE_EPG = 1;
constexpr unsigned int THERAND_TIMER_TYPE_MANUAL = 2;

bool IsVisibleTimerState(const std::string& state)
{
  return state == "scheduled" || state == "recording" || state == "error";
}

PVR_TIMER_STATE ToKodiTimerState(const std::string& state)
{
  if (state == "scheduled")
    return PVR_TIMER_STATE_SCHEDULED;
  if (state == "recording")
    return PVR_TIMER_STATE_RECORDING;
  if (state == "completed")
    return PVR_TIMER_STATE_COMPLETED;
  if (state == "cancelled")
    return PVR_TIMER_STATE_CANCELLED;
  if (state == "error")
    return PVR_TIMER_STATE_ERROR;
  return PVR_TIMER_STATE_NEW;
}

unsigned int SecondsToKodiMinutes(unsigned int seconds)
{
  // Kodi stores recording margins in minutes. Round up so a configured 1..59s
  // margin is never silently lost.
  return seconds == 0 ? 0 : (seconds + 59) / 60;
}

bool AskManualDuration(time_t startAt, time_t& stopAt)
{
  std::string minutes = "60";
  if (!kodi::gui::dialogs::Numeric::ShowAndGetNumber(
          minutes, "Durée de l'enregistrement (minutes)"))
    return false;

  try
  {
    const unsigned long value = std::stoul(minutes);
    if (value == 0 || value > 24 * 60)
      return false;
    stopAt = startAt + static_cast<time_t>(value * 60);
    return true;
  }
  catch (...)
  {
    return false;
  }
}
} // unnamed namespace

bool IptvSimple::RefreshRecorderTimerIds()
{
  m_recorderTimerIds.clear();
  if (!m_recorderClient.IsEnabled())
    return false;

  std::vector<RecorderTimer> timers;
  if (!m_recorderClient.GetTimers(timers))
    return false;

  for (const auto& timer : timers)
  {
    if (IsVisibleTimerState(timer.state))
      m_recorderTimerIds[timer.clientIndex] = timer.id;
  }
  return true;
}

std::string IptvSimple::FindRecorderTimerId(unsigned int clientIndex)
{
  auto it = m_recorderTimerIds.find(clientIndex);
  if (it != m_recorderTimerIds.end())
    return it->second;

  if (!RefreshRecorderTimerIds())
    return {};

  it = m_recorderTimerIds.find(clientIndex);
  return it == m_recorderTimerIds.end() ? std::string{} : it->second;
}

PVR_ERROR IptvSimple::GetTimerTypes(std::vector<kodi::addon::PVRTimerType>& types)
{
  if (!m_recorderClient.IsEnabled())
    return PVR_ERROR_NOT_IMPLEMENTED;

  kodi::addon::PVRTimerType epgType;
  epgType.SetId(THERAND_TIMER_TYPE_EPG);
  epgType.SetAttributes(PVR_TIMER_TYPE_SUPPORTS_CHANNELS |
                        PVR_TIMER_TYPE_SUPPORTS_START_TIME |
                        PVR_TIMER_TYPE_SUPPORTS_END_TIME |
                        PVR_TIMER_TYPE_SUPPORTS_START_END_MARGIN |
                        PVR_TIMER_TYPE_REQUIRES_EPG_TAG_ON_CREATE);
  epgType.SetDescription("Enregistrer ce programme");
  types.emplace_back(epgType);

  kodi::addon::PVRTimerType manualType;
  manualType.SetId(THERAND_TIMER_TYPE_MANUAL);
  manualType.SetAttributes(PVR_TIMER_TYPE_IS_MANUAL |
                           PVR_TIMER_TYPE_SUPPORTS_CHANNELS |
                           PVR_TIMER_TYPE_SUPPORTS_START_TIME |
                           PVR_TIMER_TYPE_SUPPORTS_END_TIME |
                           PVR_TIMER_TYPE_SUPPORTS_START_END_MARGIN |
                           PVR_TIMER_TYPE_FORBIDS_EPG_TAG_ON_CREATE);
  manualType.SetDescription("Enregistrement manuel");
  types.emplace_back(manualType);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR IptvSimple::GetTimersAmount(int& amount)
{
  amount = 0;
  if (!m_recorderClient.IsEnabled())
    return PVR_ERROR_NOT_IMPLEMENTED;

  std::vector<RecorderTimer> timers;
  if (!m_recorderClient.GetTimers(timers))
    return PVR_ERROR_SERVER_ERROR;

  m_recorderTimerIds.clear();
  for (const auto& timer : timers)
  {
    if (!IsVisibleTimerState(timer.state))
      continue;
    ++amount;
    m_recorderTimerIds[timer.clientIndex] = timer.id;
  }
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR IptvSimple::GetTimers(kodi::addon::PVRTimersResultSet& results)
{
  if (!m_recorderClient.IsEnabled())
    return PVR_ERROR_NOT_IMPLEMENTED;

  std::vector<RecorderTimer> timers;
  if (!m_recorderClient.GetTimers(timers))
    return PVR_ERROR_SERVER_ERROR;

  m_recorderTimerIds.clear();
  for (const auto& source : timers)
  {
    if (!IsVisibleTimerState(source.state))
      continue;

    kodi::addon::PVRTimer timer;
    timer.SetClientIndex(source.clientIndex);
    timer.SetParentClientIndex(PVR_TIMER_NO_PARENT);
    timer.SetState(ToKodiTimerState(source.state));
    timer.SetTimerType(source.timerType == THERAND_TIMER_TYPE_MANUAL
                           ? THERAND_TIMER_TYPE_MANUAL
                           : THERAND_TIMER_TYPE_EPG);
    timer.SetClientChannelUid(source.channelUid);
    timer.SetTitle(source.title);
    timer.SetStartTime(source.startAt);
    timer.SetEndTime(source.stopAt);
    timer.SetMarginStart(SecondsToKodiMinutes(source.marginBeforeSeconds));
    timer.SetMarginEnd(SecondsToKodiMinutes(source.marginAfterSeconds));
    timer.SetEPGUid(source.epgUid == 0 ? PVR_TIMER_NO_EPG_UID : source.epgUid);

    results.Add(timer);
    m_recorderTimerIds[source.clientIndex] = source.id;
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR IptvSimple::AddTimer(const kodi::addon::PVRTimer& timer)
{
  if (!m_recorderClient.IsEnabled())
    return PVR_ERROR_SERVER_ERROR;

  Channel channel{m_settings};
  if (!GetChannel(static_cast<unsigned int>(timer.GetClientChannelUid()), channel))
    return PVR_ERROR_INVALID_PARAMETERS;

  const time_t now = std::time(nullptr);
  const bool isInstantRecording = timer.GetStartTime() <= 0;
  time_t startAt = timer.GetStartTime();
  time_t stopAt = timer.GetEndTime();
  if (startAt <= 0)
    startAt = now;

  std::string title = timer.GetTitle();
  unsigned int epgUid = timer.GetEPGUid() == PVR_TIMER_NO_EPG_UID ? 0 : timer.GetEPGUid();
  unsigned int timerType = timer.GetTimerType();
  bool hasLiveEpg = false;

  // Kodi marks an instant recording with start=0. If the timer did not bring a
  // usable end time, use the programme currently active in our already loaded
  // XMLTV guide. Recording still starts now; only its stop time follows the EPG.
  // For instant recordings, prefer the live EPG title even if Kodi supplied a
  // generic channel-based title, so the resulting filename names the programme.
  if (stopAt <= startAt || title.empty() || epgUid == 0 || isInstantRecording)
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    EpgEntry* liveEntry = m_epg.GetLiveEPGEntry(channel);
    if (liveEntry && liveEntry->GetEndTime() > now)
    {
      hasLiveEpg = true;
      if (stopAt <= startAt)
        stopAt = liveEntry->GetEndTime();
      if ((isInstantRecording || title.empty()) && !liveEntry->GetTitle().empty())
        title = liveEntry->GetTitle();
      if (epgUid == 0 && liveEntry->GetBroadcastId() > 0)
        epgUid = static_cast<unsigned int>(liveEntry->GetBroadcastId());
    }
  }

  // No EPG and no explicit end time: ask the user rather than silently picking
  // an arbitrary duration. This is the fallback for the red record button.
  if (stopAt <= startAt)
  {
    if (!AskManualDuration(startAt, stopAt))
      return PVR_ERROR_REJECTED;
    timerType = THERAND_TIMER_TYPE_MANUAL;
    epgUid = 0;
  }
  else if (hasLiveEpg)
  {
    timerType = THERAND_TIMER_TYPE_EPG;
  }

  if (title.empty())
    title = channel.GetChannelName();

  if (timerType != THERAND_TIMER_TYPE_EPG && timerType != THERAND_TIMER_TYPE_MANUAL)
    timerType = epgUid == 0 ? THERAND_TIMER_TYPE_MANUAL : THERAND_TIMER_TYPE_EPG;

  RecorderTimerRequest request;
  request.channelUid = channel.GetUniqueId();
  request.channelName = channel.GetChannelName();
  request.tvgId = channel.GetTvgId();
  request.streamUrl = channel.GetStreamURL();
  request.title = title;
  request.startAt = startAt;
  request.stopAt = stopAt;
  request.marginBeforeSeconds =
      timer.GetMarginStart() == 0
          ? m_recorderClient.GetDefaultMarginBeforeSeconds()
          : timer.GetMarginStart() * 60;
  request.marginAfterSeconds =
      timer.GetMarginEnd() == 0
          ? m_recorderClient.GetDefaultMarginAfterSeconds()
          : timer.GetMarginEnd() * 60;
  request.timerType = timerType;
  request.epgUid = epgUid;

  RecorderTimer created;
  if (!m_recorderClient.CreateTimer(request, created))
    return PVR_ERROR_SERVER_ERROR;

  m_recorderTimerIds[created.clientIndex] = created.id;
  TriggerTimerUpdate();

  Logger::Log(LEVEL_INFO, "%s - Added Therand timer '%s' on channel '%s'", __FUNCTION__,
              created.id.c_str(), channel.GetChannelName().c_str());
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR IptvSimple::DeleteTimer(const kodi::addon::PVRTimer& timer, bool forceDelete)
{
  if (!m_recorderClient.IsEnabled())
    return PVR_ERROR_SERVER_ERROR;

  if (timer.GetState() == PVR_TIMER_STATE_RECORDING && !forceDelete)
    return PVR_ERROR_RECORDING_RUNNING;

  const std::string id = FindRecorderTimerId(timer.GetClientIndex());
  if (id.empty())
    return PVR_ERROR_INVALID_PARAMETERS;

  if (!m_recorderClient.DeleteTimer(id))
    return PVR_ERROR_SERVER_ERROR;

  m_recorderTimerIds.erase(timer.GetClientIndex());
  TriggerTimerUpdate();
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR IptvSimple::UpdateTimer(const kodi::addon::PVRTimer& timer)
{
  if (!m_recorderClient.IsEnabled())
    return PVR_ERROR_SERVER_ERROR;

  const std::string id = FindRecorderTimerId(timer.GetClientIndex());
  if (id.empty())
    return PVR_ERROR_INVALID_PARAMETERS;

  Channel channel{m_settings};
  if (!GetChannel(static_cast<unsigned int>(timer.GetClientChannelUid()), channel))
    return PVR_ERROR_INVALID_PARAMETERS;

  const time_t startAt = timer.GetStartTime();
  const time_t stopAt = timer.GetEndTime();
  if (startAt <= 0 || stopAt <= startAt)
    return PVR_ERROR_INVALID_PARAMETERS;

  RecorderTimerRequest request;
  request.channelUid = channel.GetUniqueId();
  request.channelName = channel.GetChannelName();
  request.tvgId = channel.GetTvgId();
  request.streamUrl = channel.GetStreamURL();
  request.title = timer.GetTitle().empty() ? channel.GetChannelName() : timer.GetTitle();
  request.startAt = startAt;
  request.stopAt = stopAt;
  request.marginBeforeSeconds = timer.GetMarginStart() * 60;
  request.marginAfterSeconds = timer.GetMarginEnd() * 60;
  request.timerType = timer.GetTimerType() == THERAND_TIMER_TYPE_MANUAL
                          ? THERAND_TIMER_TYPE_MANUAL
                          : THERAND_TIMER_TYPE_EPG;
  request.epgUid = timer.GetEPGUid() == PVR_TIMER_NO_EPG_UID ? 0 : timer.GetEPGUid();

  RecorderTimer updated;
  if (!m_recorderClient.UpdateTimer(id, request, updated))
    return PVR_ERROR_REJECTED;

  m_recorderTimerIds[updated.clientIndex] = updated.id;
  TriggerTimerUpdate();
  return PVR_ERROR_NO_ERROR;
}
