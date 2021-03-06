// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

use {
    super::*,
    crate::{
        client::{scan, ConnectResult, ConnectionAttemptId, ScanTxnId},
        sink::InfoSink,
        Ssid,
    },
    fidl_fuchsia_wlan_mlme as fidl_mlme, fuchsia_zircon as zx,
    log::warn,
    wlan_rsn::rsna::UpdateSink,
};

macro_rules! warn_if_err {
    ($expr:expr) => {{
        if let Err(e) = &$expr {
            warn!("[stats] {}", e);
        };
    }};
}

pub(crate) struct InfoReporter {
    info_sink: InfoSink,
    stats_collector: StatsCollector,
}

impl InfoReporter {
    pub fn new(info_sink: InfoSink) -> Self {
        Self { info_sink, stats_collector: StatsCollector::default() }
    }

    pub fn report_scan_started(
        &mut self,
        req: fidl_mlme::ScanRequest,
        join_scan: bool,
        is_connected: bool,
    ) {
        self.info_sink.send(InfoEvent::MlmeScanStart { txn_id: req.txn_id });
        if join_scan {
            warn_if_err!(self.stats_collector.report_join_scan_started(req, is_connected));
        } else {
            if let Some(_) = self.stats_collector.report_discovery_scan_started(req, is_connected) {
                warn!("[stats] evicting unfinished discovery scan attempt");
            }
        }
    }

    pub fn report_scan_ended<D, J>(&mut self, txn_id: ScanTxnId, result: &scan::ScanResult<D, J>) {
        self.info_sink.send(InfoEvent::MlmeScanEnd { txn_id });
        match result {
            scan::ScanResult::DiscoveryFinished { result, .. } => {
                let (bss_list, scan_result) = convert_scan_result(result);
                let stats = self.stats_collector.report_discovery_scan_ended(scan_result, bss_list);
                warn_if_err!(stats);
                if let Ok(stats) = stats {
                    self.info_sink.send(InfoEvent::DiscoveryScanStats(stats.0, stats.1));
                }
            }
            scan::ScanResult::JoinScanFinished { result, .. } => {
                let (bss_list, scan_result) = convert_scan_result(result);
                // Join scan stats are collected as part of ConnectStats, which will be reported
                // when the connect attempt finishes
                let bss_count = bss_list.map(|bss_list| bss_list.len()).unwrap_or(0);
                warn_if_err!(self.stats_collector.report_join_scan_ended(scan_result, bss_count));
            }
            scan::ScanResult::None => (),
        }
    }

    pub fn report_connect_started(&mut self, ssid: Ssid) {
        self.info_sink.send(InfoEvent::ConnectStarted);
        if let Some(_prev) = self.stats_collector.report_connect_started(ssid) {
            warn!("[stats] evicting unfinished connect attempt");
        }
    }

    pub fn report_candidate_network(&mut self, desc: fidl_mlme::BssDescription) {
        warn_if_err!(self.stats_collector.report_candidate_network(desc));
    }

    pub fn report_join_started(&mut self, att_id: ConnectionAttemptId) {
        self.info_sink.send(InfoEvent::JoinStarted { att_id });
    }

    pub fn report_auth_started(&mut self) {
        warn_if_err!(self.stats_collector.report_auth_started());
    }

    pub fn report_assoc_started(&mut self) {
        warn_if_err!(self.stats_collector.report_assoc_started());
    }

    pub fn report_assoc_success(&mut self, att_id: ConnectionAttemptId) {
        self.info_sink.send(InfoEvent::AssociationSuccess { att_id });
        warn_if_err!(self.stats_collector.report_assoc_success());
    }

    pub fn report_rsna_started(&mut self, att_id: ConnectionAttemptId) {
        self.info_sink.send(InfoEvent::RsnaStarted { att_id });
        warn_if_err!(self.stats_collector.report_rsna_started());
    }

    pub fn report_rsna_established(&mut self, att_id: ConnectionAttemptId) {
        self.info_sink.send(InfoEvent::RsnaEstablished { att_id });
        warn_if_err!(self.stats_collector.report_rsna_established());
    }

    pub fn report_key_exchange_timeout(&mut self) {
        warn_if_err!(self.stats_collector.report_key_exchange_timeout());
    }

    pub fn report_supplicant_updates(&mut self, update_sink: &UpdateSink) {
        warn_if_err!(self.stats_collector.report_supplicant_updates(update_sink));
    }

    pub fn report_supplicant_error(&mut self, error: failure::Error) {
        warn_if_err!(self.stats_collector.report_supplicant_error(error));
    }

    pub fn report_connect_finished(&mut self, result: ConnectResult) {
        self.info_sink.send(InfoEvent::ConnectFinished { result: result.clone() });
        let stats = self.stats_collector.report_connect_finished(result);
        warn_if_err!(stats);
        if let Ok(stats) = stats {
            self.info_sink.send(InfoEvent::ConnectStats(stats));
        }
    }

    pub fn report_connection_ping(&mut self, info: ConnectionPingInfo) {
        self.info_sink.send(InfoEvent::ConnectionPing(info));
    }

    pub fn report_connection_lost(
        &mut self,
        connected_duration: zx::Duration,
        last_rssi: i8,
        bssid: [u8; 6],
        ssid: Ssid,
    ) {
        self.info_sink.send(InfoEvent::ConnectionLost(ConnectionLostInfo {
            connected_duration,
            last_rssi,
            bssid,
        }));
        self.stats_collector.report_disconnect(ssid, DisconnectCause::Drop);
    }

    pub fn report_manual_disconnect(&mut self, ssid: Ssid) {
        self.stats_collector.report_disconnect(ssid, DisconnectCause::Manual);
    }
}

fn convert_scan_result(
    result: &Result<Vec<fidl_mlme::BssDescription>, fidl_mlme::ScanResultCodes>,
) -> (Option<&Vec<fidl_mlme::BssDescription>>, ScanResult) {
    match result {
        Ok(bss_list) => (Some(bss_list), ScanResult::Success),
        Err(code) => (None, ScanResult::Failed(*code)),
    }
}
