#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/bind/bind.hpp>
#include <nlohmann/json.hpp>

using tcp = boost::asio::ip::tcp;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
using json = nlohmann::json;

class ClientSession : public std::enable_shared_from_this<ClientSession> {
public:
    ClientSession(tcp::socket socket)
        : ws_(std::move(socket)) {}

    void start() {
        auto client_endpoint = ws_.next_layer().remote_endpoint();
        client_ip_ = client_endpoint.address().to_string();
        client_port_ = client_endpoint.port();

        std::cout << "New client connected: IP " << client_ip_
                  << ", Port " << client_port_ << std::endl;

        ws_.async_accept(boost::bind(&ClientSession::onAccept, shared_from_this(), boost::asio::placeholders::error));
    }

private:
    websocket::stream<tcp::socket> ws_;
    std::string client_ip_;
    unsigned short client_port_;

    void onAccept(const boost::system::error_code& ec) {
        if (!ec) {
            startRead();
        } else {
            std::cerr << "WebSocket handshake failed: " << ec.message() << std::endl;
        }
    }

    void startRead() {
        auto buffer = std::make_shared<beast::flat_buffer>();
        ws_.async_read(*buffer,
            boost::bind(&ClientSession::onRead, shared_from_this(), buffer,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }

    void onRead(std::shared_ptr<beast::flat_buffer> buffer,
                const boost::system::error_code& ec,
                std::size_t bytes_transferred) {
        if (!ec) {
            std::string message = beast::buffers_to_string(buffer->data());
            std::cout << "Received message from IP " << client_ip_
                      << ", Port " << client_port_ << ": " << message << std::endl;

            buffer->consume(buffer->size());
            startRead();
        } else if (ec == websocket::error::closed) {
            std::cout << "Client disconnected: IP " << client_ip_
                      << ", Port " << client_port_ << std::endl;
        } else {
            std::cerr << "Error during read: " << ec.message() << std::endl;
        }
    }
};

class SignalingServer {
public:
    SignalingServer(boost::asio::io_context& ioc, unsigned short port)
        : acceptor_(ioc, tcp::endpoint(tcp::v4(), port)) {
        startAccept();
    }

private:
    tcp::acceptor acceptor_;

    void startAccept() {
        auto socket = std::make_shared<tcp::socket>(acceptor_.get_executor());
        acceptor_.async_accept(*socket,
            [this, socket](const boost::system::error_code& ec) {
                if (!ec) {
                    std::make_shared<ClientSession>(std::move(*socket))->start();
                } else {
                    std::cerr << "Error accepting connection: " << ec.message() << std::endl;
                }
                startAccept();
            });
    }
};

int main() {
    const int port = 8765;
    try {
        boost::asio::io_context io_context;
        SignalingServer server(io_context, port);
        std::cout << "Signaling server started on " << port << std::endl;
        io_context.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
