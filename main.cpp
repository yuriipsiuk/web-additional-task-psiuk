#include <iostream>
#include <string>
#include <sstream>
#include <map>
#include <fstream>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>
using std::string;
using std::map;
using std::cout;
using std::endl;
using std::getline;
#pragma comment(lib, "ws2_32.lib")
struct http_request {
    string method;
    string path;
    map<string,string> headers;
    map<string,string> query;
    string body;
};
http_request parsing(const string request_text) {
    http_request res;
    std::istringstream stream(request_text);
    string line;

    if (getline(stream, line)) {
        if(!line.empty() && line.back()=='\r') line.pop_back();
        std::istringstream stream2(line);
        string url;
        stream2>>res.method>>url;

        bool writting_path=true;
        bool writting_key=true;
        string current_quer_key="";
        string current_quer_val="";
        for(auto ch:url){
            if(ch=='?'){
                writting_path=false;
                continue;
            }
            if(ch=='='){
                writting_key=false;
                continue;
            }
            if(ch=='&'){
                res.query[current_quer_key]=current_quer_val;
                writting_key=true;
                current_quer_key="";
                current_quer_val="";
                continue;
            }
            if(writting_path) res.path+=ch;
            else if(writting_key) current_quer_key+=ch;
            else current_quer_val+=ch;
        }
        if(current_quer_key!="") res.query[current_quer_key]=current_quer_val;
    }

    while(getline(stream, line) && line!="\r" && !line.empty()) {
        if(!line.empty() && line.back()=='\r') line.pop_back();
        auto x=line.find(": ");
        if(x!=string::npos) res.headers[line.substr(0,x)]=line.substr(x+2);
    }
    auto y=request_text.find("\r\n\r\n");
    if (y!=string::npos) res.body=request_text.substr(y+4);
    return res;
}
string make_http_response(int status, string status_text, string content_type, string body) {
    std::ostringstream res;
    res<<"HTTP/1.1 "<<status<<" "<<status_text<<"\r\n";
    res<<"Content-Type: "<<content_type<<"\r\n";
    res<<"Content-Length: "<<body.length()<<"\r\n";
    res<<"Connection: close\r\n\r\n";
    res<<body;
    return res.str();
}
string handle_request(http_request req) {
   // cout<<"[LOG] "<<req.method<<" "<<req.path<<endl;
    if (req.method=="GET") {
        if (req.path == "/page") {
            string id="no found";
            if(req.query.count("id")) id=req.query["id"];
            string html="<html><body><h1>Page ID: "+id+"</h1></body></html>";
            return  make_http_response(200, "OK", "text/html; charset=utf-8", html);
        }
        else{
            string filepath="index.html";
            if(req.path!="/") filepath=req.path.substr(1);
            std::ifstream file(filepath);
            if (file.is_open()) {
                std::ostringstream line;
                line<<file.rdbuf();
                string content_type="text/plain";
                if(filepath.size()>=4 && filepath.substr(filepath.size()-4)==".css") content_type="text/css";
                else if(filepath.size()>=5 && filepath.substr(filepath.size()-5)==".html") content_type="text/html";
                return  make_http_response(200, "OK", content_type, line.str());
            }
        }
        return  make_http_response(404,"Not Found","text/html","<h1>404 Not Found</h1>");
    }
    if (req.method=="POST") {
        if(req.headers.count("Content-Type")){
            string ct=req.headers["Content-Type"];
            if(ct.find("application/json")!=string::npos){
                //cout<<"[LOG] json="<<req.body<<endl;
                string res_body="{\"status\": \"success\", \"message\": \"OK\", \"data\": "+req.body+"}";
                return  make_http_response(200,"OK","application/json",res_body);
            }
        }
        return  make_http_response(400, "Bad Request", "application/json", "{\"error\": \"No application/json\"}");
    }
    return  make_http_response(501,"Not Implemented","text/plain","Not implemeted");
}
int main(){
    WSADATA W;
    int f1=WSAStartup(MAKEWORD(2, 2), &W);
    if (f1!= 0) {
        cout<<"WSA Start failed: "<<f1<<endl;
        WSACleanup();
        return 1;
    }
    SOCKET servak=socket(AF_INET, SOCK_STREAM, 0);
    if (servak==INVALID_SOCKET) {
        cout<<"Trouble with SOCKET# "<<WSAGetLastError()<<endl;
        WSACleanup();
        return 1;
    }
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);
    if(bind(servak,(sockaddr*)&address,sizeof(address))==SOCKET_ERROR){
        cout<<"Trouble BIND: "<<WSAGetLastError()<<endl;
        closesocket(servak);
        WSACleanup();
        return 1;
    }
    if(listen(servak, 3)==SOCKET_ERROR) {
        cout<<"Trouble listen: "<< WSAGetLastError()<<endl;
        closesocket(servak);
        WSACleanup();
        return 1;
    }

    cout<<"Servak is up on port 8080"<<endl;
    while (true) {
        SOCKET client_socket = accept(servak,0,0);
        if(client_socket==INVALID_SOCKET) continue;
        char buffer[4096] = {0};
        int bytes_received = recv(client_socket, buffer, 4096, 0);
        if (bytes_received>0) {
            http_request req=parsing(string(buffer, bytes_received));
            string http_response=handle_request(req);
            send(client_socket,http_response.c_str(),http_response.length(),0);
        }
        closesocket(client_socket);
    }
    closesocket(servak);
    WSACleanup();

    return 0;
}
