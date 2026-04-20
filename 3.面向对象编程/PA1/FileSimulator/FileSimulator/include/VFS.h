#pragma once

#include "ClientInterface.h"
#include "FileSystem.h"

class VFS {
private:
    FileSystem* filesystem;
    void printWelcome() const;
    void printPrompt(const string& username, const string& path) const;
    void handleLogin(string& username);
    void handleRegister(string& username);
    void processUserCommands(const string& username);

public:
    VFS();
    ~VFS();
    void run();
};
