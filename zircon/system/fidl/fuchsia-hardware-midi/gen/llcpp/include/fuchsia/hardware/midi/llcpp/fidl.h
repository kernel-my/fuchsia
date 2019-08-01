// WARNING: This file is machine generated by fidlgen.

#pragma once

#include <lib/fidl/internal.h>
#include <lib/fidl/cpp/vector_view.h>
#include <lib/fidl/cpp/string_view.h>
#include <lib/fidl/llcpp/array.h>
#include <lib/fidl/llcpp/coding.h>
#include <lib/fidl/llcpp/sync_call.h>
#include <lib/fidl/llcpp/traits.h>
#include <lib/fidl/llcpp/transaction.h>
#include <lib/fit/function.h>
#include <lib/zx/channel.h>
#include <zircon/fidl.h>

namespace llcpp {

namespace fuchsia {
namespace hardware {
namespace midi {

struct Info;
class Device;



// Describes what type of MIDI device an implementation of Device represents
struct Info {
  static constexpr const fidl_type_t* Type = nullptr;
  static constexpr uint32_t MaxNumHandles = 0;
  static constexpr uint32_t PrimarySize = 2;
  [[maybe_unused]]
  static constexpr uint32_t MaxOutOfLine = 0;

  // Whether or not this device is a MIDI sink
  bool is_sink = {};

  // Whether or not this device is a MIDI source
  bool is_source = {};
};

extern "C" const fidl_type_t fuchsia_hardware_midi_DeviceGetInfoResponseTable;

class Device final {
  Device() = delete;
 public:

  struct GetInfoResponse final {
    FIDL_ALIGNDECL
    fidl_message_header_t _hdr;
    Info info;

    static constexpr const fidl_type_t* Type = &fuchsia_hardware_midi_DeviceGetInfoResponseTable;
    static constexpr uint32_t MaxNumHandles = 0;
    static constexpr uint32_t PrimarySize = 24;
    static constexpr uint32_t MaxOutOfLine = 0;
  };
  using GetInfoRequest = ::fidl::AnyZeroArgMessage;


  // Collection of return types of FIDL calls in this interface.
  class ResultOf final {
    ResultOf() = delete;
   private:
    template <typename ResponseType>
    class GetInfo_Impl final : private ::fidl::internal::OwnedSyncCallBase<ResponseType> {
      using Super = ::fidl::internal::OwnedSyncCallBase<ResponseType>;
     public:
      GetInfo_Impl(zx::unowned_channel _client_end);
      ~GetInfo_Impl() = default;
      GetInfo_Impl(GetInfo_Impl&& other) = default;
      GetInfo_Impl& operator=(GetInfo_Impl&& other) = default;
      using Super::status;
      using Super::error;
      using Super::ok;
      using Super::Unwrap;
      using Super::value;
    };

   public:
    using GetInfo = GetInfo_Impl<GetInfoResponse>;
  };

  // Collection of return types of FIDL calls in this interface,
  // when the caller-allocate flavor or in-place call is used.
  class UnownedResultOf final {
    UnownedResultOf() = delete;
   private:
    template <typename ResponseType>
    class GetInfo_Impl final : private ::fidl::internal::UnownedSyncCallBase<ResponseType> {
      using Super = ::fidl::internal::UnownedSyncCallBase<ResponseType>;
     public:
      GetInfo_Impl(zx::unowned_channel _client_end, ::fidl::BytePart _response_buffer);
      ~GetInfo_Impl() = default;
      GetInfo_Impl(GetInfo_Impl&& other) = default;
      GetInfo_Impl& operator=(GetInfo_Impl&& other) = default;
      using Super::status;
      using Super::error;
      using Super::ok;
      using Super::Unwrap;
      using Super::value;
    };

   public:
    using GetInfo = GetInfo_Impl<GetInfoResponse>;
  };

  class SyncClient final {
   public:
    explicit SyncClient(::zx::channel channel) : channel_(std::move(channel)) {}
    ~SyncClient() = default;
    SyncClient(SyncClient&&) = default;
    SyncClient& operator=(SyncClient&&) = default;

    const ::zx::channel& channel() const { return channel_; }

    ::zx::channel* mutable_channel() { return &channel_; }

    // Get information about the type of MIDI device
    // Allocates 40 bytes of message buffer on the stack. No heap allocation necessary.
    ResultOf::GetInfo GetInfo();

    // Get information about the type of MIDI device
    // Caller provides the backing storage for FIDL message via request and response buffers.
    UnownedResultOf::GetInfo GetInfo(::fidl::BytePart _response_buffer);

    // Get information about the type of MIDI device
    zx_status_t GetInfo_Deprecated(Info* out_info);

    // Get information about the type of MIDI device
    // Caller provides the backing storage for FIDL message via request and response buffers.
    // The lifetime of handles in the response, unless moved, is tied to the returned RAII object.
    ::fidl::DecodeResult<GetInfoResponse> GetInfo_Deprecated(::fidl::BytePart _response_buffer, Info* out_info);

   private:
    ::zx::channel channel_;
  };

  // Methods to make a sync FIDL call directly on an unowned channel, avoiding setting up a client.
  class Call final {
    Call() = delete;
   public:

    // Get information about the type of MIDI device
    // Allocates 40 bytes of message buffer on the stack. No heap allocation necessary.
    static ResultOf::GetInfo GetInfo(zx::unowned_channel _client_end);

    // Get information about the type of MIDI device
    // Caller provides the backing storage for FIDL message via request and response buffers.
    static UnownedResultOf::GetInfo GetInfo(zx::unowned_channel _client_end, ::fidl::BytePart _response_buffer);

    // Get information about the type of MIDI device
    static zx_status_t GetInfo_Deprecated(zx::unowned_channel _client_end, Info* out_info);

    // Get information about the type of MIDI device
    // Caller provides the backing storage for FIDL message via request and response buffers.
    // The lifetime of handles in the response, unless moved, is tied to the returned RAII object.
    static ::fidl::DecodeResult<GetInfoResponse> GetInfo_Deprecated(zx::unowned_channel _client_end, ::fidl::BytePart _response_buffer, Info* out_info);

  };

  // Messages are encoded and decoded in-place when these methods are used.
  // Additionally, requests must be already laid-out according to the FIDL wire-format.
  class InPlace final {
    InPlace() = delete;
   public:

    // Get information about the type of MIDI device
    static ::fidl::DecodeResult<GetInfoResponse> GetInfo(zx::unowned_channel _client_end, ::fidl::BytePart response_buffer);

  };

  // Pure-virtual interface to be implemented by a server.
  class Interface {
   public:
    Interface() = default;
    virtual ~Interface() = default;
    using _Outer = Device;
    using _Base = ::fidl::CompleterBase;

    class GetInfoCompleterBase : public _Base {
     public:
      void Reply(Info info);
      void Reply(::fidl::BytePart _buffer, Info info);
      void Reply(::fidl::DecodedMessage<GetInfoResponse> params);

     protected:
      using ::fidl::CompleterBase::CompleterBase;
    };

    using GetInfoCompleter = ::fidl::Completer<GetInfoCompleterBase>;

    virtual void GetInfo(GetInfoCompleter::Sync _completer) = 0;

  };

  // Attempts to dispatch the incoming message to a handler function in the server implementation.
  // If there is no matching handler, it returns false, leaving the message and transaction intact.
  // In all other cases, it consumes the message and returns true.
  // It is possible to chain multiple TryDispatch functions in this manner.
  static bool TryDispatch(Interface* impl, fidl_msg_t* msg, ::fidl::Transaction* txn);

  // Dispatches the incoming message to one of the handlers functions in the interface.
  // If there is no matching handler, it closes all the handles in |msg| and closes the channel with
  // a |ZX_ERR_NOT_SUPPORTED| epitaph, before returning false. The message should then be discarded.
  static bool Dispatch(Interface* impl, fidl_msg_t* msg, ::fidl::Transaction* txn);

  // Same as |Dispatch|, but takes a |void*| instead of |Interface*|. Only used with |fidl::Bind|
  // to reduce template expansion.
  // Do not call this method manually. Use |Dispatch| instead.
  static bool TypeErasedDispatch(void* impl, fidl_msg_t* msg, ::fidl::Transaction* txn) {
    return Dispatch(static_cast<Interface*>(impl), msg, txn);
  }

};

}  // namespace midi
}  // namespace hardware
}  // namespace fuchsia
}  // namespace llcpp

namespace fidl {

template <>
struct IsFidlType<::llcpp::fuchsia::hardware::midi::Info> : public std::true_type {};
static_assert(std::is_standard_layout_v<::llcpp::fuchsia::hardware::midi::Info>);
static_assert(offsetof(::llcpp::fuchsia::hardware::midi::Info, is_sink) == 0);
static_assert(offsetof(::llcpp::fuchsia::hardware::midi::Info, is_source) == 1);
static_assert(sizeof(::llcpp::fuchsia::hardware::midi::Info) == ::llcpp::fuchsia::hardware::midi::Info::PrimarySize);

template <>
struct IsFidlType<::llcpp::fuchsia::hardware::midi::Device::GetInfoResponse> : public std::true_type {};
template <>
struct IsFidlMessage<::llcpp::fuchsia::hardware::midi::Device::GetInfoResponse> : public std::true_type {};
static_assert(sizeof(::llcpp::fuchsia::hardware::midi::Device::GetInfoResponse)
    == ::llcpp::fuchsia::hardware::midi::Device::GetInfoResponse::PrimarySize);
static_assert(offsetof(::llcpp::fuchsia::hardware::midi::Device::GetInfoResponse, info) == 16);

}  // namespace fidl
