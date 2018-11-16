// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <stdint.h>

#include <fbl/function.h>
#include <fbl/string.h>
#include <zircon/compiler.h>

namespace cobalt_client {

// Defines basic set of options for instantiating a metric.
struct MetricOptions {

    enum class Mode : uint8_t {
        // This mode marks a set of options as a placeholder, allowing metric instantiations to
        // defer initialization to a later stage.
        kLazy,
        // Metric is aggregated locally and published via collector interface.
        kLocal,
        // Metric deltas are aggregated locally, and sent for global agreggation to a remote
        // service.
        kRemote,
        // Combination of kLocal and kRemote.
        kRemoteAndLocal,
    };

    void SetMode(Mode mode) { this->mode = mode; }

    // Returns true if the metrics supports remote collection.
    // This is values collected by another service, such as Cobalt.
    bool IsRemote() const { return mode == Mode::kRemote || mode == Mode::kRemoteAndLocal; }

    // Returns true if the metric supports in process collection.
    // This is values tied to the process life-time.
    bool IsLocal() const { return mode == Mode::kLocal || mode == Mode::kRemoteAndLocal; }

    // Returns true if this does not represent a valid configuration, and is in |kLazy| mode.
    bool IsLazy() const { return mode == Mode::kLazy; }

    // Required for local metrics. If not set, and metric is both Local and Remote,
    // this will be generated from the |metric_id|, |event_code|(if not 0) and |component|(if not
    // empty).
    fbl::String name;

    // Provides refined metric collection for remote and local metrics.
    // Warning: |component| is not yet supported in the backend, so it will be ignored.
    fbl::String component;

    // Function that translates |metric_id| to a human readable name.
    // If returns |nullptr| or is unset, the stringfied version of |uint32_t| will be used.
    const char* (*get_metric_name)(uint32_t);

    // Function that translates |event_code| to a human readable name.
    // If returns |nullptr| or is unset, the stringfied version of |uint32_t| will be used.
    const char* (*get_event_name)(uint32_t);

    // Used by remote metrics to match with the respective unique id for the projects defined
    // metrics in the backend.
    uint32_t metric_id;

    // Provides refined metric collection for |kRemote| and |kLocal| metrics.
    // |event_code| 0 is reserved for Unkown events.
    // Warning: |event_code| is not yet supported in the backend, so it will be set to 0.
    uint32_t event_code;

    // Defines whether the metric is local or remote.
    // Internal use, should not be set manually.
    Mode mode = Mode::kLazy;
};

// Describes an histogram, and provides data for mapping a value to a given bucket.
// Every histogram contains two additional buckets, one at index 0 and bucket_count + 1.
// These buckets are used to store underflow and overflow respectively.
//
// buckets = [-inf, min_value) ...... [max_value, +inf)
//
// Parameters are calculated by the factory methods based on the input parameters,
// so that expectations are met.
//
// If using cobalt to flush your observations to the backend, this options should match
// your metric definitions for correct behavior. Mismatch with the respective metric definition
// will not allow proper collection and aggregation of metrics in the backend.
struct HistogramOptions : public MetricOptions {
    enum class Type {
        // Each bucket is described in the following form:
        // range(i) =  [ b * i + c, b * {i +1} + c)
        // i = (val - c) / b
        kLinear,
        // Each bucket is described in the following form:
        // range(i) =  [ b * a^i + c, b * a^{i+1} + c)
        // The cost of this type is O(1), because:
        // i = floor(log (val - c)  - log b)/log a
        kExponential,
    };

    // Returns HistogramOptions for a Histogram whose bucket size follow an exponential progression.
    // |scalar| * |base|^(current_step) + |offset| - |scalar| = lowerbound(current_step).
    // offset' = |offset| - |scalar|
    // |scalar| * |base|^(current_step) + offset' = lowerbound(current_step).
    static HistogramOptions Exponential(uint32_t bucket_count, uint32_t base, uint32_t scalar,
                                        int64_t offset);

    // Returns HistogramOptions for a Histogram whose bucket size follow an exponential progression.
    // |scalar| * current_step + offset = lowerbound(current_step).
    static HistogramOptions Linear(uint32_t bucket_count, uint32_t scalar, int64_t offset);

    HistogramOptions() = default;
    HistogramOptions(const HistogramOptions&);

    // Sanity check.
    bool IsValid() const {
        switch (type) {
        case Type::kExponential:
            if (base == 0) {
                return false;
            }
            __FALLTHROUGH;
        case Type::kLinear:
            if (scalar == 0) {
                return false;
            }
            break;
        }

        return true;
    }

    // This parameters should not be set manually.

    // Function used for mapping a value to a given bucket.
    uint32_t (*map_fn)(double, uint32_t, const HistogramOptions&) = nullptr;

    // Function used for mapping a bucket to its lowerbound.
    double (*reverse_map_fn)(uint32_t, uint32_t, const HistogramOptions&) = nullptr;

    // Base to describe the width of each step, in |kExponentialWidth|.
    double base = 1;

    // Scalar used by the type. This scales the width of each step.
    double scalar = 1;

    // This matchest offset', which is calculated depending on the histogram type.
    double offset = 0;

    // Cached upper bound for the histogram.
    double max_value = 0;

    // Type of the histogram to be constructed.
    Type type;
};

} // namespace cobalt_client
