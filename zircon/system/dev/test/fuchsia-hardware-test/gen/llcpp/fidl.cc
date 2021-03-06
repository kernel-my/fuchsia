// WARNING: This file is machine generated by fidlgen.

#include <fuchsia/hardware/test/llcpp/fidl.h>
#include <memory>

namespace llcpp {

namespace fuchsia {
namespace hardware {
namespace test {

namespace {

[[maybe_unused]]
constexpr uint64_t kDevice_GetChannel_Ordinal = 0x3ee7275100000000lu;
[[maybe_unused]]
constexpr uint64_t kDevice_GetChannel_GenOrdinal = 0x3427354841fda60alu;
extern "C" const fidl_type_t fuchsia_hardware_test_DeviceGetChannelRequestTable;
extern "C" const fidl_type_t fuchsia_hardware_test_DeviceGetChannelResponseTable;
extern "C" const fidl_type_t v1_fuchsia_hardware_test_DeviceGetChannelResponseTable;

}  // namespace
template <>
Device::ResultOf::GetChannel_Impl<Device::GetChannelResponse>::GetChannel_Impl(zx::unowned_channel _client_end) {
  constexpr uint32_t _kWriteAllocSize = ::fidl::internal::ClampedMessageSize<GetChannelRequest, ::fidl::MessageDirection::kSending>();
  ::fidl::internal::AlignedBuffer<_kWriteAllocSize> _write_bytes_inlined;
  auto& _write_bytes_array = _write_bytes_inlined;
  uint8_t* _write_bytes = _write_bytes_array.view().data();
  memset(_write_bytes, 0, GetChannelRequest::PrimarySize);
  ::fidl::BytePart _request_bytes(_write_bytes, _kWriteAllocSize, sizeof(GetChannelRequest));
  ::fidl::DecodedMessage<GetChannelRequest> _decoded_request(std::move(_request_bytes));
  Super::SetResult(
      Device::InPlace::GetChannel(std::move(_client_end), Super::response_buffer()));
}

Device::ResultOf::GetChannel Device::SyncClient::GetChannel() {
  return ResultOf::GetChannel(zx::unowned_channel(this->channel_));
}

Device::ResultOf::GetChannel Device::Call::GetChannel(zx::unowned_channel _client_end) {
  return ResultOf::GetChannel(std::move(_client_end));
}

template <>
Device::UnownedResultOf::GetChannel_Impl<Device::GetChannelResponse>::GetChannel_Impl(zx::unowned_channel _client_end, ::fidl::BytePart _response_buffer) {
  FIDL_ALIGNDECL uint8_t _write_bytes[sizeof(GetChannelRequest)] = {};
  ::fidl::BytePart _request_buffer(_write_bytes, sizeof(_write_bytes));
  memset(_request_buffer.data(), 0, GetChannelRequest::PrimarySize);
  _request_buffer.set_actual(sizeof(GetChannelRequest));
  ::fidl::DecodedMessage<GetChannelRequest> _decoded_request(std::move(_request_buffer));
  Super::SetResult(
      Device::InPlace::GetChannel(std::move(_client_end), std::move(_response_buffer)));
}

Device::UnownedResultOf::GetChannel Device::SyncClient::GetChannel(::fidl::BytePart _response_buffer) {
  return UnownedResultOf::GetChannel(zx::unowned_channel(this->channel_), std::move(_response_buffer));
}

Device::UnownedResultOf::GetChannel Device::Call::GetChannel(zx::unowned_channel _client_end, ::fidl::BytePart _response_buffer) {
  return UnownedResultOf::GetChannel(std::move(_client_end), std::move(_response_buffer));
}

::fidl::DecodeResult<Device::GetChannelResponse> Device::InPlace::GetChannel(zx::unowned_channel _client_end, ::fidl::BytePart response_buffer) {
  constexpr uint32_t _write_num_bytes = sizeof(GetChannelRequest);
  ::fidl::internal::AlignedBuffer<_write_num_bytes> _write_bytes;
  ::fidl::BytePart _request_buffer = _write_bytes.view();
  _request_buffer.set_actual(_write_num_bytes);
  ::fidl::DecodedMessage<GetChannelRequest> params(std::move(_request_buffer));
  Device::SetTransactionHeaderFor::GetChannelRequest(params);
  auto _encode_request_result = ::fidl::Encode(std::move(params));
  if (_encode_request_result.status != ZX_OK) {
    return ::fidl::DecodeResult<Device::GetChannelResponse>::FromFailure(
        std::move(_encode_request_result));
  }
  auto _call_result = ::fidl::Call<GetChannelRequest, GetChannelResponse>(
    std::move(_client_end), std::move(_encode_request_result.message), std::move(response_buffer));
  if (_call_result.status != ZX_OK) {
    return ::fidl::DecodeResult<Device::GetChannelResponse>::FromFailure(
        std::move(_call_result));
  }
  return ::fidl::Decode(std::move(_call_result.message));
}


bool Device::TryDispatch(Interface* impl, fidl_msg_t* msg, ::fidl::Transaction* txn) {
  if (msg->num_bytes < sizeof(fidl_message_header_t)) {
    zx_handle_close_many(msg->handles, msg->num_handles);
    txn->Close(ZX_ERR_INVALID_ARGS);
    return true;
  }
  fidl_message_header_t* hdr = reinterpret_cast<fidl_message_header_t*>(msg->bytes);
  switch (hdr->ordinal) {
    case kDevice_GetChannel_Ordinal:
    case kDevice_GetChannel_GenOrdinal:
    {
      auto result = ::fidl::DecodeAs<GetChannelRequest>(msg);
      if (result.status != ZX_OK) {
        txn->Close(ZX_ERR_INVALID_ARGS);
        return true;
      }
      impl->GetChannel(
          Interface::GetChannelCompleter::Sync(txn));
      return true;
    }
    default: {
      return false;
    }
  }
}

bool Device::Dispatch(Interface* impl, fidl_msg_t* msg, ::fidl::Transaction* txn) {
  bool found = TryDispatch(impl, msg, txn);
  if (!found) {
    zx_handle_close_many(msg->handles, msg->num_handles);
    txn->Close(ZX_ERR_NOT_SUPPORTED);
  }
  return found;
}


void Device::Interface::GetChannelCompleterBase::Reply(::zx::channel ch) {
  constexpr uint32_t _kWriteAllocSize = ::fidl::internal::ClampedMessageSize<GetChannelResponse, ::fidl::MessageDirection::kSending>();
  FIDL_ALIGNDECL uint8_t _write_bytes[_kWriteAllocSize] = {};
  auto& _response = *reinterpret_cast<GetChannelResponse*>(_write_bytes);
  Device::SetTransactionHeaderFor::GetChannelResponse(
      ::fidl::DecodedMessage<GetChannelResponse>(
          ::fidl::BytePart(reinterpret_cast<uint8_t*>(&_response),
              GetChannelResponse::PrimarySize,
              GetChannelResponse::PrimarySize)));
  _response.ch = std::move(ch);
  ::fidl::BytePart _response_bytes(_write_bytes, _kWriteAllocSize, sizeof(GetChannelResponse));
  CompleterBase::SendReply(::fidl::DecodedMessage<GetChannelResponse>(std::move(_response_bytes)));
}

void Device::Interface::GetChannelCompleterBase::Reply(::fidl::BytePart _buffer, ::zx::channel ch) {
  if (_buffer.capacity() < GetChannelResponse::PrimarySize) {
    CompleterBase::Close(ZX_ERR_INTERNAL);
    return;
  }
  auto& _response = *reinterpret_cast<GetChannelResponse*>(_buffer.data());
  Device::SetTransactionHeaderFor::GetChannelResponse(
      ::fidl::DecodedMessage<GetChannelResponse>(
          ::fidl::BytePart(reinterpret_cast<uint8_t*>(&_response),
              GetChannelResponse::PrimarySize,
              GetChannelResponse::PrimarySize)));
  _response.ch = std::move(ch);
  _buffer.set_actual(sizeof(GetChannelResponse));
  CompleterBase::SendReply(::fidl::DecodedMessage<GetChannelResponse>(std::move(_buffer)));
}

void Device::Interface::GetChannelCompleterBase::Reply(::fidl::DecodedMessage<GetChannelResponse> params) {
  Device::SetTransactionHeaderFor::GetChannelResponse(params);
  CompleterBase::SendReply(std::move(params));
}



void Device::SetTransactionHeaderFor::GetChannelRequest(const ::fidl::DecodedMessage<Device::GetChannelRequest>& _msg) {
  fidl_init_txn_header(&_msg.message()->_hdr, 0, kDevice_GetChannel_Ordinal);
}
void Device::SetTransactionHeaderFor::GetChannelResponse(const ::fidl::DecodedMessage<Device::GetChannelResponse>& _msg) {
  fidl_init_txn_header(&_msg.message()->_hdr, 0, kDevice_GetChannel_Ordinal);
}

}  // namespace test
}  // namespace hardware
}  // namespace fuchsia
}  // namespace llcpp
