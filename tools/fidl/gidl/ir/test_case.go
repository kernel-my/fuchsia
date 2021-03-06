// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package ir

import "fmt"

type All struct {
	EncodeSuccess []EncodeSuccess
	DecodeSuccess []DecodeSuccess
	EncodeFailure []EncodeFailure
	DecodeFailure []DecodeFailure
}

type EncodeSuccess struct {
	Name              string
	WireFormat        WireFormat
	Value             interface{}
	Bytes             []byte
	BindingsAllowlist *[]string
	BindingsDenylist  *[]string
	// Handles
}

type DecodeSuccess struct {
	Name              string
	WireFormat        WireFormat
	Value             interface{}
	Bytes             []byte
	BindingsAllowlist *[]string
	BindingsDenylist  *[]string
	// Handles
}

type EncodeFailure struct {
	Name              string
	WireFormat        WireFormat
	Value             interface{}
	Err               ErrorCode
	BindingsAllowlist *[]string
	BindingsDenylist  *[]string
}

type DecodeFailure struct {
	Name              string
	WireFormat        WireFormat
	Type              string
	Bytes             []byte
	Err               ErrorCode
	BindingsAllowlist *[]string
	BindingsDenylist  *[]string
}

type WireFormat uint

const (
	_ WireFormat = iota
	OldWireFormat
	V1WireFormat
	DefaultWireFormat = OldWireFormat
)

var wireFormats = map[string]WireFormat{
	"old": OldWireFormat,
	"v1":  V1WireFormat,
}

func (input WireFormat) String() string {
	for s, wf := range wireFormats {
		if wf == input {
			return s
		}
	}
	panic(fmt.Sprintf("wire format %d not found", input))
}

func TestCaseName(name string, wf WireFormat) string {
	if wf == DefaultWireFormat {
		return name
	}
	return fmt.Sprintf("%s_%s", name, wf)
}

func ParseWireFormat(str string) (WireFormat, error) {
	if wf, ok := wireFormats[str]; ok {
		return wf, nil
	}
	return 0, fmt.Errorf("unknown wire format %q", str)
}
