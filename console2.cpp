#include <array>
#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <iostream>


using namespace std;
using namespace boost::asio;

int main(){
    cout << "Content-type: text/html" << endl << endl;
    cout << "<h1>Hello</h1>" << endl;
    // string html = "<h1> Hello </h1>";
    // printf("%s", html.c_str());
    return 0;
}