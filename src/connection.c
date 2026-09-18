#include "types.h"
#include "connection.h"

uint32_t msgid = 0;

PACKET_DEFAULT_SIZE=64*1024;

void encode_Packet(enum packetType type, ssize_t len, const char* data, uv_stream_t* client ){
  //dumb and unvalided
  //going to need some type of parser for commands probably who knows
  packetHeader* header = (packetHeader*)malloc(sizeof(packetHeader));
  header->type = type;
  header->version = 0; //change for real versions
  header->id = ++msgid;
  header->payloadLen = len; 

  broadcast_Message(header, myname, data); //myname extern global, (uint8_t)
}

//this will be called by listening, accumulate portions of packets in a buffer and then passes full packets to decode_Packet()
void receive_Packet(uv_stream_t* client, ssize_t nread, uv_buf_t* buf){ //need to check that char* is what I actually want here 
  //accumulate the data into a linked list of buffers that hold packets (shouldnt need more than a few entries at a time)
  //when the data makes up (header + payloadlen) we will make it into a packet and process with decode_Packet which will handle it according to type
  //going to put all the packets into a LL that will contain packetInfo objects 
  //i need to make sure i am freeing the uv_buf_t properly somewhere 
   
  if (nread < 0) {return;} //TEMP ERROR HANDLING
  packetInfo* packet; 
  inProgress* currentData = NULL;
  HASH_FIND_PTR(packetlist, &client, currentData);


  if (currentData != NULL){ readdata:
    char* tempbuf = (char*)malloc(PACKET_DEFAULT_SIZE);
    memcpy(tempbuf, currentData->rawData, currentData->rawSize);
    memcpy(tempbuf + currentData->rawSize, buf->base, nread);
    int readdata = 0;
    int leftoverdata = nread + currentData->rawSize;

    //everything is allocated, just have to see what we have and add it 
    if (currentData->partHeader->type == 0) {goto Ype;}
    else if (currentData->partHeader->version == 0) {goto Ver;}
    else if (currentData->partHeader->id == 0) {goto Di;}
    else if (currentData->partHeader->payloadLen == 0) {goto Len;}
    else {goto Payload;}
    
    Ype:
      if (leftoverdata >= sizeof(uint8_t)) {//sizeof for portability?
        //moving the data in char 1
        currentData->partHeader->type = tempbuf[0 + readdata];
        leftoverdata = leftoverdata - sizeof(uint8_t);
        readdata = readdata + sizeof(uint8_t);
      } else goto Done;
    Ver:
      if(leftoverdata >= sizeof(uint8_t)){
        currentData->partHeader->version = tempbuf[0 + readdata];
        leftoverdata = leftoverdata - sizeof(uint8_t);
        readdata = readdata + sizeof(uint8_t);
      } else goto Done;
    Di:
      if(leftoverdata >= sizeof(uint32_t)){
        //heard this is better than memcpy for explicit ordering
        currentData->partHeader->id = 
          tempbuf[readdata] << 24 | tempbuf[readdata + 1] << 16 |
          tempbuf[readdata+2] << 8 | tempbuf[readdata + 3];
        leftoverdata = leftoverdata - sizeof(uint32_t);
        readdata = readdata + sizeof(uint32_t);
      } else goto Done;
    Len:
      if(leftoverdata >= sizeof(uint32_t)){
        currentData->partHeader->payloadLen = 
          tempbuf[readdata] << 24 | tempbuf[readdata + 1] << 16 |
          tempbuf[readdata+2] << 8 | tempbuf[readdata + 3];
        leftoverdata = leftoverdata - sizeof(uint32_t);
        readdata = readdata + sizeof(uint32_t);
      } else goto Done;
    Payload:
      if (leftoverdata == currentData->partHeader->payloadLen){
        char* tchar = (char*)malloc(currentData->partHeader->payloadLen);
        memcpy(tchar, tempbuf + readdata, currentData->partHeader->payloadLen);
        packet = (packetInfo*)malloc(sizeof(packetInfo));
        packet->header = currentData->partHeader;
        packet->client = client;
        packet->data = tchar;
        //check to see if i have to delete anything else
        HASH_DEL(packetlist,currentData);
        free(tempbuf);
        goto finishedPacket;
      }

    Done:
      memcpy(currentData->rawData, tempbuf + readdata, leftoverdata);
      currentData->rawSize = leftoverdata;
      free(tempbuf);

  } else {    
    //create the header
    packetHeader* processHeader = (packetHeader*)malloc(sizeof(packetHeader));
    processHeader->type = 0;
    processHeader->version = 0;
    processHeader->id = 0;
    processHeader->payloadLen = 0;

    //create the inProgress struct
    currentData = (inProgress*)malloc(sizeof(inProgress)); 
    currentData->partHeader = processHeader;
    currentData->rawSize = 0;
    currentData->rawData = (char*)malloc(PACKET_DEFAULT_SIZE);// expandable in future likely
    currentData->client = client;
    HASH_ADD_PTR(packetlist, client, currentData);

    goto readdata;
  }
  finishedPacket:
  decode_Packet(packet);
}

//packet gives me header, client*, and data*
void decode_Packet(packetInfo* packet){
  //redundant but its easier to have this "alias"
  enum packetType type = (enum packetType)packet->header->type;
  //i _think_ it makes sense to have it here unless its only neeeded for resending messages

  switch (type) {
    //sending
    case USER_GET:
      memset(packet->data, 0, PACKET_DEFAULT_SIZE);
      broadcast_Message(packet->header, myname, packet->data);
      break;
    case USER_LEAVE:
    case CHANNEL_JOIN:
    case CHANNEL_REQ_NEW:
    case CHANNEL_LIST:
    case USER_LEAVE:
    case USER_UPDATE:
    case SEND_MESSAGE:
      write_req_t* req = malloc(sizeof(write_req_t));
      uv_buf_init(packet->data, packet->header->payloadLen);
      uv_write((uv_write_t*)&req->req, client, &req->buf,1,echo_write2);
      break;
    case SEND_PIC:
      //send placeholder message to reserve the place, and fill it in with the actual picture after it is done sending
      break;

    //recieving
    case USER_INFO:
    case CHANNEL_NEW:
    case RECEIVE_MESSAGE: 
      write_req_t* req = malloc(sizeof(write_req_t));
      uv_buf_init(packet->data, packet->header->payloadLen);
      uv_write((uv_write_t*)&req->req, ttyin, &req->buf,1,echo_write2);
      break;
    default:
      fprintf(stderr, "whoops I haven't implemented that packet type yet\n");
    }
}

//using write_req_t
void echo_write2(uv_write_t* req, int status){
  write_req_t* wr = (write_req_t*)req;
  free(wr->buf->data);
  free(wr);
}
//using writeReq
void echo_write (uv_write_t* req, int staus){
  writeReq* wr = (writeReq*)req;
  free(wr->data);
  free(wr);
}

//uses the pointers from the packet created in receive_Packet, packetInfo->data
//should eventually make alternative that selects for roles
void broadcast_Message(packetHeader* info, const char* buf){
  writeReq* req = (writeReq*)malloc(sizeof(writeReq));
  char* message = (char*)malloc(info->payloadLen);
  memcpy(message, buf, info->payloadLen);
  req->buf = uv_buf_init(message, fullsize);
  req->data = message;

  uv_write((uv_write_t* )&req->req, client, &req->buf,1,echo_write);
  }
}
