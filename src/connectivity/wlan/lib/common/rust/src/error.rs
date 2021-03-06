// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

use {
    crate::appendable,
    failure::{self, Fail},
};

#[derive(Fail, Debug, PartialEq, Eq)]
pub enum FrameWriteError {
    #[fail(display = "Buffer is too small")]
    BufferTooSmall,
    #[fail(display = "Attempted to write an invalid frame: {}", debug_message)]
    InvalidData { debug_message: String },
}

impl FrameWriteError {
    pub fn new_invalid_data<S: Into<String>>(debug_message: S) -> Self {
        FrameWriteError::InvalidData { debug_message: debug_message.into() }
    }
}

impl From<appendable::BufferTooSmall> for FrameWriteError {
    fn from(_error: appendable::BufferTooSmall) -> Self {
        FrameWriteError::BufferTooSmall
    }
}

#[derive(Fail, Debug, PartialEq, Eq)]
#[fail(display = "Error parsing frame: {}", debug_message)]
pub struct FrameParseError {
    debug_message: &'static str,
}

impl FrameParseError {
    pub fn new(debug_message: &'static str) -> Self {
        return FrameParseError { debug_message };
    }

    pub fn debug_message(&self) -> &str {
        self.debug_message
    }
}

pub type FrameParseResult<T> = Result<T, FrameParseError>;
