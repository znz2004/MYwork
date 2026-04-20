#pragma once

#include "FileSystem.h"

class ClientInterface {
private:
    FileSystem* filesystem;
    string username;

    // helper fuction
    std::vector<string> parseCommand(const string& cmdLine);
    bool execueCommand(const std::vector<string>& cmd);

public:
    ClientInterface(const string& username, FileSystem* filesystem);
    ~ClientInterface() = default;

    // process input command line
    void processCommand(const string& cmdLine);
  
    // show help information
    void showHelp() const;

    // file operation for user
    bool createFile(const string& name);
    bool deleteFile(const string& name);
    string readFile(const string& name);
    bool writeFile(const string& name, const string& data);
    
    
    // dir operation for user
    bool createDir(const string& name);
    bool deleteDir(const string& name, bool recursive = false);
    bool changeDir(const string& path);
    void listCurrentDir();

    // helper fuction for user
    string getCurrentPath() const;
    string search(const string& name, const string& type);
};