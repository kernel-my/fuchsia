// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

use {failure::Error, fuchsia_inspect::reader::NodeHierarchy};

pub use crate::json::*;

pub mod json;
mod utils;

/// Node hierarchies to be formatted including information about the path.
/// Example:
///
/// ```
/// HierarchyData {
///     hierarchy: SOME_HIERARCHY,
///     file_path: "/some/path",
///     fields: vec!["root", "node1", "node2"],
/// }
/// ```
///
/// Means that the hierarchy `SOME_HIERARCHY` points to the inspect node named
/// `node2` that is child of the node `node1` under `root` in an inspect file
/// located at `/some/path`.
///
pub struct HierarchyData {
    /// The node hierarchy to be formatted.
    pub hierarchy: NodeHierarchy,

    /// The path where the inspect file is.
    pub file_path: String,

    /// The path to the node within the inspect tree.
    pub fields: Vec<String>,
}

/// Implementors of this trait will provide different ways of formatting an
/// inspect hierarchy.
pub trait HierarchyFormatter {
    fn format(hierarchies: Vec<HierarchyData>) -> Result<String, Error>;
}
