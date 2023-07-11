#include "OpenAI_Handler.h"
#include <random>

//globally define the model
static const std::string model = "gpt-3.5-turbo-0613";

OpenAI_Connection::OpenAI_Connection(net::io_context& ioc, ssl::context& ctx)
	: ioc(ioc), ctx(ctx), resolver(ioc), stream(ioc, ctx) 
{
	loadApiKey();

	host = "api.openai.com";
	port = "443";
	target = "/v1/chat/completions";
	version = 11;

	// This holds the root certificate used for verification
	load_root_certificates(ctx);

	// Verify the remote server's certificate
	ctx.set_verify_mode(ssl::verify_peer);
	
	// connect to the server
	connect();
	getAvailableModels();

}

OpenAI_Connection::~OpenAI_Connection()
{
	// close the connection

	beast::error_code ec;
	stream.shutdown(ec);

	if (ec == net::error::eof)
	{
		// Rationale:
		// http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
		ec = {};
	}
	if (ec)
		throw beast::system_error{ec};

}

void OpenAI_Connection::loadApiKey()
{
    // Load the api key using _dupenv_s from the environment variable OPENAI_API_KEY.
	// store the api key in the private member variable apiKey.
	char* apiKeyBuffer = nullptr; // buffer to store the api key
	size_t size = 0; // size of the buffer

	// _dupenv_s returns 0 if the environment variable was found and the buffer was successfully allocated.
	if (_dupenv_s(&apiKeyBuffer, &size, "OPENAI_API_KEY") == 0 && apiKeyBuffer != nullptr)
	{
		apiKey = std::string(apiKeyBuffer, size - 1);
		free(apiKeyBuffer);
	}
	else
	{	
		std::cout << "Could not load api key from environment variable OPENAI_API_KEY." << std::endl;
		apiKey = "";
	}
}

void OpenAI_Connection::connect()
{
	std::cout << "Connecting to " << host << ":" << port << "\n";
	auto const results = resolver.resolve(host, port);
	net::connect(stream.next_layer(), results.begin(), results.end());
	if (!stream.next_layer().is_open()) {
		std::cout << "Failed to connect.\n";
		connected = false;
	}
	else {
		std::cout << "Connection successful.\n";
		connected = true;
	}
	if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str())) {
		throw boost::system::system_error(::ERR_get_error(), boost::asio::error::get_ssl_category());
	}
	try {
		stream.handshake(ssl::stream_base::client);
		connected = true;
	}
	catch (const boost::system::system_error& e) {
		std::cout << "Handshake error: " << e.what() << "\n";
		std::cout << "Error code: " << e.code() << "\n";
		connected = false;
	}
	catch (const std::exception& e) {
		std::cout << "An error occurred during the handshake: " << e.what() << "\n";
		connected = false;
	}
}

bool OpenAI_Connection::isConnected() {

	// check if the connection is still open
	if (!stream.next_layer().is_open()) {
		std::cout << "Connection is closed.\n";
		connected = false;
	}

	return connected;
}


void OpenAI_Connection::getAvailableModels()
{
	// Create the GET request
	http::request<http::string_body> req{http::verb::get, "/v1/models", version};
	req.set(http::field::host, host);
	req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
	req.set(http::field::authorization, "Bearer " + apiKey);

	// Send the request
	http::write(stream, req);

	// Receive the response
	beast::flat_buffer buffer;
	http::response<http::dynamic_body> res;
	http::read(stream, buffer, res);

	// Parse the response body into a json object
	std::string responseBody = beast::buffers_to_string(res.body().data());
	json responseJson = json::parse(responseBody);

	// Check if the response contains a 'data' field
	std::vector<std::string> modelIds;
	if (responseJson.contains("data") && responseJson["data"].is_array()) {
		// Iterate over the 'data' array and store the 'id' of each model
		for (const auto& model : responseJson["data"]) {
			if (model.contains("id") && model["id"].is_string()) {
				modelIds.push_back(model["id"].get<std::string>());
			}
		}
	}
	else {
		std::cout << "The response does not contain a list of models." << std::endl;
	}

	// Sort the model IDs
	std::sort(modelIds.begin(), modelIds.end());

	// Print the sorted model IDs
	for (const auto& modelId : modelIds) {
		std::cout << "Model ID: " << modelId << std::endl;
	}
}



// constructor for OpenAI_Conversation
// takes in a reference to an OpenAI_Connection and a system message
OpenAI_Conversation::OpenAI_Conversation(OpenAI_Connection& conn, const std::string& system_message)
	: connection(conn), system_message(system_message)
{
	// Define the randomNumber function
	nlohmann::json randFunc;

	randFunc["name"] = "randomNumber";
	randFunc["description"] = "Returns a random number between 0 and 1000";
	randFunc["parameters"]["type"] = "object";
	randFunc["parameters"]["properties"] = json::object();

	// Add the randomNumber function to the functions array
	functions.push_back(randFunc); 

	// Print the functions array
	std::cout << functions.dump(4) << std::endl;

}




OpenAI_Conversation::~OpenAI_Conversation()
{

}

std::string OpenAI_Conversation::sendMessage(const std::string& message, const bool summary)
{
	// add the message to the history
	history.push_back({ "user", message });

	// create the body of the request
	std::string body = createBody(history, summary);
	
	// create the request
	http::request<http::string_body> req{ http::verb::post, target, version };
	req.set(http::field::host, connection.getHost());
	req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
	req.set(http::field::authorization, "Bearer " + connection.getApiKey());
	req.set(http::field::content_type, "application/json");
	req.body() = body;
	req.prepare_payload();


	//print out a json formatted version of the request
	std::cout << "\n==========================================================" << std::endl;
	std::cout << "Request:" << std::endl;
	std::cout << req << std::endl;
	std::cout << "\n==========================================================" << std::endl;



	try {
		http::write(connection.getStream(), req);
	}
	catch (const boost::system::system_error& e) {
		std::cout << "Write error: " << e.what() << "\n";
		std::cout << "Error code: " << e.code() << "\n";
	}


	beast::flat_buffer buffer;
	http::response<http::dynamic_body> res;

	try {
		http::read(connection.getStream(), buffer, res);
	}
	catch (const boost::system::system_error& e) {
		std::cout << "Read error: " << e.what() << "\n";
		std::cout << "Error code: " << e.code() << "\n";
	}

	if (res.result_int() != 200) {
		std::cout << "Response returned with status: " << res.result_int() << "\n";
		return std::to_string(res.result_int());
	}

	std::string responseBody = beast::buffers_to_string(res.body().data());
	std::cout << "Received Response body: " << responseBody << std::endl;

	// Extract the assistant's message from the response and add it to the history.
	json response_json = json::parse(responseBody);
	std::string assistant_message = response_json["choices"][0]["message"]["content"];

	// get the token information from the response


	history.push_back({ "assistant", assistant_message });
	std::cout << "==========================================================" << std::endl;
	std::cout << "Assistant: " << assistant_message << std::endl;
	std::cout << "==========================================================" << std::endl;

	return assistant_message;
}

std::string OpenAI_Conversation::createBody(std::vector<std::pair<std::string, std::string>>& history, const bool summary)
{
	// Clear the body
	json body;


	// Add the functions array
	body["functions"] = functions;
	// Add the model


	// Add the system message
	json system_message_json;
	system_message_json["role"] = "system";
	system_message_json["content"] = system_message;

	// Add the system message to the history
	body["messages"].push_back(system_message_json);


	// Add the messages
	for (const auto& message : history) {
		json messageJson;
		messageJson["role"] = message.first;
		messageJson["content"] = message.second;
		body["messages"].push_back(messageJson);
	}

	// Add the function_call parameter
	//body["function_call"] = "auto";

	body["model"] = model;


	std::cout << "====== Estimated Tokens: " << countTokens(body.dump()) << " ======" << std::endl;

	// Check if the body is larger than 3500 tokens. if so, request a summary instead.
	if (countTokens(body.dump()) > 500 && !summary) {
		// Get the summary and use it as the new history.
		std::string summary = summarize(50);

		// Print out the summary.
		std::cout << "==========================================================" << std::endl;
		std::cout << "                    !!!!SUMARIZING!!!!                    " << std::endl;
		std::cout << "==========================================================" << std::endl;
		std::cout << "Summary: " << summary << std::endl;
		std::cout << "==========================================================" << std::endl;

		// Clear the history and add the summary.
		history.clear();
		history.push_back({ "assistant", summary });

		// Recreate the body with the new history.
		body.clear();
		body["model"] = model;
		for (const auto& message : history) {
			json messageJson;
			messageJson["role"] = message.first;
			messageJson["content"] = message.second;
			body["messages"].push_back(messageJson);
		}
	}

	// pretty-print the body
	std::cout << "==========================================================" << std::endl;
	std::cout << "Body:" << std::endl;
	std::cout << body.dump(4) << std::endl;
	std::cout << "==========================================================" << std::endl;

	return body.dump();
}




std::string OpenAI_Conversation::summarize(int maxTokens)
{
	// Create a string that contains all the messages in the history.
	std::string fullHistory;

	std::cout << "concatenating history" << std::endl;
	for (const auto& message : history) {
		fullHistory += message.first + ": " + message.second + "\n";
	}

	// Now create the prompt for the summary.
	std::string prompt = "Please summarize the following conversation in less than " + std::to_string(maxTokens) + " tokens. DO NOT MODIFY FUNCTIONS.\n" + fullHistory;

	// Use the sendMessage function to get the summary.
	// (You might need to modify this function or create a new one if you need to control the length of the model's responses.)
	std::string summary = sendMessage(prompt, 1);

	return summary;
}


int OpenAI_Conversation::countTokens(const std::string& str)
{
	int tokenCount = 0;
	std::string word;
	std::string delimiters = " .,:;!?'\"-(){}[]<>+=/*_";
	std::istringstream stream(str);

	while (stream >> word) {
		// check if word contains any delimiter
		size_t found = word.find_first_of(delimiters);
		while (found != std::string::npos)
		{
			// when delimiter is found, increase token count and remove the delimiter
			tokenCount++;
			word.erase(word.begin() + found);
			found = word.find_first_of(delimiters);
		}
		// consider remaining words as token
		if (word.length() > 4) {
			tokenCount += 2;  // consider as two tokens
		}
		else {
			tokenCount++;  // consider as one token
		}
	}
	return tokenCount;
}


// random number generator function.
int OpenAI_Conversation::randomNumber(int min, int max)
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(min, max);

	return dis(gen);
}