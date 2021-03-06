// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_DEVELOPER_FEEDBACK_CRASHPAD_AGENT_INSPECT_MANAGER_H_
#define SRC_DEVELOPER_FEEDBACK_CRASHPAD_AGENT_INSPECT_MANAGER_H_

#include <lib/inspect/cpp/vmo/types.h>
#include <lib/timekeeper/clock.h>

#include <map>
#include <string>
#include <vector>

#include "src/developer/feedback/crashpad_agent/config.h"
#include "src/developer/feedback/crashpad_agent/settings.h"
#include "src/developer/feedback/utils/inspect_node_manager.h"
#include "src/lib/fxl/macros.h"

namespace feedback {

// Encapsulates the global state exposed through Inspect.
class InspectManager {
 public:
  InspectManager(inspect::Node* root_node, timekeeper::Clock* clock);

  // Exposes the static configuration of the crash reporter.
  void ExposeConfig(const feedback::Config& config);

  // Exposes the mutable settings of the crash reporter.
  void ExposeSettings(feedback::Settings* settings);

  // Adds a new report under the given program.
  //
  // Returns false if there is already a report with |local_report_id| as ID (for the given program
  // or another).
  bool AddReport(const std::string& program_name, const std::string& local_report_id);

  // Marks an existing report as uploaded, storing its server report ID.
  //
  // Returns false if there are no reports with |local_report_id| as ID.
  bool MarkReportAsUploaded(const std::string& local_report_id,
                            const std::string& server_report_id);

 private:
  bool Contains(const std::string& local_report_id);

  // Callback to update |settings_| on upload policy changes.
  void OnUploadPolicyChange(const feedback::Settings::UploadPolicy& upload_policy);

  // Returns a non-localized human-readable timestamp of the current time according to |clock_|.
  std::string CurrentTime();

  // Inspect node containing the static configuration.
  struct Config {
    // Inspect node containing the database configuration.
    struct CrashpadDatabaseConfig {
      inspect::StringProperty path;
      inspect::UintProperty max_size_in_kb;
    };

    // Inspect node containing the crash server configuration.
    struct CrashServerConfig {
      inspect::StringProperty upload_policy;
      inspect::StringProperty url;
    };

    CrashpadDatabaseConfig crashpad_database;
    CrashServerConfig crash_server;
  };

  // Inspect node containing the mutable settings.
  struct Settings {
    inspect::StringProperty upload_policy;
  };

  // Inspect node for a single report.
  struct Report {
    Report(const std::string& path) : path_(path) {}

    const std::string& Path() { return path_; }

    inspect::StringProperty creation_time_;

    inspect::StringProperty server_id_;
    inspect::StringProperty server_creation_time_;

   private:
    // A |Report|'s path is its location relative to the root Inspect node in the Inspect tree.
    //
    // E.g., "/reports/$program_id/$local_report_id"
    const std::string path_;
  };

  InspectNodeManager node_manager_;
  timekeeper::Clock* clock_;
  Config config_;
  Settings settings_;
  // Maps a local report ID to a |Report|.
  std::map<std::string, Report> reports_;

  FXL_DISALLOW_COPY_AND_ASSIGN(InspectManager);
};

}  // namespace feedback

#endif  // SRC_DEVELOPER_FEEDBACK_CRASHPAD_AGENT_INSPECT_MANAGER_H_
