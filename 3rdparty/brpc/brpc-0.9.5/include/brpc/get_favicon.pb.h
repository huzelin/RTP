// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: brpc/get_favicon.proto

#ifndef PROTOBUF_INCLUDED_brpc_2fget_5ffavicon_2eproto
#define PROTOBUF_INCLUDED_brpc_2fget_5ffavicon_2eproto

#include <string>

#include <google/protobuf/stubs/common.h>

#if GOOGLE_PROTOBUF_VERSION < 3006000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please update
#error your headers.
#endif
#if 3006000 < GOOGLE_PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/arena.h>
#include <google/protobuf/arenastring.h>
#include <google/protobuf/generated_message_table_driven.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/inlined_string_field.h>
#include <google/protobuf/metadata.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>  // IWYU pragma: export
#include <google/protobuf/extension_set.h>  // IWYU pragma: export
#include <google/protobuf/service.h>
#include <google/protobuf/unknown_field_set.h>
// @@protoc_insertion_point(includes)
#define PROTOBUF_INTERNAL_EXPORT_protobuf_brpc_2fget_5ffavicon_2eproto 

namespace protobuf_brpc_2fget_5ffavicon_2eproto {
// Internal implementation detail -- do not use these members.
struct TableStruct {
  static const ::google::protobuf::internal::ParseTableField entries[];
  static const ::google::protobuf::internal::AuxillaryParseTableField aux[];
  static const ::google::protobuf::internal::ParseTable schema[2];
  static const ::google::protobuf::internal::FieldMetadata field_metadata[];
  static const ::google::protobuf::internal::SerializationTable serialization_table[];
  static const ::google::protobuf::uint32 offsets[];
};
void AddDescriptors();
}  // namespace protobuf_brpc_2fget_5ffavicon_2eproto
namespace brpc {
class GetFaviconRequest;
class GetFaviconRequestDefaultTypeInternal;
extern GetFaviconRequestDefaultTypeInternal _GetFaviconRequest_default_instance_;
class GetFaviconResponse;
class GetFaviconResponseDefaultTypeInternal;
extern GetFaviconResponseDefaultTypeInternal _GetFaviconResponse_default_instance_;
}  // namespace brpc
namespace google {
namespace protobuf {
template<> ::brpc::GetFaviconRequest* Arena::CreateMaybeMessage<::brpc::GetFaviconRequest>(Arena*);
template<> ::brpc::GetFaviconResponse* Arena::CreateMaybeMessage<::brpc::GetFaviconResponse>(Arena*);
}  // namespace protobuf
}  // namespace google
namespace brpc {

// ===================================================================

class GetFaviconRequest : public ::google::protobuf::Message /* @@protoc_insertion_point(class_definition:brpc.GetFaviconRequest) */ {
 public:
  GetFaviconRequest();
  virtual ~GetFaviconRequest();

  GetFaviconRequest(const GetFaviconRequest& from);

  inline GetFaviconRequest& operator=(const GetFaviconRequest& from) {
    CopyFrom(from);
    return *this;
  }
  #if LANG_CXX11
  GetFaviconRequest(GetFaviconRequest&& from) noexcept
    : GetFaviconRequest() {
    *this = ::std::move(from);
  }

  inline GetFaviconRequest& operator=(GetFaviconRequest&& from) noexcept {
    if (GetArenaNoVirtual() == from.GetArenaNoVirtual()) {
      if (this != &from) InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }
  #endif
  inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const {
    return _internal_metadata_.unknown_fields();
  }
  inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields() {
    return _internal_metadata_.mutable_unknown_fields();
  }

  static const ::google::protobuf::Descriptor* descriptor();
  static const GetFaviconRequest& default_instance();

  static void InitAsDefaultInstance();  // FOR INTERNAL USE ONLY
  static inline const GetFaviconRequest* internal_default_instance() {
    return reinterpret_cast<const GetFaviconRequest*>(
               &_GetFaviconRequest_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    0;

  void Swap(GetFaviconRequest* other);
  friend void swap(GetFaviconRequest& a, GetFaviconRequest& b) {
    a.Swap(&b);
  }

  // implements Message ----------------------------------------------

  inline GetFaviconRequest* New() const final {
    return CreateMaybeMessage<GetFaviconRequest>(NULL);
  }

  GetFaviconRequest* New(::google::protobuf::Arena* arena) const final {
    return CreateMaybeMessage<GetFaviconRequest>(arena);
  }
  void CopyFrom(const ::google::protobuf::Message& from) final;
  void MergeFrom(const ::google::protobuf::Message& from) final;
  void CopyFrom(const GetFaviconRequest& from);
  void MergeFrom(const GetFaviconRequest& from);
  void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input) final;
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const final;
  ::google::protobuf::uint8* InternalSerializeWithCachedSizesToArray(
      bool deterministic, ::google::protobuf::uint8* target) const final;
  int GetCachedSize() const final { return _cached_size_.Get(); }

  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(GetFaviconRequest* other);
  private:
  inline ::google::protobuf::Arena* GetArenaNoVirtual() const {
    return NULL;
  }
  inline void* MaybeArenaPtr() const {
    return NULL;
  }
  public:

  ::google::protobuf::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // @@protoc_insertion_point(class_scope:brpc.GetFaviconRequest)
 private:

  ::google::protobuf::internal::InternalMetadataWithArena _internal_metadata_;
  ::google::protobuf::internal::HasBits<1> _has_bits_;
  mutable ::google::protobuf::internal::CachedSize _cached_size_;
  friend struct ::protobuf_brpc_2fget_5ffavicon_2eproto::TableStruct;
};
// -------------------------------------------------------------------

class GetFaviconResponse : public ::google::protobuf::Message /* @@protoc_insertion_point(class_definition:brpc.GetFaviconResponse) */ {
 public:
  GetFaviconResponse();
  virtual ~GetFaviconResponse();

  GetFaviconResponse(const GetFaviconResponse& from);

  inline GetFaviconResponse& operator=(const GetFaviconResponse& from) {
    CopyFrom(from);
    return *this;
  }
  #if LANG_CXX11
  GetFaviconResponse(GetFaviconResponse&& from) noexcept
    : GetFaviconResponse() {
    *this = ::std::move(from);
  }

  inline GetFaviconResponse& operator=(GetFaviconResponse&& from) noexcept {
    if (GetArenaNoVirtual() == from.GetArenaNoVirtual()) {
      if (this != &from) InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }
  #endif
  inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const {
    return _internal_metadata_.unknown_fields();
  }
  inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields() {
    return _internal_metadata_.mutable_unknown_fields();
  }

  static const ::google::protobuf::Descriptor* descriptor();
  static const GetFaviconResponse& default_instance();

  static void InitAsDefaultInstance();  // FOR INTERNAL USE ONLY
  static inline const GetFaviconResponse* internal_default_instance() {
    return reinterpret_cast<const GetFaviconResponse*>(
               &_GetFaviconResponse_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    1;

  void Swap(GetFaviconResponse* other);
  friend void swap(GetFaviconResponse& a, GetFaviconResponse& b) {
    a.Swap(&b);
  }

  // implements Message ----------------------------------------------

  inline GetFaviconResponse* New() const final {
    return CreateMaybeMessage<GetFaviconResponse>(NULL);
  }

  GetFaviconResponse* New(::google::protobuf::Arena* arena) const final {
    return CreateMaybeMessage<GetFaviconResponse>(arena);
  }
  void CopyFrom(const ::google::protobuf::Message& from) final;
  void MergeFrom(const ::google::protobuf::Message& from) final;
  void CopyFrom(const GetFaviconResponse& from);
  void MergeFrom(const GetFaviconResponse& from);
  void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input) final;
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const final;
  ::google::protobuf::uint8* InternalSerializeWithCachedSizesToArray(
      bool deterministic, ::google::protobuf::uint8* target) const final;
  int GetCachedSize() const final { return _cached_size_.Get(); }

  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(GetFaviconResponse* other);
  private:
  inline ::google::protobuf::Arena* GetArenaNoVirtual() const {
    return NULL;
  }
  inline void* MaybeArenaPtr() const {
    return NULL;
  }
  public:

  ::google::protobuf::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // @@protoc_insertion_point(class_scope:brpc.GetFaviconResponse)
 private:

  ::google::protobuf::internal::InternalMetadataWithArena _internal_metadata_;
  ::google::protobuf::internal::HasBits<1> _has_bits_;
  mutable ::google::protobuf::internal::CachedSize _cached_size_;
  friend struct ::protobuf_brpc_2fget_5ffavicon_2eproto::TableStruct;
};
// ===================================================================

class ico_Stub;

class ico : public ::google::protobuf::Service {
 protected:
  // This class should be treated as an abstract interface.
  inline ico() {};
 public:
  virtual ~ico();

  typedef ico_Stub Stub;

  static const ::google::protobuf::ServiceDescriptor* descriptor();

  virtual void default_method(::google::protobuf::RpcController* controller,
                       const ::brpc::GetFaviconRequest* request,
                       ::brpc::GetFaviconResponse* response,
                       ::google::protobuf::Closure* done);

  // implements Service ----------------------------------------------

  const ::google::protobuf::ServiceDescriptor* GetDescriptor();
  void CallMethod(const ::google::protobuf::MethodDescriptor* method,
                  ::google::protobuf::RpcController* controller,
                  const ::google::protobuf::Message* request,
                  ::google::protobuf::Message* response,
                  ::google::protobuf::Closure* done);
  const ::google::protobuf::Message& GetRequestPrototype(
    const ::google::protobuf::MethodDescriptor* method) const;
  const ::google::protobuf::Message& GetResponsePrototype(
    const ::google::protobuf::MethodDescriptor* method) const;

 private:
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(ico);
};

class ico_Stub : public ico {
 public:
  ico_Stub(::google::protobuf::RpcChannel* channel);
  ico_Stub(::google::protobuf::RpcChannel* channel,
                   ::google::protobuf::Service::ChannelOwnership ownership);
  ~ico_Stub();

  inline ::google::protobuf::RpcChannel* channel() { return channel_; }

  // implements ico ------------------------------------------

  void default_method(::google::protobuf::RpcController* controller,
                       const ::brpc::GetFaviconRequest* request,
                       ::brpc::GetFaviconResponse* response,
                       ::google::protobuf::Closure* done);
 private:
  ::google::protobuf::RpcChannel* channel_;
  bool owns_channel_;
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(ico_Stub);
};


// ===================================================================


// ===================================================================

#ifdef __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif  // __GNUC__
// GetFaviconRequest

// -------------------------------------------------------------------

// GetFaviconResponse

#ifdef __GNUC__
  #pragma GCC diagnostic pop
#endif  // __GNUC__
// -------------------------------------------------------------------


// @@protoc_insertion_point(namespace_scope)

}  // namespace brpc

// @@protoc_insertion_point(global_scope)

#endif  // PROTOBUF_INCLUDED_brpc_2fget_5ffavicon_2eproto
