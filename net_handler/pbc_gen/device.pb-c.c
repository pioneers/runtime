/* Generated by the protocol buffer compiler.  DO NOT EDIT! */
/* Generated from: device.proto */

/* Do not generate deprecated warnings for self */
#ifndef PROTOBUF_C__NO_DEPRECATED
#define PROTOBUF_C__NO_DEPRECATED
#endif

#include "device.pb-c.h"
void   param__init
                     (Param         *message)
{
  static const Param init_value = PARAM__INIT;
  *message = init_value;
}
size_t param__get_packed_size
                     (const Param *message)
{
  assert(message->base.descriptor == &param__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t param__pack
                     (const Param *message,
                      uint8_t       *out)
{
  assert(message->base.descriptor == &param__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t param__pack_to_buffer
                     (const Param *message,
                      ProtobufCBuffer *buffer)
{
  assert(message->base.descriptor == &param__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
Param *
       param__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (Param *)
     protobuf_c_message_unpack (&param__descriptor,
                                allocator, len, data);
}
void   param__free_unpacked
                     (Param *message,
                      ProtobufCAllocator *allocator)
{
  if(!message)
    return;
  assert(message->base.descriptor == &param__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
void   device__init
                     (Device         *message)
{
  static const Device init_value = DEVICE__INIT;
  *message = init_value;
}
size_t device__get_packed_size
                     (const Device *message)
{
  assert(message->base.descriptor == &device__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t device__pack
                     (const Device *message,
                      uint8_t       *out)
{
  assert(message->base.descriptor == &device__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t device__pack_to_buffer
                     (const Device *message,
                      ProtobufCBuffer *buffer)
{
  assert(message->base.descriptor == &device__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
Device *
       device__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (Device *)
     protobuf_c_message_unpack (&device__descriptor,
                                allocator, len, data);
}
void   device__free_unpacked
                     (Device *message,
                      ProtobufCAllocator *allocator)
{
  if(!message)
    return;
  assert(message->base.descriptor == &device__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
void   dev_data__init
                     (DevData         *message)
{
  static const DevData init_value = DEV_DATA__INIT;
  *message = init_value;
}
size_t dev_data__get_packed_size
                     (const DevData *message)
{
  assert(message->base.descriptor == &dev_data__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t dev_data__pack
                     (const DevData *message,
                      uint8_t       *out)
{
  assert(message->base.descriptor == &dev_data__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t dev_data__pack_to_buffer
                     (const DevData *message,
                      ProtobufCBuffer *buffer)
{
  assert(message->base.descriptor == &dev_data__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
DevData *
       dev_data__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (DevData *)
     protobuf_c_message_unpack (&dev_data__descriptor,
                                allocator, len, data);
}
void   dev_data__free_unpacked
                     (DevData *message,
                      ProtobufCAllocator *allocator)
{
  if(!message)
    return;
  assert(message->base.descriptor == &dev_data__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
static const ProtobufCFieldDescriptor param__field_descriptors[4] =
{
  {
    "name",
    1,
    PROTOBUF_C_LABEL_NONE,
    PROTOBUF_C_TYPE_STRING,
    0,   /* quantifier_offset */
    offsetof(Param, name),
    NULL,
    &protobuf_c_empty_string,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "fval",
    2,
    PROTOBUF_C_LABEL_NONE,
    PROTOBUF_C_TYPE_FLOAT,
    offsetof(Param, val_case),
    offsetof(Param, fval),
    NULL,
    NULL,
    0 | PROTOBUF_C_FIELD_FLAG_ONEOF,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "ival",
    3,
    PROTOBUF_C_LABEL_NONE,
    PROTOBUF_C_TYPE_INT32,
    offsetof(Param, val_case),
    offsetof(Param, ival),
    NULL,
    NULL,
    0 | PROTOBUF_C_FIELD_FLAG_ONEOF,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "bval",
    4,
    PROTOBUF_C_LABEL_NONE,
    PROTOBUF_C_TYPE_BOOL,
    offsetof(Param, val_case),
    offsetof(Param, bval),
    NULL,
    NULL,
    0 | PROTOBUF_C_FIELD_FLAG_ONEOF,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned param__field_indices_by_name[] = {
  3,   /* field[3] = bval */
  1,   /* field[1] = fval */
  2,   /* field[2] = ival */
  0,   /* field[0] = name */
};
static const ProtobufCIntRange param__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 4 }
};
const ProtobufCMessageDescriptor param__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "Param",
  "Param",
  "Param",
  "",
  sizeof(Param),
  4,
  param__field_descriptors,
  param__field_indices_by_name,
  1,  param__number_ranges,
  (ProtobufCMessageInit) param__init,
  NULL,NULL,NULL    /* reserved[123] */
};
static const ProtobufCFieldDescriptor device__field_descriptors[4] =
{
  {
    "name",
    1,
    PROTOBUF_C_LABEL_NONE,
    PROTOBUF_C_TYPE_STRING,
    0,   /* quantifier_offset */
    offsetof(Device, name),
    NULL,
    &protobuf_c_empty_string,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "uid",
    2,
    PROTOBUF_C_LABEL_NONE,
    PROTOBUF_C_TYPE_STRING,
    0,   /* quantifier_offset */
    offsetof(Device, uid),
    NULL,
    &protobuf_c_empty_string,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "type",
    3,
    PROTOBUF_C_LABEL_NONE,
    PROTOBUF_C_TYPE_INT32,
    0,   /* quantifier_offset */
    offsetof(Device, type),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "params",
    4,
    PROTOBUF_C_LABEL_REPEATED,
    PROTOBUF_C_TYPE_MESSAGE,
    offsetof(Device, n_params),
    offsetof(Device, params),
    &param__descriptor,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned device__field_indices_by_name[] = {
  0,   /* field[0] = name */
  3,   /* field[3] = params */
  2,   /* field[2] = type */
  1,   /* field[1] = uid */
};
static const ProtobufCIntRange device__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 4 }
};
const ProtobufCMessageDescriptor device__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "Device",
  "Device",
  "Device",
  "",
  sizeof(Device),
  4,
  device__field_descriptors,
  device__field_indices_by_name,
  1,  device__number_ranges,
  (ProtobufCMessageInit) device__init,
  NULL,NULL,NULL    /* reserved[123] */
};
static const ProtobufCFieldDescriptor dev_data__field_descriptors[1] =
{
  {
    "devices",
    1,
    PROTOBUF_C_LABEL_REPEATED,
    PROTOBUF_C_TYPE_MESSAGE,
    offsetof(DevData, n_devices),
    offsetof(DevData, devices),
    &device__descriptor,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned dev_data__field_indices_by_name[] = {
  0,   /* field[0] = devices */
};
static const ProtobufCIntRange dev_data__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 1 }
};
const ProtobufCMessageDescriptor dev_data__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "DevData",
  "DevData",
  "DevData",
  "",
  sizeof(DevData),
  1,
  dev_data__field_descriptors,
  dev_data__field_indices_by_name,
  1,  dev_data__number_ranges,
  (ProtobufCMessageInit) dev_data__init,
  NULL,NULL,NULL    /* reserved[123] */
};