#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <iostream>
#include <set>
#include <thread>
#include <chrono>
#include <nlohmann/json.hpp>
#include <random>

typedef websocketpp::server<websocketpp::config::asio> server;
using websocketpp::connection_hdl;
using json = nlohmann::json;

class WebSocketServer {
public:
    WebSocketServer() {
        m_server.init_asio();

        m_server.set_open_handler([this](connection_hdl hdl) {
            std::cout << "Client connected\n";
            m_connections.insert(hdl);
            sendGestureMessages(hdl);
        });

        m_server.set_close_handler([this](connection_hdl hdl) {
            std::cout << "Client disconnected\n";
            m_connections.erase(hdl);
        });

        m_server.set_fail_handler([](connection_hdl hdl){
            std::cout << "Connection failed\n";
        });
    }

    void run(uint16_t port) {
        m_server.listen(port);
        m_server.start_accept();
        std::cout << "Server started on port " << port << "\n";
        m_server.run();
    }

private:
    void sendGestureMessages(connection_hdl hdl) {
        std::thread([this, hdl]() {
            std::vector<std::string> commands = {
                "Accept the request",
                "Deny the request",
                "Next slide",
                "Previous slide",
                "Zoom in",
                "Zoom out",
                "Attention",
                "Time out"
            };

            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dist(0, commands.size() - 1);

            for (int i = 0; i < 1000; ++i) {
                std::string random_command = commands[dist(gen)];

                json message = {
                    {"type", "gesture"},
                    {"command", random_command},
                    {"source", "hardware"},
                    {"clientId", "raspberry-pi-01"}
                };

                std::string msg_str = message.dump();
                websocketpp::lib::error_code ec;
                m_server.send(hdl, msg_str, websocketpp::frame::opcode::text, ec);

                if (ec) {
                    std::cerr << "Send failed: " << ec.message() << std::endl;
                    break;
                }

                std::this_thread::sleep_for(std::chrono::seconds(1));
            }

            std::cout << "Finished sending 1000 messages.\n";
        }).detach();
    }

    server m_server;
    std::set<connection_hdl, std::owner_less<connection_hdl>> m_connections;
};

int main() {
    try {
        WebSocketServer server;
        server.run(9002);
    } catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}
