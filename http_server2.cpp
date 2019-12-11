#include <array>
#include <boost/asio.hpp>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <string>
#include <regex>
#include <unistd.h>
#include <stdlib.h>
#include <map>
#include <sys/types.h>
#include <sys/wait.h>

using namespace std;
using namespace boost::asio;

io_service global_io_service;

class HttpSession : public enable_shared_from_this<HttpSession> {
    private:
        enum { max_length = 1024 };
        ip::tcp::socket _socket;
        array<char, max_length> _data;

    public:
        HttpSession(ip::tcp::socket socket) : _socket(move(socket)) {}

        void start(){do_read();}

    private:
        void do_read() {
            auto self(shared_from_this());
            _socket.async_read_some(
                buffer(_data, max_length),
                [this, self](boost::system::error_code ec, std::size_t length) {
                    /*for(const auto& s: _data) cout << s ;
                    cout<<endl;*/
                    //if (!ec) do_write(length);
                    string buf(_data.begin(),_data.begin()+length);
                    cout<<buf<<endl;
                    do_cmd(buf);
                });
        }

        void do_write(std::size_t length) {
            auto self(shared_from_this());
            _socket.async_send(
                buffer(_data, length),
                [this, self](boost::system::error_code ec, std::size_t /* length */) {
                    if (!ec) do_read();
                });
        }

        void do_cmd(string str){
            smatch sm;
            map<string,string> env;
            regex pattern("(.*) /(.*)cgi(.*) (.*)");
            cout<<"------------------------------"<<endl;
            cout<<regex_search(str, pattern)<<endl;
            if(regex_search(str , sm , pattern)){
                string cmd = sm[2].str();
                cmd = "./" + cmd + "cgi"; 
                env["REQUEST_METHOD"] = sm[1].str();
                env["REQUEST_URI"] = sm[2].str() + "cgi" + sm[3].str();
                if(sm[3].str().size()!=0){
                    string temp = sm[3].str().substr(1,sm[3].str().size()-1);
                    env["QUERY_STRING"] = temp;
                }
                env["SERVER_PROTOCOL"] = sm[4].str();
                regex pattern2("(.*): (.*)");
                while (std::regex_search (str,sm,pattern2)) {
                    env["HTTP_"+sm[1].str()] = sm[2].str();
                    if(sm[1]=="Host")env["HTTP_HOST"] = sm[2].str();
                    str = sm.suffix().str();
                }
                env["SERVER_ADDR"] = _socket.local_endpoint().address().to_string();
                env["SERVER_PORT"] = to_string(_socket.local_endpoint().port());
                env["REMOTE_ADDR"] = _socket.remote_endpoint().address().to_string();
                env["REMOTE_PORT"] = to_string(_socket.remote_endpoint().port());

                pid_t pid;
                global_io_service.notify_fork(boost::asio::io_service::fork_prepare);
                pid = fork();
                if(pid == 0){
                    cout<<"fork"<<endl;
                    global_io_service.notify_fork(boost::asio::io_service::fork_child);
                    char* const argv[2] = {(char *)cmd.c_str(),NULL};
                    env_set(env);
                    dup2(_socket.native_handle(),0);
                    dup2(_socket.native_handle(),1);
                    dup2(_socket.native_handle(),2);
                    _socket.close();
                    cout << "HTTP/1.1 200 OK"<<endl;
                    if(execvp(argv[0],argv) == -1){
                        cout << "HTTP/1.1 200 OK"<<endl;
                        cout << "Content-type: text/html" << endl << endl;
                        cout << "<h1>123456789</h1>" << endl;
                    }
                }else{
                    global_io_service.notify_fork(boost::asio::io_service::fork_parent);
                    _socket.close();
                }
            }
        }
        void env_set(map<string,string> env){
            map<string, string>::iterator iter;
            for(iter = env.begin(); iter != env.end(); iter++){
                cout<<iter->first<<" "<<iter->second<<endl;
                setenv(iter->first.c_str(),iter->second.c_str(),1);
            }
        }
};


class HttpServer {
    private:
    ip::tcp::acceptor _acceptor;
    ip::tcp::socket _socket;

    public:
        HttpServer(short port)
        : _acceptor(global_io_service, ip::tcp::endpoint(ip::tcp::v4(), port)),
        _socket(global_io_service) {
            do_accept();
        }

    private:
        void do_accept() {
            _acceptor.async_accept(_socket, [this](boost::system::error_code ec) {
                if (!ec) make_shared<HttpSession>(move(_socket))->start();
                do_accept();
            });
        }
};
void signal_handle(int signum){
    int status;
    while(waitpid(-1,&status,WNOHANG)>0){
    }
}
int main(int argc, char* const argv[]) {
    if (argc != 2) {
        std::cerr << "Usage:" << argv[0] << " [port]" << endl;
        return 1;
    }
    signal(SIGCHLD, signal_handle);
    clearenv();
    setenv("PATH","/usr/bin:.",1);

    try {
        short port = atoi(argv[1]);
        HttpServer server(port);
        global_io_service.run();
    } catch (exception& e) {
        cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}