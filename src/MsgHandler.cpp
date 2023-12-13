#include "MsgHandler.h"
#include <iostream>
#include <stdexcept>
#include <thread>

#define MARKER_HEAD 0xAA
#define MARKER_TAIL 0x55

MsgHandler::MsgHandler(sockpp::tcp_acceptor& acceptor) : acceptor_(acceptor), currentState(nullptr) {}

MsgHandler::~MsgHandler() {
    delete currentState;
}

void* connectionHandler(void* arg) {
    MsgHandler* msgHandler = static_cast<MsgHandler*>(arg);

    // Gọi đối tượng MsgHandler để xử lý gói tin từ client
    msgHandler->parseMsgClient(msgHandler->getCurrentSocket());

    pthread_exit(nullptr);
}
void MsgHandler::handleConnections() {
    while (true) {
        sockpp::inet_address peer;
        currentSocket = acceptor_.accept(&peer);

        if (!currentSocket) {
            std::cerr << "Error accepting incoming connection: " << acceptor_.last_error_str() << std::endl;
            continue;
        }
        std::cout << "Received a connection request from " << peer << std::endl;
//        // Tạo một luồng mới để xử lý gói tin từ client
//		std::thread([&]() {
//			// Gọi đối tượng MsgHandler để xử lý gói tin từ client
//			parseMsgClient(currentSocket);
//			currentSocket.close();
//			std::cout << "Connection closed from " << peer << std::endl;
//		}).detach();
//		//parseMsgClient(currentSocket);

        pthread_t thread;
		int result = pthread_create(&thread, nullptr, connectionHandler, this);
		if (result != 0) {
			std::cerr << "Error creating thread: " << strerror(result) << std::endl;

		}
		pthread_detach(thread);
    }
}
sockpp::tcp_socket& MsgHandler::getCurrentSocket(){
	return currentSocket;
}

void MsgHandler::parseMsgClient(sockpp::tcp_socket& socket) {

    try {
        // Đọc dữ liệu từ socket
        n_read_bytes = socket.read(buf, sizeof(buf));

        // Kiểm tra xem đã đọc đúng số byte hay không
        if (n_read_bytes < sizeof(buf)) {
            throw std::runtime_error("Not enough bytes read");
        }

        // Kiểm tra marker không khớp
        if (buf[0] != MARKER_HEAD || buf[4] != MARKER_TAIL) {
            throw std::runtime_error("Invalid markers");
        }

        // Xử lý dữ liệu và chuyển trạng thái
        switch (buf[1]) {
            case 0x1b:
                transitionToState(new SleepState());
                break;
            case 0x58:
                transitionToState(new WorkState());
                break;
            default:
                break;
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}

void MsgHandler::transitionToState(State* newState) {
    if (currentState != nullptr) {
        delete currentState;
    }
    currentState = newState;
    currentState->handle(*this); // Xử lý chuyển trạng thái ngay sau khi chuyển
}
void MsgHandler::sendDataToClient(const std::vector<char>& data) {
    try {
        // Gửi dữ liệu đến client
        int n = currentSocket.write(data.data(), data.size());
        if(n<0) {
        	std::cout << "Loi!!!" << std::endl;
        } else {
        	std::cout << "Sent data to Client" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}