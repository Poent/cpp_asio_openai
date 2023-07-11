#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "OpenAI_Handler.h"

int main() {

    net::io_context ioc;
    ssl::context ctx{ ssl::context::tlsv12_client };

    // Create the connection object.
    OpenAI_Connection connection(ioc, ctx);

    // sleep for 1 second
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // check the connection
    if (!connection.isConnected()) {
        std::cout << "Connection failed.\n";
        return EXIT_FAILURE;
    }

    // Create the conversation object.
    OpenAI_Conversation conversation(connection, "You are a helpful assistant that can call pre-defined functions. ");

    // while we're connected
    while (connection.isConnected()) {
		// get the user's message
		std::string message;
		std::cout << "You: ";
		std::getline(std::cin, message);

        //check to make sure the connection is still good, if not try to reconnect
        if (!connection.isConnected()) {
		    
            std::cout << "Connection lost. Attempting to reconnect...\n";
            
            // reconnect
            connection.connect();
            // check the connection
            if (!connection.isConnected()) {
				std::cout << "Reconnect failed.\n";
				return EXIT_FAILURE;
			} 
        }

		// send the message to the server
		std::string response = conversation.sendMessage(message, 0);

	}
}
