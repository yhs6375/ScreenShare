#pragma once
#include "ServerDriver.h"
#include <string>
#include <assert.h>
#include <chrono>
#include <mutex>
#include <string.h>
#include <thread>
#include <vector>
#include "Clipboard/Clipboard.h"
#include "Util/ImageUtils.h"
#include "Interface/Interface.h"

namespace HS {
	namespace RAT_Server {
		struct MonitorToWatch {
			Screen_Capture::Monitor monitor;
			std::shared_ptr<Interface::WindowStruct> windowStruct;
		};
		struct Client {
			std::shared_ptr<Socket::Socket> socket;
			std::vector<Screen_Capture::Monitor> monitors;
			std::vector<MonitorToWatch> monitorsToWatch;
			std::vector<int> monitorsToWatchId = { 0 };
		};
		class ServerImpl;
		class Server {
			ServerImpl *ServerImpl_;
			std::thread *serverThread;
			bool running = false;
		public:
			int port;
		public:
			Server();
			~Server();
			void MaxConnections(int maxconnections);
			int MaxConnections() const;

			void Run(unsigned short port);
			void Close();
			void threadProc();
		};
	}
}
