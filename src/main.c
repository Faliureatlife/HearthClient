#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <uv.h>
// #include "types.h"

typedef struct{
  uv_write_t req;
  uv_buf_t buf;
} write_req_t;


uv_loop_t* loop;
uv_tcp_t client;
uv_tty_t ttyin;

char* myname;
uint8_t namelen = 0;

//aka write_cb
void on_write(uv_write_t* req, int status){
  if (status < 0) 
    fprintf(stderr, "Write error :%s\n", uv_strerror(status));
  write_req_t* wr = (write_req_t*) req;
  free(wr->buf.base);
  free(req);
}

void send_write(uv_stream_t* stream, char* data, int len){
  write_req_t* req = malloc(sizeof(write_req_t));
  char* tbuf = malloc(len);
  memcpy(tbuf, data, len);

  req->buf = uv_buf_init(tbuf,len);
  uv_write((uv_write_t*)req, stream, &req->buf, 1, on_write);
}

void on_close(uv_handle_t* handle){
  fprintf(stderr, "Disconnecting... \n");
  free(handle);
}

void outputline(const char* incoming, size_t len){
  fwrite(incoming, 1, len, stdout);
  fputc('\n',stdout);
  fflush(stdout);
}

//aka read_cb
void on_read(uv_stream_t* server, ssize_t nread, const uv_buf_t* buf){
  char* cpybuf = malloc(nread);

  if (nread > 0){
    memcpy(cpybuf, buf->base, nread);
    outputline(cpybuf,nread);

  } else if (nread < 0){
      if (nread != UV_EOF)
        fprintf(stderr, "Read error :%s\n", uv_strerror(nread));
      uv_close((uv_handle_t*) server, on_close);
  }
  free(buf->base);
}

void on_stdin_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf){
  if (nread > 0) {
    char* line = malloc(nread);
    memcpy(line,buf->base,nread);
    encode_packet(SEND_MESSAGE,nread,line,server); //rewrite to make server into a list and use appropriate ones to be in multiple servers?
  } else if (nread < 0){
      if (nread != UV_EOF)
        fprintf(stderr, "Read error :%s\n", uv_strerror(nread));
      uv_close((uv_handle_t*)stream, on_close);
  }
  free(buf->base);
}


//aka alloc_cb
//directly allocating the two so i dont forget what i allocate .w.
void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf){
  buf->base = malloc(suggested_size);
  buf->len = suggested_size;
}

void on_connect(uv_connect_t* req, int status){
  if (status < 0){
    fprintf(stderr, "Error connecting :%s\n", uv_strerror(status));
    return;
  }
  uv_read_start((uv_stream_t*)&client, alloc_buffer, on_read);
  uv_read_start((uv_stream_t*)&ttyin, alloc_buffer, on_stdin_read);
  encode_Packet(INTRODUCE, len, client_data, client); //will need to do more once this packet is formalized
  // send_write((uv_stream_t*)&client, "HELLO", 5);
}

int main(int argc, char* argv[]){
  loop = uv_default_loop();

  uv_tty_init(loop, &ttyin, STDIN_FILENO, 1);

  uv_tcp_init(loop, &client);
  struct sockaddr_in dest;
  uv_ip4_addr("0.0.0.0", 7000, &dest);
  myname = malloc(sizeof(char)*128);
  myname = "defaultname";
  namelen = strlen(myname);

  uv_connect_t connect_req;
  uv_tcp_connect(&connect_req, (uv_tcp_t*)&client, (const struct sockaddr*)&dest, on_connect);
  uv_run(loop,UV_RUN_DEFAULT);
  uv_loop_close(loop);
  return 0;

}
