#include <iostream>
#include <memory>
#include <string>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/bind/bind.hpp>
#include <nlohmann/json.hpp>

using tcp = boost::asio::ip::tcp;
namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
using json = nlohmann::json;

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
            boost::bind(&SignalingServer::onAccept, this, socket,
                boost::asio::placeholders::error));
    }

    void onAccept(std::shared_ptr<tcp::socket> socket, const boost::system::error_code& ec) {
        if (!ec) {
            std::cout << "New client connected!" << std::endl;
            startWebSocket(socket);
        } else {
            std::cerr << "Error accepting connection: " << ec.message() << std::endl;
        }
        startAccept();
    }

    void startWebSocket(std::shared_ptr<tcp::socket> socket) {
        auto ws = std::make_shared<websocket::stream<tcp::socket>>(std::move(*socket));
        boost::system::error_code ec;

        ws->accept(ec);
        if (ec) {
            std::cerr << "WebSocket handshake failed: " << ec.message() << std::endl;
            return;
        }

        startRead(ws);
    }

    void startRead(std::shared_ptr<websocket::stream<tcp::socket>> ws) {
        auto buffer = std::make_shared<beast::flat_buffer>();
        ws->async_read(*buffer,
            boost::bind(&SignalingServer::onRead, this, ws, buffer,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }

    void onRead(std::shared_ptr<websocket::stream<tcp::socket>> ws,
                std::shared_ptr<beast::flat_buffer> buffer,
                const boost::system::error_code& ec,
                std::size_t bytes_transferred) {
        if (!ec) {
            std::string message = beast::buffers_to_string(buffer->data());
            processMessage(*ws, message);
            buffer->consume(buffer->size());
            startRead(ws);
        } else {
            std::cerr << "Client disconnected: " << ec.message() << std::endl;
        }
    }

    void processMessage(websocket::stream<tcp::socket>& ws, const std::string& message) {
        try {
            std::cout << "Received message: " << message << std::endl;

            json j = json::parse(message);
            if (j.contains("type") && j["type"] == "offer") {
                std::cout << "Processing offer..." << std::endl;
                sendResponse(ws, "response_type", "response_data");
            }
        } catch (const json::parse_error& e) {
            std::cerr << "JSON parse error: " << e.what() << std::endl;
        }
    }

    void sendResponse(websocket::stream<tcp::socket>& ws, const std::string& type, const std::string& data) {
        json response = {{"type", type}, {"data", data}};
        ws.async_write(boost::asio::buffer(response.dump()),
            [](boost::system::error_code ec, std::size_t) {
                if (ec) {
                    std::cerr << "Error sending response: " << ec.message() << std::endl;
                }
            });
    }
};

int main() {
    const int port = 8080;
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
