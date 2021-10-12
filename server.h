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


//! An FTP Session class
/*!
  An instance of this is created via shared pointer each time a new client tries to connect.
  The instance is destroyed and all memory is freed once all operations finish and client disconnects, by destroying all shared pointers, pointing at an instance.
*/

class Session : public std::enable_shared_from_this<Session>{
public:
//! A Session constructor.
/*! Each session is instantiated using its main communication socket and IO-context executor. 
 *  Values of home directory and conneciton acceptor are also defined.
 *  @param socket Main communication socket 
 *  @param dir Home directory
 *  @param io_context IO-context for inter-thread communications
 */
  Session(tcp::socket socket, std::string dir, io_context& iocontext): 
    sock(std::move(socket)), 
    iocontext_(iocontext), 
    acceptor_(iocontext) 
    {
      currentPath = current_path();
      active = true;
    }
  void start();
  void file_start();
  bool active;
private:
  path currentPath;
  //! Handle RETR command from client 
  /*!  @param file Name of a file to be downloaded
   *  @param yield Context handler for coroutines
   *  @return Response code to send to client
   *
   *  Sends 450 if operation wasnt successful, 226 otherwise
   */
  std::string handleRetr(std::string file, yield_context yield);
  
  //! Handle CHDIR command from client.
  /*! @param dir Directory, where client whants to go
   *  @return Response code for client.
   *
   *  Sends 450 if dir is not a directory or doesnt exits, 250 otherwise. 
   */
  std::string handleChdir(std::string dir);

  //! Handle LIST command from client.
  /*! @param yield Yield context
   *  @return Responce code 150 before and 226 after data stream.
   */
  std::string handleList(yield_context yield);

  //! Main command parses function
  /*! Parses incomming commands and calls respective functions for each command, or returns response 502 - command not implemented. 
   *
   * @param yield Yield context
   *  @return Exact data to be sent to the client.
   */
  std::string parseInput(yield_context yield);

  //! Compares two strings
  /*! Transforms all incoming commands into lowercase to determine correct handler function to call
   *  @return True if strings are the same, False otherwise
   */
  bool isEqualTo(std::string it, std::string that);

  //! Main loop function.
  /*! While connection exists, takes command in and then sends response to it in a loop.
   * @param yield Yielf context
   */
  void do_echo(yield_context yield);

  //! Reading function
  /*! Reads an incomming data from main communication socket, and puts it into a session read buffer.
   * @param yield Yield context
   */
  void do_read(yield_context);

  //! Writing function
  /*! Sends a data packet into the main communication socket
   * @param message Data to be sent to client
   * @param yield Yield context
   */
  void do_write(std::string message, yield_context);

  std::string handleStor(std::string, yield_context);


  //! Main communication socket of a session
  /*! Through this socket session talks to client, reading incomming commands and sending response codes or short data packets.
   */
  tcp::socket sock;
  char write_buffer[4096];
  char read_buffer[4096];

  //! Connection acceptor.
  /*! Listens for, binds and accepts all incoming connections.
   */
  tcp::acceptor acceptor_;
  
  //! IO-context of a session
  /*! Does stuff :)
   */
  io_context& iocontext_;
};


//! Server class
/*! Responsible for accepting new clients and creating sessions for each of them. 
 */
class Server : public std::enable_shared_from_this<Server>{
public:

  //! Constructor
  /*! Creates a server instance from IO-context and port to listen for clients on.
   * @param iocontext IO-context for inter-thread communication
   * @param port Main server listening port. Clients will attempt to connect to this port to initiate an FTP session
   */
  Server(io_context& iocontext, int port): 
    iocontext_(iocontext),
    acceptor(iocontext_, tcp::endpoint(tcp::v4(), port))
  {}

  //! Accept function
  /*! Listens for new connections and creates new sessions once connection attempt is made by client
   * @param sock Socket, on which acceptos should be listening for incoming connections
   */
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
