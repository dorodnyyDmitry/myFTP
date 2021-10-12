#ifndef SERVER_H
#define SERVER_H

#include <boost/smart_ptr/enable_shared_from_this.hpp>
#include <cstdio>
#include <boost/asio/io_context.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/shared_ptr.hpp>
#include <cstdlib>
#include <unistd.h>
#include <string>
#include <iostream>
#include <vector>
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <regex>
#include <iterator>
#include <algorithm>
#include <list>
#include <memory>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <boost/array.hpp>

using namespace boost::asio;
using namespace boost::asio::ip;
using namespace std::filesystem;

class Session : public std::enable_shared_from_this<Session>{
public:
  Session(tcp::socket socket, std::string dir, io_context& iocontext): 
    sock(std::move(socket)),
    iocontext_(iocontext),
    acceptor_(iocontext)
    {
      currentPath = current_path();
    }
  void start();
  void file_start();
private:
  path currentPath;
  std::string handleRetr(std::string, yield_context);
  std::string handleChdir(std::string);
  void sendList(std::string, yield_context);
  std::string parseInput(yield_context);
  bool isEqualTo(std::string, std::string);
  void do_echo(yield_context yield);
  void do_read(yield_context);
  void do_write(std::string message, yield_context);
  tcp::socket sock;
  char write_buffer[4096];
  char read_buffer[4096];
  std::vector<char> buf;
  tcp::acceptor acceptor_;
  io_context& iocontext_;
};


class Server : public std::enable_shared_from_this<Server>{
public:
  Server(io_context& iocontext, int port): 
    iocontext_(iocontext),
    acceptor(iocontext_, tcp::endpoint(tcp::v4(), port))
  {}

  void do_accept(tcp::socket& sock){
    acceptor.set_option(socket_base::reuse_address{true});

    spawn(iocontext_, 
        [this, &sock](yield_context yield){
          for(;;){
            boost::system::error_code ecode;

            acceptor.async_accept(sock, yield[ecode]);
            if(ecode){ return; }
            
            std::make_shared<Session>(std::move(sock), "./", iocontext_)->start();
          }
        });

  }
private:
  io_context& iocontext_;
  tcp::acceptor acceptor;
};

#endif //SERVER_H
