#include "server.h"
#include <boost/asio/strand.hpp>
#include <iostream>

int main(int argc, char* argv[]){

//  spawn(iocontext, 
//      [&iocontext, port = std::atoi(argv[1]), dir = std::string(argv[2])](yield_context yield){
//        tcp::endpoint ep{tcp::v4(), (uint16_t) port};
//        tcp::acceptor acceptor(iocontext, ep);
//
//    
//        for(;;){
//
//          boost::system::error_code ecode;
//          tcp::socket sock(iocontext);
//
//          acceptor.listen();
//
//          acceptor.async_accept(sock, yield[ecode]);
//
//          if(!ecode){ std::make_shared<Session>(std::move(sock), std::move(dir), iocontext)->start(); }
//
//          std::cout << "After accept\n";
//
//        }
//      });

  if(argc != 3){
    std::cerr << "Wrong params! Usage: \"myFTP\" \"port\" \"threads\"\n";
    return(-1);
  }

  unsigned int port = std::stoi(argv[1]);
  std::size_t nthreads = std::stoi(argv[2]);

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


