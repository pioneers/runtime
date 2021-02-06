/* Generated by the protocol buffer compiler.  DO NOT EDIT! */
/* Generated from: device.proto */

#ifndef PROTOBUF_C_device_2eproto__INCLUDED
#define PROTOBUF_C_device_2eproto__INCLUDED

#include <protobuf-c/protobuf-c.h>

PROTOBUF_C__BEGIN_DECLS

#if PROTOBUF_C_VERSION_NUMBER < 1003000
#error This file was generated by a newer version of protoc-c which is incompatible with your libprotobuf-c headers. Please update your headers.
#elif 1003003 < PROTOBUF_C_MIN_COMPILER_VERSION
#error This file was generated by an older version of protoc-c which is incompatible with your libprotobuf-c headers. Please regenerate this file with a newer version of protoc-c.
#endif


typedef struct _Param Param;
typedef struct _Device Device;
typedef struct _DevData DevData;


/* --- enums --- */


/* --- messages --- */

typedef enum {
    PARAM__VAL__NOT_SET = 0,
    PARAM__VAL_FVAL = 2,
    PARAM__VAL_IVAL = 3,
    PARAM__VAL_BVAL = 4 PROTOBUF_C__FORCE_ENUM_TO_BE_INT_SIZE(PARAM__VAL)
} Param__ValCase;

/*
 *message for describing a single device parameter
 */
struct _Param {
    ProtobufCMessage base;
    char* name;
    protobuf_c_boolean readonly;
    Param__ValCase val_case;
    union {
        float fval;
        int32_t ival;
        protobuf_c_boolean bval;
    };
};
#define PARAM__INIT                                                      \
    {                                                                    \
        PROTOBUF_C_MESSAGE_INIT(&param__descriptor)                      \
        , (char*) protobuf_c_empty_string, 0, PARAM__VAL__NOT_SET, { 0 } \
    }


/*
 *message for describing a single device
 */
struct _Device {
    ProtobufCMessage base;
    char* name;
    uint64_t uid;
    uint32_t type;
    /*
   *each device has some number of params
   */
    size_t n_params;
    Param** params;
};
#define DEVICE__INIT                                     \
    {                                                    \
        PROTOBUF_C_MESSAGE_INIT(&device__descriptor)     \
        , (char*) protobuf_c_empty_string, 0, 0, 0, NULL \
    }


struct _DevData {
    ProtobufCMessage base;
    /*
   *this single field has information about all requested params of devices
   */
    size_t n_devices;
    Device** devices;
};
#define DEV_DATA__INIT                                 \
    {                                                  \
        PROTOBUF_C_MESSAGE_INIT(&dev_data__descriptor) \
        , 0, NULL                                      \
    }


/* Param methods */
void param__init(Param* message);
size_t param__get_packed_size(const Param* message);
size_t param__pack(const Param* message,
                   uint8_t* out);
size_t param__pack_to_buffer(const Param* message,
                             ProtobufCBuffer* buffer);
Param*
param__unpack(ProtobufCAllocator* allocator,
              size_t len,
              const uint8_t* data);
void param__free_unpacked(Param* message,
                          ProtobufCAllocator* allocator);
/* Device methods */
void device__init(Device* message);
size_t device__get_packed_size(const Device* message);
size_t device__pack(const Device* message,
                    uint8_t* out);
size_t device__pack_to_buffer(const Device* message,
                              ProtobufCBuffer* buffer);
Device*
device__unpack(ProtobufCAllocator* allocator,
               size_t len,
               const uint8_t* data);
void device__free_unpacked(Device* message,
                           ProtobufCAllocator* allocator);
/* DevData methods */
void dev_data__init(DevData* message);
size_t dev_data__get_packed_size(const DevData* message);
size_t dev_data__pack(const DevData* message,
                      uint8_t* out);
size_t dev_data__pack_to_buffer(const DevData* message,
                                ProtobufCBuffer* buffer);
DevData*
dev_data__unpack(ProtobufCAllocator* allocator,
                 size_t len,
                 const uint8_t* data);
void dev_data__free_unpacked(DevData* message,
                             ProtobufCAllocator* allocator);
/* --- per-message closures --- */

typedef void (*Param_Closure)(const Param* message,
                              void* closure_data);
typedef void (*Device_Closure)(const Device* message,
                               void* closure_data);
typedef void (*DevData_Closure)(const DevData* message,
                                void* closure_data);

/* --- services --- */


/* --- descriptors --- */

extern const ProtobufCMessageDescriptor param__descriptor;
extern const ProtobufCMessageDescriptor device__descriptor;
extern const ProtobufCMessageDescriptor dev_data__descriptor;

PROTOBUF_C__END_DECLS


#endif /* PROTOBUF_C_device_2eproto__INCLUDED */
