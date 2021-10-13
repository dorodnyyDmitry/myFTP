#include "server.h"

int main(int argc, char* argv[]){
  if(argc != 3){
    std::cerr << "Wrong params! Usage: \"myFTP\" \"port\" \"threads\"\n";
    return(-1);
  }
  
  unsigned int port;
  std::size_t nthreads; 

  try{
    port = std::stoi(argv[1]);
  }
  catch(std::invalid_argument exception){
    std::cerr << "Bad port!\n";
    return(-1);
  }

  try{
    nthreads = std::stoi(argv[2]);
  }
  catch(std::invalid_argument exception){
    std::cerr << "Bad threads!\n";
    return(-1);
  }

  io_context iocontext;
  thread_pool threadpool(nthreads);
  Server server(iocontext, port);
  tcp::socket sock(iocontext);

  server.do_accept(sock);

  for(int i = 0; i < nthreads; ++i){
    post(threadpool, [&iocontext](){
        iocontext.run();
        });
  }

  threadpool.join();

	return 0;
}


