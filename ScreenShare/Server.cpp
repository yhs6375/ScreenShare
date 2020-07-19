#include "Server.h"


using namespace std::chrono_literals;

namespace HS {
	namespace RAT_Server {
		class ServerImpl {
		public:
			std::shared_ptr<Clipboard::IClipboard_Manager> Clipboard_;
			std::shared_ptr<IRATServer> ratServer;

			RAT::Server_Status Status_ = RAT::Server_Status::SERVER_STOPPED;
			mutable std::shared_mutex ClientsLock;
			mutable std::shared_mutex monitorMutex;
			std::vector<std::shared_ptr<Client>> Clients;
			std::vector<Screen_Capture::Monitor> AllMonitors;


			RAT::ClipboardSharing ShareClip = RAT::ClipboardSharing::NOT_SHARED;
			
			
			RAT::ImageEncoding EncodeImagesAsGrayScale = RAT::ImageEncoding::COLOR;
			

			ServerImpl()
			{
				Status_ = RAT::Server_Status::SERVER_STOPPED;
			}

			virtual ~ServerImpl()
			{
				Status_ = RAT::Server_Status::SERVER_STOPPED;
			}
			void SendtoAll(const Socket::WSMessage &msg)
			{
				std::shared_lock<std::shared_mutex> lock(ClientsLock);
				for (const auto &a : Clients) {
					a->socket->send(msg, Socket::CompressionOptions::NO_COMPRESSION);
				}
			}
			void Run(unsigned short port)
			{
				Status_ = RAT::Server_Status::SERVER_RUNNING;
				auto listener = Socket::CreateListener(port);
				ratServer = RAT_Server::CreateServerDriverConfiguration()
					->onConnection([&](const std::shared_ptr<Socket::Socket> &socket) {
					printf("client bind connect to server.\n");
					
					auto c = std::make_shared<Client>();
					c->socket = socket;
					std::unique_lock<std::shared_mutex> lock(ClientsLock);
					Clients.push_back(c);
				})
					->onMessage([&](const std::shared_ptr<Socket::Socket> &socket, const Socket::WSMessage &msg) {
					printf("@@@@@@@@@@@@@@@%s\n", msg.data);
				})
					->onDisconnection([&](const std::shared_ptr<Socket::Socket> &socket, unsigned short code, const std::string &msg) {
					std::unique_lock<std::shared_mutex> lock(ClientsLock);
					Clients.erase(std::remove_if(std::begin(Clients), std::end(Clients), [&](auto &s) { return s->socket == socket; }),
						std::end(Clients));
				})
					->onMonitorsChanged([&](const std::shared_ptr<Socket::Socket> &socket, const std::vector<Screen_Capture::Monitor> & monitors) {
					auto clientIt = std::find_if(std::begin(Clients), std::end(Clients), [&](std::shared_ptr<Client> const &client) { return client->socket == socket; });
					std::shared_ptr<Client> client;
					if (clientIt != Clients.end()) {
						//모니터 mutex 활성화
						//unique_lock은 shared_lock이나 다른 unique_lock도 접근하지 못하게 한다.
						std::unique_lock<std::shared_mutex> monitorLock(monitorMutex);
						client = *clientIt;
						client->monitors.clear();
						client->monitorsToWatch.clear();
						for(auto monitor : monitors) {
							client->monitors.push_back(monitor);
							for (auto id : client->monitorsToWatchId) {
								if (monitor.Id == id) {
									client->monitorsToWatch.push_back({});
									auto monitorToWatchEnd = --(client->monitorsToWatch.end());
									monitorToWatchEnd->windowStruct = Interface::MakeNewWindow();
									monitorToWatchEnd->windowStruct->CreateWindowAndShow();
									monitorToWatchEnd->monitor = client->monitors[client->monitors.size() - 1];
								}
							}
						}
						RAT::ServerSettings serverSettings;
						serverSettings.ShareClip = ShareClip;
						serverSettings.EncodeImagesAsGrayScale = EncodeImagesAsGrayScale;
						for (auto monitorToWatchId : client->monitorsToWatchId) {
							serverSettings.MonitorsToWatchId.push_back(monitorToWatchId);
						}
						ratServer->SendServerSetting(socket, serverSettings);
						//모니터 mutex 언락
						monitorLock.unlock();
					}
				})
					->onFrameChanged([&](const std::shared_ptr<Socket::Socket> &socket, const RAT::Image &image, const Screen_Capture::Monitor &monitor) {
					
				})
					->onNewFrame([&](const std::shared_ptr<Socket::Socket> &socket, const RAT::Image &image, const Screen_Capture::Monitor &monitor) {
					//shared_lock을걸면 다른 shared_lock도 접근할 수 있다.
					std::shared_lock<std::shared_mutex> monitorLock(monitorMutex);
					auto clientIt = std::find_if(std::begin(Clients), std::end(Clients), [&](std::shared_ptr<Client> const &client) { return client->socket == socket; });
					std::shared_ptr<Client> client;
					if (clientIt != Clients.end()) {
						client = *clientIt;
						for (auto watchMonitor : client->monitorsToWatch) {
							if (watchMonitor.monitor.Id == monitor.Id) {
								//받아온 이미지데이터를 HBITMAP으로 변환
								RECT r = { image.Rect_.Origin.X, image.Rect_.Origin.Y, image.Rect_.Width, image.Rect_.Height };
								watchMonitor.windowStruct->DrawScreen(Util::BytesToBitmap((unsigned char *)image.Data, image.Rect_.Width, image.Rect_.Height), &r, true);
							}
						}
					}
					
					
					//Util::HBITMAP2BMP(windowStruct->hBitmap, "c:\\aa.bmp");
					/*BITMAPINFO bmpInfo;
					memset(&bmpInfo, 0, sizeof(BITMAPINFOHEADER));
					bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
					GetDIBits(GetDC(windowStruct->hWnd), windowStruct->hBitmap, 0, 0, NULL, &bmpInfo, DIB_RGB_COLORS);
					bmpInfo.bmiHeader.biCompression = BI_RGB;
					BYTE* pImageData = new BYTE[bmpInfo.bmiHeader.biSizeImage];
					int a = GetDIBits(GetDC(windowStruct->hWnd), windowStruct->hBitmap, 0, bmpInfo.bmiHeader.biHeight, pImageData, &bmpInfo, DIB_RGB_COLORS);
					printf("%d\n", a);
					printf("%d\n", pImageData[0]);*/
				})
					->onMouseImageChanged([&](const RAT::Image &image) {
					
				})
					->onMousePositionChanged([&](const RAT::Point &point) {
					
				})
					->onClipboardChanged([&](const std::shared_ptr<Socket::Socket> &socket, const std::string &clipmsg) {
					ratServer->SendClipboardChanged(socket,clipmsg);
				})
					->Build(listener);
				listener->listen();
			}
		};

		Server::Server() { ServerImpl_ = new ServerImpl(); }
		Server::~Server() {
			serverThread->detach();
			delete ServerImpl_;
			delete serverThread;
		}
		void Server::MaxConnections(int maxconnections)
		{
			if (ServerImpl_->ratServer) {
				ServerImpl_->ratServer->MaxConnections(maxconnections);
			}
		}
		int Server::MaxConnections() const
		{
			if (ServerImpl_->ratServer) {
				return ServerImpl_->ratServer->MaxConnections();
			}
			return 0;
		}

		std::shared_ptr<IRATServerConfiguration> CreateServerDriverConfiguration()
		{
			return std::make_shared<RATServerConfiguration>(std::make_shared<RATServer>());
		}
		void Server::Run(unsigned short port)
		{
			this->port = port;
			this->running = true;
			serverThread = new std::thread(&Server::threadProc, this);
			//serverThread->join();
		}
		void Server::Close() {

		}
		void Server::threadProc() {
			ServerImpl_->Run(port);
			/*while (ServerImpl_->Status_ == RAT::Server_Status::SERVER_RUNNING) {
				std::this_thread::sleep_for(50ms);
			}*/
		}
	} // namespace RAT_Server
}
