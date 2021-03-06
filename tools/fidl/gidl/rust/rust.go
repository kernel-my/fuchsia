// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package rust

import (
	"fmt"
	"io"
	"strconv"
	"strings"
	"text/template"

	fidlcommon "fidl/compiler/backend/common"
	fidlir "fidl/compiler/backend/types"
	gidlir "gidl/ir"
	gidlmixer "gidl/mixer"
)

var tmpl = template.Must(template.New("tmpls").Parse(`
use fidl::{Error, encoding::{Context, Decodable, Decoder, Encoder}};
use fidl_conformance as conformance;

const OLD_CONTEXT: &Context = &Context { unions_use_xunion_format: false };
const V1_CONTEXT: &Context = &Context { unions_use_xunion_format: true };

{{ range .EncodeSuccessCases }}
#[test]
fn test_{{ .Name }}_encode() {
	let value = &mut {{ .Value }};
	let bytes = &mut Vec::new();
	Encoder::encode_with_context({{ .Context }}, bytes, &mut Vec::new(), value).unwrap();
	assert_eq!(*bytes, &[{{ .Bytes }}][..]);
}
{{ end }}

{{ range .DecodeSuccessCases }}
#[test]
fn test_{{ .Name }}_decode() {
	let value = &mut {{ .ValueType }}::new_empty();
	let bytes = &mut [{{ .Bytes }}];
	Decoder::decode_with_context({{ .Context }}, bytes, &mut [], value).unwrap();
	assert_eq!(*value, {{ .Value }});
}
{{ end }}

{{ range .EncodeFailureCases }}
#[test]
fn test_{{ .Name }}_encode_failure() {
	let value = &mut {{ .Value }};
	let bytes = &mut Vec::new();
	match Encoder::encode_with_context({{ .Context }}, bytes, &mut Vec::new(), value) {
		Err({{ .ErrorCode }} { .. }) => (),
		Err(err) => panic!("unexpected error: {}", err),
		Ok(_) => panic!("unexpected successful encoding"),
	}
}
{{ end }}

{{ range .DecodeFailureCases }}
#[test]
fn test_{{ .Name }}_decode_failure() {
	let value = &mut {{ .ValueType }}::new_empty();
	let bytes = &mut [{{ .Bytes }}];
	match Decoder::decode_with_context({{ .Context }}, bytes, &mut [], value) {
		Err({{ .ErrorCode }} { .. }) => (),
		Err(err) => panic!("unexpected error: {}", err),
		Ok(_) => panic!("unexpected successful decoding"),
	}
}
{{ end }}
`))

type tmplInput struct {
	EncodeSuccessCases []encodeSuccessCase
	DecodeSuccessCases []decodeSuccessCase
	EncodeFailureCases []encodeFailureCase
	DecodeFailureCases []decodeFailureCase
}

type encodeSuccessCase struct {
	Name, Context, Value, Bytes string
}

type decodeSuccessCase struct {
	Name, Context, ValueType, Value, Bytes string
}

type encodeFailureCase struct {
	Name, Context, Value, ErrorCode string
}

type decodeFailureCase struct {
	Name, Context, ValueType, Bytes, ErrorCode string
}

// Generate generates Rust tests.
func Generate(wr io.Writer, gidl gidlir.All, fidl fidlir.Root) error {
	encodeSuccessCases, err := encodeSuccessCases(gidl.EncodeSuccess, fidl)
	if err != nil {
		return err
	}
	decodeSuccessCases, err := decodeSuccessCases(gidl.DecodeSuccess, fidl)
	if err != nil {
		return err
	}
	encodeFailureCases, err := encodeFailureCases(gidl.EncodeFailure, fidl)
	if err != nil {
		return err
	}
	decodeFailureCases, err := decodeFailureCases(gidl.DecodeFailure, fidl)
	if err != nil {
		return err
	}
	input := tmplInput{
		EncodeSuccessCases: encodeSuccessCases,
		DecodeSuccessCases: decodeSuccessCases,
		EncodeFailureCases: encodeFailureCases,
		DecodeFailureCases: decodeFailureCases,
	}
	return tmpl.Execute(wr, input)
}

func encodingContext(wireFormat gidlir.WireFormat) string {
	switch wireFormat {
	case gidlir.OldWireFormat:
		return "OLD_CONTEXT"
	case gidlir.V1WireFormat:
		return "V1_CONTEXT"
	default:
		panic(fmt.Sprintf("unexpected wire format %v", wireFormat))
	}
}

func encodeSuccessCases(gidlEncodeSuccesses []gidlir.EncodeSuccess, fidl fidlir.Root) ([]encodeSuccessCase, error) {
	var encodeSuccessCases []encodeSuccessCase
	for _, encodeSuccess := range gidlEncodeSuccesses {
		decl, err := gidlmixer.ExtractDeclaration(encodeSuccess.Value, fidl)
		if err != nil {
			return nil, fmt.Errorf("encode success %s: %s", encodeSuccess.Name, err)
		}
		if gidlir.ContainsUnknownField(encodeSuccess.Value) {
			continue
		}
		encodeSuccessCases = append(encodeSuccessCases, encodeSuccessCase{
			Name:    fidlcommon.ToSnakeCase(encodeSuccess.Name),
			Context: encodingContext(encodeSuccess.WireFormat),
			Value:   visit(encodeSuccess.Value, decl),
			Bytes:   bytesBuilder(encodeSuccess.Bytes),
		})
	}
	return encodeSuccessCases, nil
}

func decodeSuccessCases(gidlDecodeSuccesses []gidlir.DecodeSuccess, fidl fidlir.Root) ([]decodeSuccessCase, error) {
	var decodeSuccessCases []decodeSuccessCase
	for _, decodeSuccess := range gidlDecodeSuccesses {
		decl, err := gidlmixer.ExtractDeclaration(decodeSuccess.Value, fidl)
		if err != nil {
			return nil, fmt.Errorf("decode success %s: %s", decodeSuccess.Name, err)
		}
		if gidlir.ContainsUnknownField(decodeSuccess.Value) {
			continue
		}
		decodeSuccessCases = append(decodeSuccessCases, decodeSuccessCase{
			Name:      fidlcommon.ToSnakeCase(decodeSuccess.Name),
			Context:   encodingContext(decodeSuccess.WireFormat),
			ValueType: rustType(decodeSuccess.Value.(gidlir.Object).Name),
			Value:     visit(decodeSuccess.Value, decl),
			Bytes:     bytesBuilder(decodeSuccess.Bytes),
		})
	}
	return decodeSuccessCases, nil
}

func encodeFailureCases(gidlEncodeFailures []gidlir.EncodeFailure, fidl fidlir.Root) ([]encodeFailureCase, error) {
	var encodeFailureCases []encodeFailureCase
	for _, encodeFailure := range gidlEncodeFailures {
		decl, err := gidlmixer.ExtractDeclarationUnsafe(encodeFailure.Value, fidl)
		if err != nil {
			return nil, fmt.Errorf("encode failure %s: %s", encodeFailure.Name, err)
		}
		if gidlir.ContainsUnknownField(encodeFailure.Value) {
			continue
		}
		errorCode, err := rustErrorCode(encodeFailure.Err)
		if err != nil {
			return nil, err
		}
		encodeFailureCases = append(encodeFailureCases, encodeFailureCase{
			Name:      fidlcommon.ToSnakeCase(encodeFailure.Name),
			Context:   encodingContext(encodeFailure.WireFormat),
			Value:     visit(encodeFailure.Value, decl),
			ErrorCode: errorCode,
		})
	}
	return encodeFailureCases, nil
}

func decodeFailureCases(gidlDecodeFailures []gidlir.DecodeFailure, fidl fidlir.Root) ([]decodeFailureCase, error) {
	var decodeFailureCases []decodeFailureCase
	for _, decodeFailure := range gidlDecodeFailures {
		errorCode, err := rustErrorCode(decodeFailure.Err)
		if err != nil {
			return nil, err
		}
		decodeFailureCases = append(decodeFailureCases, decodeFailureCase{
			Name:      fidlcommon.ToSnakeCase(decodeFailure.Name),
			Context:   encodingContext(decodeFailure.WireFormat),
			ValueType: rustType(decodeFailure.Type),
			Bytes:     bytesBuilder(decodeFailure.Bytes),
			ErrorCode: errorCode,
		})
	}
	return decodeFailureCases, nil
}

// TODO(fxb/39685) extract out to common library
func bytesBuilder(bytes []byte) string {
	var builder strings.Builder
	for i, b := range bytes {
		builder.WriteString(fmt.Sprintf("0x%02x", b))
		builder.WriteString(",")
		if i%8 == 7 {
			builder.WriteString("\n")
		}
	}
	return builder.String()
}

func visit(value interface{}, decl gidlmixer.Declaration) string {
	switch value := value.(type) {
	case bool:
		return strconv.FormatBool(value)
	case int64, uint64, float64:
		suffix := primitiveTypeName(decl.(gidlmixer.PrimitiveDeclaration).Subtype())
		return fmt.Sprintf("%v%s", value, suffix)
	case string:
		// TODO(fxb/39686) Consider Go/Rust escape sequence differences
		return wrapNullable(decl, fmt.Sprintf("String::from(%q)", value))
	case gidlir.Object:
		switch decl := decl.(type) {
		case *gidlmixer.StructDecl:
			return onStruct(value, decl)
		case *gidlmixer.TableDecl:
			return onTable(value, decl)
		case *gidlmixer.UnionDecl:
			return onUnion(value, decl)
		case *gidlmixer.XUnionDecl:
			return onUnion(value, decl)
		}
	case []interface{}:
		switch decl := decl.(type) {
		case *gidlmixer.ArrayDecl:
			return onList(value, decl)
		case *gidlmixer.VectorDecl:
			return onList(value, decl)
		}
	case nil:
		if !decl.IsNullable() {
			panic(fmt.Sprintf("got nil for non-nullable type: %T", decl))
		}
		return "None"
	}
	panic(fmt.Sprintf("unexpected value visited %v of type %T (decl %v)", value, value, decl))
}

func rustType(irType string) string {
	return "conformance::" + fidlcommon.ToUpperCamelCase(irType)
}

func primitiveTypeName(subtype fidlir.PrimitiveSubtype) string {
	switch subtype {
	case fidlir.Bool:
		return "bool"
	case fidlir.Int8:
		return "i8"
	case fidlir.Uint8:
		return "u8"
	case fidlir.Int16:
		return "i16"
	case fidlir.Uint16:
		return "u16"
	case fidlir.Int32:
		return "i32"
	case fidlir.Uint32:
		return "u32"
	case fidlir.Int64:
		return "i64"
	case fidlir.Uint64:
		return "u64"
	case fidlir.Float32:
		return "f32"
	case fidlir.Float64:
		return "f64"
	default:
		panic(fmt.Sprintf("Unexpected subtype %s", string(subtype)))
	}
}

func wrapNullable(decl gidlmixer.Declaration, valueStr string) string {
	if !decl.IsNullable() {
		return valueStr
	}
	switch decl.(type) {
	case *gidlmixer.ArrayDecl, *gidlmixer.VectorDecl, *gidlmixer.StringDecl:
		return fmt.Sprintf("Some(%s)", valueStr)
	case *gidlmixer.StructDecl, *gidlmixer.UnionDecl, *gidlmixer.XUnionDecl:
		return fmt.Sprintf("Some(Box::new(%s))", valueStr)
	case *gidlmixer.BoolDecl, *gidlmixer.NumberDecl, *gidlmixer.FloatDecl, *gidlmixer.TableDecl:
		panic(fmt.Sprintf("decl %v should not be nullable", decl))
	}
	panic(fmt.Sprintf("unexpected decl %v", decl))
}

func onStruct(value gidlir.Object, decl *gidlmixer.StructDecl) string {
	var structFields []string
	providedKeys := make(map[string]struct{}, len(value.Fields))
	for _, field := range value.Fields {
		if field.Key.Name == "" {
			panic("unknown field not supported")
		}
		providedKeys[field.Key.Name] = struct{}{}
		fieldName := fidlcommon.ToSnakeCase(field.Key.Name)
		fieldDecl, _ := decl.ForKey(field.Key)
		fieldValueStr := visit(field.Value, fieldDecl)
		structFields = append(structFields, fmt.Sprintf("%s: %s", fieldName, fieldValueStr))
	}
	for _, member := range decl.Members {
		key := string(member.Name)
		if _, ok := providedKeys[key]; !ok {
			if !member.Type.Nullable {
				panic(fmt.Sprintf("omitted field %s of struct %s is not nullable", member.Name, value.Name))
			}
			fieldName := fidlcommon.ToSnakeCase(key)
			structFields = append(structFields, fmt.Sprintf("%s: None", fieldName))
		}
	}
	structName := rustType(value.Name)
	valueStr := fmt.Sprintf("%s { %s }", structName, strings.Join(structFields, ", "))
	return wrapNullable(decl, valueStr)
}

func onTable(value gidlir.Object, decl *gidlmixer.TableDecl) string {
	var tableFields []string
	providedKeys := make(map[string]struct{}, len(value.Fields))
	for _, field := range value.Fields {
		if field.Key.Name == "" {
			panic("unknown field not supported")
		}
		providedKeys[field.Key.Name] = struct{}{}
		fieldName := fidlcommon.ToSnakeCase(field.Key.Name)
		fieldDecl, _ := decl.ForKey(field.Key)
		fieldValueStr := visit(field.Value, fieldDecl)
		tableFields = append(tableFields, fmt.Sprintf("%s: Some(%s)", fieldName, fieldValueStr))
	}
	for _, member := range decl.Members {
		if member.Reserved {
			continue
		}
		key := string(member.Name)
		if _, ok := providedKeys[key]; !ok {
			fieldName := fidlcommon.ToSnakeCase(key)
			tableFields = append(tableFields, fmt.Sprintf("%s: None", fieldName))
		}
	}
	tableName := rustType(value.Name)
	valueStr := fmt.Sprintf("%s { %s }", tableName, strings.Join(tableFields, ", "))
	return wrapNullable(decl, valueStr)
}

func onUnion(value gidlir.Object, decl gidlmixer.KeyedDeclaration) string {
	if len(value.Fields) != 1 {
		panic(fmt.Sprintf("union has %d fields, expected 1", len(value.Fields)))
	}
	field := value.Fields[0]
	if field.Key.Name == "" {
		panic("unknown field not supported")
	}
	unionName := rustType(value.Name)
	fieldName := fidlcommon.ToUpperCamelCase(field.Key.Name)
	fieldDecl, _ := decl.ForKey(field.Key)
	fieldValueStr := visit(field.Value, fieldDecl)
	valueStr := fmt.Sprintf("%s::%s(%s)", unionName, fieldName, fieldValueStr)
	return wrapNullable(decl, valueStr)
}

func onList(value []interface{}, decl gidlmixer.ListDeclaration) string {
	var elements []string
	elemDecl, _ := decl.Elem()
	for _, item := range value {
		elements = append(elements, visit(item, elemDecl))
	}
	elementsStr := strings.Join(elements, ", ")
	switch decl.(type) {
	case *gidlmixer.ArrayDecl:
		return fmt.Sprintf("[%s]", elementsStr)
	case *gidlmixer.VectorDecl:
		return fmt.Sprintf("vec![%s]", elementsStr)
	}
	panic(fmt.Sprintf("unexpected decl %v", decl))
}

// Rust errors are defined in garnet/public/lib/fidl/rust/fidl/src/error.rs
var rustErrorCodeNames = map[gidlir.ErrorCode]string{
	gidlir.StringTooLong:              "OutOfRange",
	gidlir.NonEmptyStringWithNullBody: "UnexpectedNullRef",
	gidlir.StrictXUnionFieldNotSet:    "UnknownUnionTag",
	gidlir.StrictXUnionUnknownField:   "UnknownUnionTag",
}

func rustErrorCode(code gidlir.ErrorCode) (string, error) {
	if str, ok := rustErrorCodeNames[code]; ok {
		return fmt.Sprintf("Error::%s", str), nil
	}
	return "", fmt.Errorf("no rust error string defined for error code %s", code)
}
