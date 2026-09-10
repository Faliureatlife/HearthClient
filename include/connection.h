#ifndef connection
#define connection

#include "types.h"

void encode_Packet(packetHeader* header, const char* data, uv_stream_t* client );

void receive_Packet(uv_stream_t* client, ssize_t nread, uv_buf_t* buf);

void decode_Packet(packetInfo* packet);

void broadcast_Message(packetHeader* info, const char* name, const char* buf);

#endif
