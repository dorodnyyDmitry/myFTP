#include "server.h"

void Session::do_write(std::string message, yield_context yield) {
  auto self(shared_from_this());

  message.append("\n");

  boost::system::error_code ecode;

  self->sock.async_send(buffer(message.c_str(), message.length()), yield[ecode]);
  if(ecode){ std::cerr << "Sus writing\n"; }
      
  std::cout << "Response " << message << " sent to " << &self->sock << "\n";
  std::cout << "\n\n";
}

void Session::do_read(yield_context yield) {
  auto self(shared_from_this());

  memset(self->read_buffer, 0, 4096);

  boost::system::error_code ecode;

  self->sock.async_read_some(buffer(self->read_buffer, 4096), yield[ecode]);
  if(ecode){
    return;
  } else {
    std::cout << "Read " << std::string(self->read_buffer) << "\n";
  }
}

void Session::do_echo(yield_context yield){
  auto self(shared_from_this());
  
  for(;;){
    do_read(yield);

    do_write(parseInput(yield), yield);
  }
}

bool Session::isEqualTo(std::string it, std::string that){
  std::transform(it.begin(), it.end(), it.begin(), tolower);
  return (it.compare(that) == 0);
}

void Session::sendList(std::string toSend, yield_context yield){
  auto self(shared_from_this());
  auto data_sock = std::make_shared<tcp::socket>(iocontext_);

  boost::system::error_code ecode;

  /*acceptor_.async_accept(*data_sock, yield[ecode]);

  data_sock->async_write_some(buffer(toSend.c_str(), toSend.size()), yield[ecode]);*/

  spawn(iocontext_, [self, data_sock, toSend](yield_context yield){
    boost::system::error_code ecode;
    self->acceptor_.async_accept(*data_sock, yield[ecode]);
    data_sock->async_write_some(buffer(toSend.c_str(), toSend.size()), yield[ecode]);

    if(ecode){ std::cerr << "Error sending list dir\n"; }
  });
}

std::string Session::handleChdir(std::string goTo){
        auto newWD = current_path() / goTo;

        if(!exists(newWD)){
          return("450 action not taken, given directory doesnt exist");
        }
        if(status(newWD).type() != file_type::directory){
          return("450 action not taken, not a directory!");
        }
        this->currentPath.assign(newWD);
        return("250 change dir ok");
}

std::string Session::handleRetr(std::string fname, yield_context yield){
  auto self(shared_from_this());

  auto data_sock = std::make_shared<tcp::socket>(iocontext_);

  std::ifstream local_file;
  std::filesystem::path local_file_path(fname);

  local_file.open(local_file_path, std::ios_base::binary|std::ios_base::ate);

  std::size_t filesize = local_file.tellg();
  local_file.seekg(0);

  boost::system::error_code ecode;

  if(!exists(local_file_path)){
    return("450 action not taken, file doesnt exist");
  }
  if(status(local_file_path).type() != file_type::regular){
    return("450 action not taken, this is not a file");
  }

  char * buff = new char [filesize];
  if(local_file){
    local_file.read(buff, (std::streamsize) filesize);

    std::cout << "Sending " << local_file.gcount() << " bytes\n";
    do_write("150 Opening BINARY mode to transfer data", yield);

    spawn(iocontext_,
        [self, data_sock, buff, buffsize = local_file.gcount()](yield_context yield){
          boost::system::error_code ecode;
          self->acceptor_.async_accept(*data_sock, yield[ecode]);
          std::cout << data_sock->local_endpoint() << " --- " << data_sock->remote_endpoint() << "\n";
          sleep(3);
          data_sock->async_write_some(buffer(buff, buffsize), yield[ecode]);

    });
  }
  return("226 file transfer successful");
}

std::string Session::parseInput(yield_context yield){
  auto self(shared_from_this());

  std::string input = std::string(read_buffer);

  std::cout << "Echo input is: " << input << '\n';

  std::string response = "502 bad command";
  std::regex word_regex("(\\S+)");
  
  if(input.size() == 0){
    return response;
  }
  auto words_begin = std::sregex_iterator(input.begin(), input.end(), word_regex);
  auto words_end = std::sregex_iterator();

  std::smatch match = *words_begin;
  std::string command = match.str();
    
  if (std::distance(words_begin, words_end) == 1){
    if (isEqualTo(command, "pwd")){
      response = "257 \"" + std::string(this->currentPath) + "\" is the current directory";
    }
    else if (isEqualTo(command, "list")) {
      for (const auto & file : directory_iterator(this->currentPath)){
        response.append(file.path());
        response.append("\r\n");
      }  

      do_write("150 sending LIST data", yield);
      sendList(response, yield);
      return("226 Directory data sent");
    }
    else if (isEqualTo(command, "pasv")){

      if(acceptor_.is_open()){
          acceptor_.close();
      }
      boost::system::error_code ecode;

      tcp::endpoint ep(tcp::v4(), 0);

      acceptor_.open(ep.protocol(), ecode);
      acceptor_.bind(ep);
      acceptor_.listen(socket_base::max_connections, ecode);

      auto ip_bytes = sock.local_endpoint().address().to_v4().to_bytes();
      auto port     = acceptor_.local_endpoint().port();

      std::stringstream stream;
      stream << "227 ";
      for (size_t i = 0; i < 4; i++)
      {
        stream << static_cast<int>(ip_bytes[i]) << ",";
      }
      stream << ((port >> 8) & 0xff) << "," << (port & 0xff);

      response = stream.str();
    }
    else if (isEqualTo(command, "quit")){
      response = "221 connection closed by client";
    }
    else if (isEqualTo(command, "feat")){
      response = "211 End";
    }
    else if (isEqualTo(command, "syst")) {
      response = "215 UNIX Type: sus";
    }
    else if (isEqualTo(command, "cdup")){
      if(currentPath == "/"){
        return("450 cant go up from root directory");
      } 
      else{
        currentPath = current_path().parent_path();
        return("200 chdir ok");
      }
    }
  } 
  else{
    std::vector<std::string> queryParams;

    for(std::sregex_iterator i = ++words_begin; i !=  words_end; ++i){
      queryParams.push_back((*i).str());
    }

    if(isEqualTo(command, "cwd")){
      if(queryParams.size() > 1){
        response = "450 action not taken, no directory specified";
      }
      else {
        return(handleChdir(queryParams.front()));
      }
    }
    else if (isEqualTo(command, "port")){
      response = "451 action not taken, use PASSIVE mode";
    }
    else if (isEqualTo(command, "retr")){
      if(queryParams.size() > 1){
        response = "450 action not taken, no file specified";
      }
      else {
        return(handleRetr(queryParams.front(), yield));
        }
      }
    else if (isEqualTo(command, "user")) {
      response = "331 Please specify the password";
    }
    else if (isEqualTo(command, "pass")) {
      response = "230 Login successful";
    }
    else if (isEqualTo(command, "type")) {
      response = "200 Switching to BINARY mode";
    }
  }
  return response;
}

void Session::start(){
  auto self(shared_from_this());

  std::cout << "---NEW CONNECTION---\n";
  
  std::cout << this->sock.local_endpoint() << " --- " << this->sock.remote_endpoint() << '\n';

  spawn(this->sock.get_executor(), 
      [self](yield_context yield){
        self->do_write("220 (vsFTPd 3.0.3)", yield); 

        self->do_echo(yield);
      });
}
