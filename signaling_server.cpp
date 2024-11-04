#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <nlohmann/json.hpp>

using boost::asio::ip::tcp;
using json = nlohmann::json;

class SignalingServer {
public:
    SignalingServer(boost::asio::io_context& io_context, int port)
        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
        startAccept();
    }

private:
    void startAccept() {
        auto socket = std::make_shared<tcp::socket>(acceptor_.get_executor());
        acceptor_.async_accept(*socket, [this, socket](boost::system::error_code ec) {
            if (!ec) {
                std::cout << "New client connected!" << std::endl;
                startRead(socket);
            }
            startAccept();
        });
    }

    void startRead(std::shared_ptr<tcp::socket> socket) {
        auto buffer = std::make_shared<std::array<char, 1024>>();
        socket->async_read_some(boost::asio::buffer(*buffer), [this, socket, buffer](boost::system::error_code ec, std::size_t length) {
            if (!ec) {
                std::string message(buffer->data(), length);
                processMessage(socket, message);
                startRead(socket);
            } else {
                std::cerr << "Client disconnected." << std::endl;
            }
        });
    }

    void processMessage(std::shared_ptr<tcp::socket> socket, const std::string& message) {
        json j = json::parse(message);
        if (j.contains("type") && j["type"] == "offer") {
            std::cout << "Received offer: " << j.dump() << std::endl;
            sendResponse(socket, "answer", "This is a sample answer.");
        }
    }

    void sendResponse(std::shared_ptr<tcp::socket> socket, const std::string& type, const std::string& data) {
        json response = {{"type", type}, {"data", data}};
        boost::asio::async_write(*socket, boost::asio::buffer(response.dump()), [](boost::system::error_code ec, std::size_t) {
            if (ec) {
                std::cerr << "Failed to send response." << std::endl;
            }
        });
    }

    tcp::acceptor acceptor_;
};

int main() {
    try {
        boost::asio::io_context io_context;
        SignalingServer server(io_context, 8080);
        std::cout << "Signaling server started." << std::endl;
        io_context.run();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    return 0;
}
