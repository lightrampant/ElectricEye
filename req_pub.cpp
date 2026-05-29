#include <iostream>
#include <cstring>
#include <thread>
#include <atomic>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <termios.h>
#include <unistd.h>
#include <iostream>
#include <fstream>

void printTimestamp();
void hexDump(const unsigned char* data, int length);

bool showHexDump = 0;
bool showTimeStamp = 1;
bool enableLogs = 0;

class ListeningPost{

		private:

				int sockfd;
				int port;
				std::atomic<bool> running;
				sockaddr_in serverAddr;

		public:

				ListeningPost(int listenPort){

						port = listenPort;
						running = true;

						sockfd = socket(AF_INET, SOCK_DGRAM, 0);

						if (sockfd < 0){
								std::cerr << "Socket creation failed\n";
								exit(1);
						}

						memset(&serverAddr, 0, sizeof(serverAddr));

						serverAddr.sin_family = AF_INET;
						serverAddr.sin_addr.s_addr = INADDR_ANY;
						serverAddr.sin_port = htons(port);

						if (bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0){
								std::cerr << "Bind failed\n";
								exit(1);
						}

						std::cout << "Listening on port " << port << std::endl;
						std::cout << "Press Q then ENTER to quit.\n";
				}

				void listenLoop(){
						char buffer[1024];
						sockaddr_in clientAddr;
						socklen_t addrLen = sizeof(clientAddr);

						while (running){
								int bytes = recvfrom(
												sockfd,
												buffer,
												sizeof(buffer) - 1,
												MSG_DONTWAIT,
												(struct sockaddr*)&clientAddr,
												&addrLen
												);

								if (bytes > 0){
										buffer[bytes] = '\0';
										if(showTimeStamp){
												printTimestamp();
										}
										std::cout << "Received from "
												<< inet_ntoa(clientAddr.sin_addr)
												<< ":" << ntohs(clientAddr.sin_port)
												<< " -> " << buffer << std::endl
												<< "Size: " << bytes << " bytes\n";

										if(showHexDump){
												std::cout << "\nHex Dump (" << bytes << " bytes)\n";
												hexDump((unsigned char*)buffer, bytes);
										}
								}

								usleep(10000); 
						}
				}

				void keyboardMonitor(){
						char input;

						while (running){
								std::cin >> input;

								if (input == 'q' || input == 'Q'){
										std::cout << "Stopping listener...\n";
										running = false;
								}
						}
				}

				void startListening(){
						std::thread listener(&ListeningPost::listenLoop, this);
						std::thread keyboard(&ListeningPost::keyboardMonitor, this);

						listener.join();
						keyboard.join();
				}

				~ListeningPost(){
						close(sockfd);
				}
};

void enableRawMode(){

		termios term;

		tcgetattr(STDIN_FILENO, &term);

		term.c_lflag &= ~(ICANON | ECHO);

		tcsetattr(STDIN_FILENO, TCSAFLUSH, &term);
}

void disableRawMode(){

		termios term;

		tcgetattr(STDIN_FILENO, &term);

		term.c_lflag |= (ICANON | ECHO);

		tcsetattr(STDIN_FILENO, TCSAFLUSH, &term);
}

void toggleSettings(bool &showHexDump, bool &showTimeStamp, bool &enableLogs){

		std::cin.clear();
		std::cin.ignore(1000, '\n');

		int selection = 0;
		const int optionCount = 3;

		enableRawMode();

		while(true){

				std::cout << "\033[2J\033[H"; 

				std::cout << "ElectricEye Settings\n";
				std::cout << "--------------------\n\n";

				std::cout << (selection==0?"> ":"  ")
						<< "Toggle HexDump     [" << (showHexDump?"ON":"OFF") << "]\n";

				std::cout << (selection==1?"> ":"  ")
						<< "Toggle Timestamp   [" << (showTimeStamp?"ON":"OFF") << "]\n";

				std::cout << (selection==2?"> ":"  ")
						<< "Return to Main Menu\n";

				char c = getchar();

				if(c == '\033'){
						getchar(); 
						switch(getchar()){

								case 'A': 
										selection--;
										if(selection < 0)
												selection = optionCount - 1;
										break;

								case 'B': 
										selection++;
										if(selection >= optionCount)
												selection = 0;
										break;
						}
				}

				else if(c == '\n'){
						if(selection == 0)
								showHexDump = !showHexDump;

						else if(selection == 1)
								showTimeStamp = !showTimeStamp;

						else if(selection == 2)
								break;
				}
		}

		disableRawMode();
}

// Additional Helper Functions
std::string generateRandomMessage(int length){
		const std::string chars =
				"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
				"abcdefghijklmnopqrstuvwxyz"
				"0123456789";

		std::string result;

		for(int i = 0; i < length; i++){
				result += chars[rand() % chars.size()];
		}

		return result;
}

void hexDump(const unsigned char* data, int length){
		for (int i = 0; i < length; i++){
				if (i % 16 == 0)
						printf("%04x  ", i);

				printf("%02x ", data[i]);

				if (i % 16 == 15 || i == length - 1){
						int j;

						for (j = (i % 16); j < 15; j++)
								printf("   ");

						printf(" ");

						int start = i - (i % 16);
						int end = i;

						for (j = start; j <= end; j++){
								if (data[j] >= 32 && data[j] <= 126)
										printf("%c", data[j]);
								else
										printf(".");
						}

						printf("\n");
				}
		}
}

void printTimestamp(){
		auto now = std::chrono::system_clock::now();
		time_t time = std::chrono::system_clock::to_time_t(now);

		std::cout << "["
				<< std::put_time(localtime(&time), "%H:%M:%S")
				<< "] ";
}

// Usable Functions
void udpRepeatSender(){
		std::string ip;
		int port;
		std::string message;
		int delayMs;
		char randomMode;    

		std::cout << "==========================\n";
		std::cout << "====    UDP Cannon     ===\n";
		std::cout << "==========================\n";


		std::cout << "\nEnter destination IP: ";
		std::cin >> ip;

		std::cout << "Enter destination port: ";
		std::cin >> port;

		std::cin.ignore();

		std::cout << "Enter message to repeatedly send: ";
		std::getline(std::cin, message);

		std::cout << "Use random messages? (y/n): ";
		std::cin >> randomMode;

		std::cout << "Delay between packets (ms): ";
		std::cin >> delayMs;

		std::atomic<bool> running(true);

		int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

		if(sockfd < 0){
				std::cout << "Socket creation failed\n";
				return;
		}

		sockaddr_in destAddr{};
		destAddr.sin_family = AF_INET;
		destAddr.sin_port = htons(port);

		inet_pton(AF_INET, ip.c_str(), &destAddr.sin_addr);


		std::thread sender([&](){
						int packetsSent = 0;

						while(running){
						std::string packetData;

						if(randomMode == 'y' || randomMode == 'Y')
						packetData = generateRandomMessage(16);

						else
						packetData = message;

						sendto(
										sockfd,
										packetData.c_str(),
										packetData.size(),
										0,
										(sockaddr*)&destAddr,
										sizeof(destAddr)
							  );

						packetsSent++;

						std::cout << "\rPackets Sent: "
								<< packetsSent
								<< " | Last Packet: "
								<< packetData
								<< std::flush;

						usleep(delayMs * 1000);
						}
		}
		);

		std::thread keyboard([&](){
						char input;

						std::cout << "\n\nPress Q then ENTER to stop.\n";

						while(running){
						std::cin >> input;

						if(input == 'q' || input == 'Q'){
						running = false;
						}
						}
						});

		sender.join();
		keyboard.join();

		close(sockfd);

		std::cout << "\nRepeat sender stopped.\n";
}

void modeListen(){

		int port;

		std::cout << "==========================\n";
		std::cout << "====UDP Listening Post====\n";
		std::cout << "==========================\n";

		std::cout << "\nEnter port to listen on: ";
		std::cin >> port;

		ListeningPost receiver(port);

		receiver.startListening();
}

void modeSend(){

		std::string ip;
		int port;

		std::cout << "==========================\n";
		std::cout << "======UDP Send Mode=======\n";
		std::cout << "==========================\n";

		std::cout << "\nEnter destination IP: ";
		std::cin >> ip;

		std::cout << "Enter destination port: ";
		std::cin >> port;

		int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

		if (sockfd < 0){
				std::cout << "Socket creation failed\n";
				return;
		}

		sockaddr_in destAddr;
		memset(&destAddr, 0, sizeof(destAddr));

		destAddr.sin_family = AF_INET;
		destAddr.sin_port = htons(port);
		inet_pton(AF_INET, ip.c_str(), &destAddr.sin_addr);

		std::cin.ignore();

		while (true){
				std::string message;

				std::cout << "\nEnter message (Q to quit): ";
				std::getline(std::cin, message);

				if (message == "Q" || message == "q"){
						std::cout << "Returning to main menu...\n";
						break;
				}

				sendto(
								sockfd,
								message.c_str(),
								message.size(),
								0,
								(sockaddr*)&destAddr,
								sizeof(destAddr)
					  );

				std::cout << "Message sent.\n"; 
		}	

		close(sockfd); 

}

void modeHTTP(){

		std::string host;
		std::string path;

		std::cout << "==========================\n";
		std::cout << "====HTTP Requests Mode====\n";
		std::cout << "==========================\n";

		std::cout << "\nEnter hostname (example: example.com) or Q to quit: ";
		std::cin >> host;

		if (host == "Q" || host == "q")
				return;

		std::cout << "Enter path (example: / ): ";
		std::cin >> path;

		struct hostent* server = gethostbyname(host.c_str());

		if (server == nullptr){
				std::cout << "Error resolving host\n";
				return;
		}

		int sockfd = socket(AF_INET, SOCK_STREAM, 0);

		if (sockfd < 0){
				std::cout << "Socket creation failed\n";
				return;
		}

		sockaddr_in serverAddr{};
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(80);

		memcpy(&serverAddr.sin_addr.s_addr, server->h_addr, server->h_length);

		if (connect(sockfd, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0){
				std::cout << "Connection failed\n";
				close(sockfd);
				return;
		}

		std::string request =
				"GET " + path + " HTTP/1.1\r\n"
				"Host: " + host + "\r\n"
				"Connection: close\r\n\r\n";

		send(sockfd, request.c_str(), request.size(), 0);

		char buffer[4096];
		int bytes;

		std::cout << "\n=== HTTP Response ===\n" << std::endl;

		while ((bytes = recv(sockfd, buffer, sizeof(buffer)-1, 0)) > 0){
				buffer[bytes] = '\0';
				std::cout << buffer;
		}

		std::cout << "\n\n=== End Response ===\n";

		close(sockfd);
}

void portScanner(){
		std::string targetIP;
		int startPort, endPort;

		std::cout << "==================\n";
		std::cout << "== Port Scanner ==\n";
		std::cout << "==================\n";

		std::cout << "Target IP: ";
		std::cin >> targetIP;

		std::cout << "Start Port: ";
		std::cin >> startPort;

		std::cout << "End Port: ";
		std::cin >> endPort;

		std::cout << "\nScanning...\n";

		for(int port = startPort; port <= endPort; port++){
				int sock = socket(AF_INET, SOCK_STREAM, 0);

				sockaddr_in target{};
				target.sin_family = AF_INET;
				target.sin_port = htons(port);
				inet_pton(AF_INET, targetIP.c_str(), &target.sin_addr);

				int result = connect(sock, (sockaddr*)&target, sizeof(target));

				if(result == 0){
						std::cout << "Port " << port << " OPEN" << std::endl;
				}

				close(sock);
		}	

		std::cout << "\nScan complete.\n";
}

void titleScreen(){


		std::cout << "[====================================]\n";
		std::cout << "          Electric Eye v0.1           \n";
		std::cout << "      UDP • HTTP • Socket Utility     \n";
		std::cout << "[====================================]\n";

}

int main (){

		srand(time(nullptr));

		bool mainLoopStatus = true;

		titleScreen();

		while (mainLoopStatus){
				int userMenuSelection;

				std::cout << "\nSelect a function: \n";
				std::cout << "1. UDP Listener\n";
				std::cout << "2. UDP Sender\n";
				std::cout << "3. UDP Cannon\n";    
				std::cout << "4. HTTP Banner Grabber\n";
				std::cout << "5. Simple Port Scanner\n";
				std::cout << "6. Settings\n";
				std::cout << "7. Exit Program\n";
				std::cout << ">> ";
				std::cin >> userMenuSelection;

				if (std::cin.fail()){
						std::cin.clear();
						std::cin.ignore(1000, '\n');
						std::cout << "Invalid input, returning to Main Menu\n";
						continue;
				}
				else if (userMenuSelection == 1){
						modeListen();
				}
				else if (userMenuSelection == 2){ 
						modeSend();
				}
				else if (userMenuSelection == 3){ 
						udpRepeatSender();	
				}
				else if (userMenuSelection == 4){ 
						modeHTTP();
				}
				else if (userMenuSelection == 5){ 
						portScanner();	
				}
				else if (userMenuSelection == 6){ 
						toggleSettings(showHexDump, showTimeStamp, enableLogs);
				}	
				else if(userMenuSelection == 7){ 
						break;
				}
				else{
						std::cout << "Invalid input, returning to Main Menu\n";
				}
		}
}
