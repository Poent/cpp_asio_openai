#pragma once

#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include "libs/beast/example/common/root_certificates.hpp"

using json = nlohmann::json;

using tcp = boost::asio::ip::tcp;
namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;

class OpenAI_Connection {
private:
    net::io_context& ioc;
    ssl::context& ctx;
    tcp::resolver resolver;
    ssl::stream<tcp::socket> stream;

    std::string host = "api.openai.com";
    std::string port = "443";
    std::string target = "/v1/chat/completions";
    std::string model = "gpt - 3.5 - turbo";
    int version = 11;

    std::string apiKey;

    bool connected = false;

    void loadApiKey();

public:
    OpenAI_Connection(net::io_context& ioc, ssl::context& ctx);
    ~OpenAI_Connection();

    void connect();

    ssl::stream<tcp::socket>& getStream() { return stream; }
    std::string getHost() { return host; }
    std::string getApiKey() { return apiKey; }
    std::string getModel() { return model; }

    bool isConnected();

    void getAvailableModels();

};

class OpenAI_Conversation {
private:
    OpenAI_Connection& connection;
    std::string target = "/v1/chat/completions";
    int version = 11;
    
    std::string system_message;
    std::string conversation_id;

    json functions = json::array(); 


    std::vector<std::pair<std::string, std::string>> history;

    // function to create the body of the request
    // this is a separate function because it is used in both the constructor and sendMessage
    std::string createBody(std::vector<std::pair<std::string, std::string>>& history, const bool summary);

public:
    OpenAI_Conversation(OpenAI_Connection& conn, const std::string& system_message);
    ~OpenAI_Conversation();

    int countTokens(const std::string& text);

    std::string sendMessage(const std::string& message, const bool summary);
    std::string summarize(int maxTokens);

    int randomNumber(int min, int max);

};
