// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_DEVELOPER_DEBUG_ZXDB_SYMBOLS_LAZY_SYMBOL_H_
#define SRC_DEVELOPER_DEBUG_ZXDB_SYMBOLS_LAZY_SYMBOL_H_

#include <stdint.h>

#include <memory>

#include "src/developer/debug/zxdb/symbols/symbol_factory.h"
#include "src/lib/fxl/memory/ref_ptr.h"

namespace zxdb {

class Symbol;

// Symbols can be complex and in many cases are not required. This class holds enough information to
// construct a type from the symbol file as needed. Once constructed, it will cache the type for
// future use.
class LazySymbol {
 public:
  LazySymbol();  // Creates a !is_valid() one.
  LazySymbol(const LazySymbol& other);
  LazySymbol(LazySymbol&& other);
  LazySymbol(fxl::RefPtr<SymbolFactory> factory, uint32_t factory_data);

  // Implicitly creates a non-lazy one with a pre-cooked object, mostly for tests.
  template <class SymbolType>
  LazySymbol(fxl::RefPtr<SymbolType> symbol) : symbol_(symbol) {}
  LazySymbol(const Symbol* symbol);

  ~LazySymbol();

  LazySymbol& operator=(const LazySymbol& other);
  LazySymbol& operator=(LazySymbol&& other);

  // Validity tests both for the factory and the symbol since non-lazy ones don't need a factory.
  bool is_valid() const { return factory_.get() || symbol_.get(); }
  explicit operator bool() const { return is_valid(); }

  // Returns the type associated with this LazySymbol. If this class is invalid or the symbol fails
  // to resolve this will return an empty one. It will never return null.
  const Symbol* Get() const;

 private:
  // May be null if this contains no type reference.
  fxl::RefPtr<SymbolFactory> factory_;

  // Opaque data passed to the factory to construct a type Symbol for this. In the DWARF factory,
  // this is a DIE offset.
  uint32_t factory_data_ = 0;

  mutable fxl::RefPtr<Symbol> symbol_;
};

}  // namespace zxdb

#endif  // SRC_DEVELOPER_DEBUG_ZXDB_SYMBOLS_LAZY_SYMBOL_H_
