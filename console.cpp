#include <array>
#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <regex>
#include <stdlib.h>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>

using namespace std;
using namespace boost::asio;
io_service global_io_service;

void output_shell(string id, string content){
    boost::replace_all(content,"\r\n","&NewLine;");//Replace all \r\n with &NewLine in content.
    boost::replace_all(content,"\n","&NewLine;");
    boost::replace_all(content, "\\", "\\\\");
    boost::replace_all(content,"\'","\\\'");
    boost::replace_all(content,"<","&lt;");
    boost::replace_all(content,">","&gt;");
    printf("<script>document.getElementById('\"%s\"').innerHTML += '\"%s\"';</script>\n", id.c_str(), content.c_str());
    // cout<<"<script>document.getElementById('"<<id<<"').innerHTML += '"<<content<<"';</script>"<<endl;
    fflush(stdout);
}

void output_command(string id, string content){
    boost::replace_all(content,"\r\n","&NewLine;"); 
    boost::replace_all(content,"\n","&NewLine;");
    boost::replace_all(content, "\\", "\\\\");
    boost::replace_all(content,"\'","\\\'");
    boost::replace_all(content,"<","&lt;");
    boost::replace_all(content,">","&gt;");
    printf("<script>document.getElementById('\"%s\"').innerHTML += '<b>\"%s\"</b>';</script>", id.c_str(), content.c_str());
    // cout<<"<script>document.getElementById('"<<id<<"').innerHTML += '<b>"<<content<<"</b>';</script>"<<endl;
    fflush(stdout);
};

class Shell_session :public enable_shared_from_this<Shell_session>{
    private:
        enum{max_length = 1024};
        ip::tcp::socket _socket;
        ip::tcp::resolver _resolver;
        ip::tcp::resolver::query query;
        string test_case;
        string shell_id;
        string host_id;
        array<char, max_length> _data;
        ifstream test_file;
        deadline_timer timer;

    public:
        Shell_session(string host, string port, string in_test_case, string in_shell_id, string in_host_id) :
        timer(global_io_service), _resolver(global_io_service), _socket(global_io_service),
        query(ip::tcp::v4(), host, port), test_case(in_test_case), shell_id(in_shell_id), host_id(in_host_id) {}
        void start(){
            test_file.open("test_case/" + test_case);
            do_resolve();
        }

    private:
        void wait2sec(){
            auto self(shared_from_this());
            timer.expires_from_now(boost::posix_time::seconds(2));
            timer.async_wait([this,self](boost::system::error_code ec){
                if(!ec){
                    output_command(shell_id,"hello\n"); 
                    wait2sec();
                }
            });
        }
        void do_resolve() {
            auto self(shared_from_this());
            _resolver.async_resolve(query, [this,self](boost::system::error_code ec,ip::tcp::resolver::iterator endpoint_iterator) {
                if (!ec) do_connect(endpoint_iterator);
                else _socket.close();
            }); 
        }
        void do_connect(ip::tcp::resolver::iterator endpoint_iterator){
            auto self(shared_from_this());
            boost::asio::async_connect(_socket, endpoint_iterator, [this, self](boost::system::error_code ec, ip::tcp::resolver::iterator){
                //cout<<"<h1>error "<<ec.message()<<"</h1></br>"<<endl;
                if (!ec) {
                    do_read();
                    wait2sec();
                } 
                else _socket.close();
            }); 
        }
        void do_read() {
            //cout<<"<h1>3"<<test_case<<"</h1></br>"<<endl;
            auto self(shared_from_this());
            _socket.async_read_some(
                buffer(_data, max_length),[this, self](boost::system::error_code ec, std::size_t length) {
                    if (!ec) {
                        string buf(_data.begin(),_data.begin()+length);
                        output_shell(shell_id, buf);
                        if (buf.find("% ") != string::npos)
                            do_send_cmd();
                        do_read();
                    } else {
                        _socket.close();
                    }
                });
        }
        void do_send_cmd() {
            auto self(shared_from_this());
            string temp;
            getline(test_file,temp);
            temp +="\n";
            output_command(shell_id,temp);
            _socket.async_send(buffer(temp),[this, self](boost::system::error_code ec, std::size_t /* length */) {
                if (!ec) do_read();
                else _socket.close();
            });
        }
};

int main(){
    char* query_string = getenv("QUERY_STRING");
    string query(query_string);
    query = "?" + query;
    smatch sm;
    regex pattern("((?:\\?|&)\\w+=)([^&]+)");
    cout << "HTTP/1.1 200 OK"<<endl;
    cout << "Content-type: text/html" << endl << endl;
    string html = 
        R"(<!DOCTYPE html>
        <html lang="en">
        <head>
            <meta charset="UTF-8" />
            <title>NP Project 3 Console</title>
            <link
            rel="stylesheet"
            href="https://stackpath.bootstrapcdn.com/bootstrap/4.1.3/css/bootstrap.min.css"
            integrity="sha384-MCw98/SFnGE8fJT3GXwEOngsV7Zt27NXFoaoApmYm81iuXoPkFOJwJ8ERdknLPMO"
            crossorigin="anonymous"
            />
            <link
            href="https://fonts.googleapis.com/css?family=Source+Code+Pro"
            rel="stylesheet"
            />
            <link
            rel="icon"
            type="image/png"
            href="https://cdn0.iconfinder.com/data/icons/small-n-flat/24/678068-terminal-512.png"
            />
            <style>
            * {
                font-family: 'Source Code Pro', monospace;
                font-size: 1rem !important;
            }
            body {
                background-color: #212529;
            }
            pre {
                color: #cccccc;
            }
            b {
                color: #ffffff;
            }
            </style>
        </head>
        <body>
            <table class="table table-dark table-bordered">
            <thead>
                <tr>
                    <th><pre id="h0" class="mb-0"></pre></th>
                    <th><pre id="h1" class="mb-0"></pre></th>
                    <th><pre id="h2" class="mb-0"></pre></th>
                    <th><pre id="h3" class="mb-0"></pre></th>
                    <th><pre id="h4" class="mb-0"></pre></th>
                </tr>
            </thead>
            <tbody>
                <tr>
                    <td><pre id="s0" class="mb-0"></pre></td>
                    <td><pre id="s1" class="mb-0"></pre></td>
                    <td><pre id="s2" class="mb-0"></pre></td>
                    <td><pre id="s3" class="mb-0"></pre></td>
                    <td><pre id="s4" class="mb-0"></pre></td>
                </tr>
            </tbody>
            </table>
        </body>
        </html>)";
    printf("%s", html.c_str());
    return 0;
}