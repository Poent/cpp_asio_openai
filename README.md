# OpenAI C++ Client

This project is a C++ client for the OpenAI API. It uses the Boost.Asio library for networking and OpenSSL for SSL.

Please note that this project is a work in progress and may contain errors. I program as a hobby, and while I strive to produce quality code, there may be issues that I haven't encountered or addressed. I welcome any feedback or contributions that can help improve the project.

## Features

- Connects to the OpenAI API using secure HTTPS connections.
- Sends and receives messages in a conversation with the OpenAI API.
- Handles connection errors and attempts to reconnect if the connection is lost.
- Retrieves the OpenAI API key from an environment variable for security.
- Uses the Boost.Asio library for networking and OpenSSL for SSL, demonstrating usage of these libraries in a real-world application.
- Early implementation of "Function Calling" in the conversation with the OpenAI API.

## Prerequisites

- A modern C++ compiler with C++14 support
- [Boost libraries](https://www.boost.org/)
- [OpenSSL](https://www.openssl.org/)
- [nlohmann::json](https://github.com/nlohmann/json) for handling JSON data

## Building

1. Clone the repository.
2. Ensure that the Boost, OpenSSL, and nlohmann::json libraries are installed and properly configured in your development environment.
3. Build the project using your preferred C++ build system or IDE.

## Usage

1. Set the `OPENAI_KEY` environment variable to your OpenAI API key.
2. Run the built executable. The program will connect to the OpenAI API and start a conversation.
3. Enter your messages at the `You: ` prompt. The program will send your messages to the OpenAI API and print the responses.

## Contributing

Contributions are welcome! Please open an issue to discuss your idea, or open a pull request with your proposed changes.

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE.txt) file for details.
