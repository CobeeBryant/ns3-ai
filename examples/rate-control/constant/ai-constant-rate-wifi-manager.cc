/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2004,2005 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 * Modify: Xun Deng <dorence@hust.edu.cn>
 *         Hao Yin <haoyin@uw.edu>
 */

#include "ai-constant-rate-wifi-manager.h"

#include <ns3/log.h>
#include <ns3/string.h>
#include <ns3/wifi-phy.h>
#include <ns3/wifi-tx-vector.h>
#include <ns3/wifi-utils.h>

#define Min(a, b) ((a < b) ? a : b)

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("AiConstantRateWifiManager");

NS_OBJECT_ENSURE_REGISTERED(AiConstantRateWifiManager);

NS3AIRL<AiConstantRateEnvStruct, AiConstantRateActStruct> m_ns3ai_mod =
    NS3AIRL<AiConstantRateEnvStruct, AiConstantRateActStruct>(4096);
bool ns3ai_initialized = false;

TypeId
AiConstantRateWifiManager::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::AiConstantRateWifiManager")
            .SetParent<WifiRemoteStationManager>()
            .SetGroupName("Wifi")
            .AddConstructor<AiConstantRateWifiManager>()
            .AddAttribute("DataMode",
                          "The transmission mode to use for every data packet transmission",
                          StringValue("OfdmRate6Mbps"),
                          MakeWifiModeAccessor(&AiConstantRateWifiManager::m_dataMode),
                          MakeWifiModeChecker())
            .AddAttribute("ControlMode",
                          "The transmission mode to use for every RTS packet transmission.",
                          StringValue("OfdmRate6Mbps"),
                          MakeWifiModeAccessor(&AiConstantRateWifiManager::m_ctlMode),
                          MakeWifiModeChecker());
    return tid;
}

AiConstantRateWifiManager::AiConstantRateWifiManager()
{
    NS_LOG_FUNCTION(this);

    AiConstantRateEnvStruct env_struct{0, 0, 0};
    AiConstantRateActStruct act_struct{0, 0};

    if (!ns3ai_initialized)
    {
        NS_ASSERT(m_ns3ai_mod.m_env->empty());
        m_ns3ai_mod.m_env->resize(1, env_struct);
        NS_ASSERT(m_ns3ai_mod.m_act->empty());
        m_ns3ai_mod.m_act->resize(1, act_struct);
        ns3ai_initialized = true;
    }
}

AiConstantRateWifiManager::~AiConstantRateWifiManager()
{
    NS_LOG_FUNCTION(this);
}

WifiRemoteStation*
AiConstantRateWifiManager::DoCreateStation() const
{
    NS_LOG_FUNCTION(this);
    WifiRemoteStation* station = new WifiRemoteStation();
    return station;
}

void
AiConstantRateWifiManager::DoReportRxOk(WifiRemoteStation* station, double rxSnr, WifiMode txMode)
{
    NS_LOG_FUNCTION(this << station << rxSnr << txMode);
}

void
AiConstantRateWifiManager::DoReportRtsFailed(WifiRemoteStation* station)
{
    NS_LOG_FUNCTION(this << station);
}

void
AiConstantRateWifiManager::DoReportDataFailed(WifiRemoteStation* station)
{
    NS_LOG_FUNCTION(this << station);
}

void
AiConstantRateWifiManager::DoReportRtsOk(WifiRemoteStation* st,
                                         double ctsSnr,
                                         WifiMode ctsMode,
                                         double rtsSnr)
{
    NS_LOG_FUNCTION(this << st << ctsSnr << ctsMode << rtsSnr);
}

void
AiConstantRateWifiManager::DoReportDataOk(WifiRemoteStation* st,
                                          double ackSnr,
                                          WifiMode ackMode,
                                          double dataSnr,
                                          uint16_t dataChannelWidth,
                                          uint8_t dataNss)
{
    NS_LOG_FUNCTION(this << st << ackSnr << ackMode << dataSnr << dataChannelWidth << +dataNss);
}

void
AiConstantRateWifiManager::DoReportFinalRtsFailed(WifiRemoteStation* station)
{
    NS_LOG_FUNCTION(this << station);
}

void
AiConstantRateWifiManager::DoReportFinalDataFailed(WifiRemoteStation* station)
{
    NS_LOG_FUNCTION(this << station);
}

WifiTxVector
AiConstantRateWifiManager::DoGetDataTxVector(WifiRemoteStation* st, uint16_t allowedWidth)
{
    NS_LOG_FUNCTION(this << st);

    m_ns3ai_mod.set_env_begin();
    m_ns3ai_mod.m_env->at(0).transmitStreams = GetMaxNumberOfTransmitStreams();
    m_ns3ai_mod.m_env->at(0).supportedStreams = GetNumberOfSupportedStreams(st);
    if (m_dataMode.GetModulationClass() == WIFI_MOD_CLASS_HT) {
        m_ns3ai_mod.m_env->at(0).mcs = m_dataMode.GetMcsValue();
    } else {
        m_ns3ai_mod.m_env->at(0).mcs = 0xffU;
    }
    m_ns3ai_mod.set_env_end();

    m_ns3ai_mod.get_act_begin();
    uint8_t nss = m_ns3ai_mod.m_act->at(0).nss;
    uint8_t next_mcs = m_ns3ai_mod.m_act->at(0).next_mcs;
    NS_LOG_FUNCTION(next_mcs);
    m_ns3ai_mod.get_act_end();

    // uncomment to specify arbitrary MCS
    // m_dataMode = GetMcsSupported (st, next_mcs);

    return WifiTxVector(
        m_dataMode,
        GetDefaultTxPowerLevel(),
        GetPreambleForTransmission(m_dataMode.GetModulationClass(), GetShortPreambleEnabled()),
        ConvertGuardIntervalToNanoSeconds(m_dataMode,
                                          GetShortGuardIntervalSupported(st),
                                          NanoSeconds(GetGuardInterval(st))),
        GetNumberOfAntennas(),
        nss,
        0,
        GetPhy()->GetTxBandwidth(m_dataMode, std::min(allowedWidth, GetChannelWidth(st))),
        GetAggregation(st));
}

WifiTxVector
AiConstantRateWifiManager::DoGetRtsTxVector(WifiRemoteStation* st)
{
    NS_LOG_FUNCTION(this << st);
    return WifiTxVector(
        m_ctlMode,
        GetDefaultTxPowerLevel(),
        GetPreambleForTransmission(m_ctlMode.GetModulationClass(), GetShortPreambleEnabled()),
        ConvertGuardIntervalToNanoSeconds(m_ctlMode,
                                          GetShortGuardIntervalSupported(st),
                                          NanoSeconds(GetGuardInterval(st))),
        1,
        1,
        0,
        GetPhy()->GetTxBandwidth(m_dataMode, GetChannelWidth(st)),
        GetAggregation(st));
}

} // namespace ns3
