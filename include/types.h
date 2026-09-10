#ifndef TYPES
#define TYPES

#include <uv.h>
#include "uthash/src/uthash.h"


//ew portability
#if defined(_WIN32)
  #include <rpc.h>
#elif defined(__APPLE__) || defined(__linux__)
  #include <uuid/uuid.h>
#endif

#define MAX_MSG_LEN 4096

typedef struct{
  char*           name; //the key
  int             default_channel; //a bool 
  UT_hash_handle  hh;
  char**          required_permission; //tbh im not going to use this for a while
} Channel;

typedef struct {
  uv_write_t      req;
  uv_buf_t        buf;
} write_req_t;

//we are hashing the user handle pointers, this allows us to O(1) lookup information about a user
typedef struct {
  uint16_t channelID; //id of where it was sent
  uint16_t length;
  char* text; //message body
} receiveMessage;

typedef struct { //dont need pretty sure
  uint16_t channelID; //id of where it was sent
  uint8_t nameLen;
  char* name;
  uint16_t length;
  char* text; //message body
} sendMessage;

typedef struct {
  uint16_t channelID; //id of new channel 
  uint16_t length; 
  char* text; //name of channel
} newChannel;

typedef struct {
  uint16_t length;
  char* text; //the list of all channels
} listChannel;

typedef struct {
  uint16_t lengthName;
  char* name; 
  uint16_t lengthBio;
  char* bio;
  //future pfp data?
} infoUser;

typedef struct {
  uint16_t lengthName;
  char* name; //whatever is not getting updated will be null or smth
  uint16_t lengthBio;
  char* bio;
} updateUser;

//god only knows what types i am forgetting but this is enough to have an idea at least I hope
//using naming from the server's POV 
enum packetType {
  HELLO = 0x01,

  SEND_MESSAGE    = 0x10,
  RECEIVE_MESSAGE = 0x11, //
  SEND_PIC        = 0x12,

  CHANNEL_NEW     = 0x20, //notify users of new channel
  CHANNEL_LIST    = 0x21, //send list of all channels
  CHANNEL_JOIN    = 0x22, //use if we want to avoid client caching
  CHANNEL_REQ_NEW = 0x23, //usr request new channel

  USER_GET        = 0x30, //request for user information
  USER_UPDATE     = 0x31, //update selected user field
  USER_INFO       = 0x32, //all of a users information
  USER_LEAVE      = 0x33,
};


typedef struct {
  uint8_t         type; //need to cast just to be safe
  uint8_t         version;
  uint32_t        id;
  uint32_t        payloadLen;
} packetHeader;

//no clue if this is useful
//pretty sure not 23 Aug 2026
typedef struct {    
  packetHeader*   header;
  uv_stream_t*    client;
  const char*     data;
} packetInfo;

typedef struct{
  packetHeader*   partHeader;
  uint32_t        rawSize;
  char*           rawData;         //this is the data that we work with
  uv_stream_t*    client;
  UT_hash_handle  hh;
} inProgress;

typedef struct {
  uv_write_t      req;
  uv_buf_t        buf;
  char*           data;
} writeReq;


extern inProgress* packetlist;
extern uv_tcp_t client;
extern void echo_write(uv_write_t* req, int status);

#endif
