#include "VFS.h"

VFS::VFS() {
    filesystem = new FileSystem("root", InodeFactory::generateInode());
}

VFS::~VFS() {
    delete filesystem;
}

void VFS::printWelcome() const {
    std::cout << "File System Simulator Started" << std::endl;
    std::cout << "Please login with your username" << std::endl;
}

void VFS::printPrompt(const string& username, const string& path) const {
    std::cout << username << "@FileSimulator:" << path << "$ ";
}

void VFS::handleLogin(string& username) {
    std::cout << "User Login (Please input your username): \n";
    std::getline(std::cin, username);
    
    if(!filesystem->hasUser(username)) {
        throw std::runtime_error("User does not exist. Please register first.");
    }
}

void VFS::handleRegister(string& username) {
    std::cout << "Register new user (Please input desired username): \n";
    std::getline(std::cin, username);
    
    if(filesystem->hasUser(username)) {
        throw std::runtime_error("Username already exists. Please try another one.");
    }
    
    if(!filesystem->registerUser(username)) {
        throw std::runtime_error("Failed to register user.");
    }
    std::cout << "User registered successfully!\n";
}

void VFS::processUserCommands(const string& username) {
    ClientInterface client(username, filesystem);
    string command;
    
    while(true) {
        printPrompt(username, filesystem->getCurrentPath());
        std::getline(std::cin, command);

        if (command == "quit")
        {
            break;
        }

        if (command == "exit")
        {
            filesystem->setUser("");
            std::cout << std::endl << "# exit the program" << std::endl;
            std::cout << "User Login (Please input who you are):" << std::endl;
            std::getline(std::cin, command);
            std::cout << "Bye!" << std::endl;
            std::getchar();
            break;
        }

        try {
            client.processCommand(command);
        } catch(const std::exception& e) {
            std::cout << "Error: " << e.what() << std::endl;
        }
    }
}

void VFS::run() {
    string username, command;
    bool running = true;

    while(running) {
        // Clear screen first
        #ifdef _WIN32
            system("cls");
        #else
            system("clear");
        #endif

        std::cout << "============================\n";
        std::cout << "  File System Simulator\n";
        std::cout << "============================\n";
        std::cout << "1. Login\n2. Register\n3. Exit\nPlease choose (1-3): ";
        std::string choice;
        std::getline(std::cin, choice);

        try {
            if(choice == "3" || choice == "exit" || choice == "quit" || choice == "q") {
                running = false;
                continue;
            }

            // Clear screen second
            #ifdef _WIN32
                system("cls");
            #else
                system("clear");
            #endif

            if(choice == "1" || choice == "login" || choice == "l" || choice == "L") {
                handleLogin(username);
            }
            else if(choice == "2" || choice == "register" || choice == "r" || choice == "R") {
                handleRegister(username);
            }
            else {
                std::cout << "Invalid choice. Please try again.\n";
                std::cout << "Press Enter to continue...";
                std::cin.get();
                continue;
            }

            if(!filesystem->setUser(username)) {
                std::cout << "Failed to set user: " << username << std::endl;
                std::cout << "Press Enter to continue...";
                std::cin.get();
                continue;
            }

            // Clear screen third
            #ifdef _WIN32
                system("cls");
            #else
                system("clear");
            #endif

            processUserCommands(username);

        } catch(const std::exception& e) {
            std::cout << "Error: " << e.what() << std::endl;
            std::cout << "Press Enter to continue...";
            std::cin.get();
        }
    }

    std::cout << "\nThank you for using File System Simulator!\nBye!" << std::endl;
}
