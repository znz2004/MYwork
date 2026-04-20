#include "ClientInterface.h"

ClientInterface::ClientInterface(const string& username, FileSystem* filesystem)
    : filesystem(filesystem), username(username) {
    // no change
}

std::vector<string> ClientInterface::parseCommand(const string& cmdLine) {
    std::vector<string> tokens;
    std::istringstream cmd_stream(cmdLine);
    string token, current;
    bool inQuotes = false;
    while (cmd_stream >> std::ws)
    {
        char ch = cmd_stream.peek();
        if (ch == '"')
        {
            inQuotes = !inQuotes;
            cmd_stream.get();
        }

        if (inQuotes)
        {
            char c;
            while (cmd_stream.get(c))
            {
                if (c == '"') break;
                current += c;
            }
        }
        else
        {
            cmd_stream >> token;
            current = token;
        }

        tokens.push_back(current);
        current.clear();
    }

    return tokens;
}

bool ClientInterface::execueCommand(const std::vector<string>& cmd) {
    if (cmd.empty())
    {
        return false;
    }

    const string& command = cmd[0];
    if (command == "create")
    {
        for (int i = 1; i < (int)cmd.size(); ++i)
        {
            if (!createFile(cmd[i]))
            {
                return false;
            }
        }

        return true;
    }
    else if (command == "delete")
    {
        if (cmd.size() != 2)
        {
            return false;
        }

        return deleteFile(cmd[1]);
    }
    else if (command == "read")
    {
		for (int i = 1; i < (int)cmd.size(); ++i)
		{
			std::cout << "=== " << cmd[i] << " ===" << std::endl;
			std::cout << readFile(cmd[i]) << std::endl;
		}

        return true;
    }
    else if (command == "write")
    {
        if (cmd.size() <  2)
        {
            return false;
        }

        for (int i = 2; i < (int)cmd.size(); ++i)
        {
            if (!writeFile(cmd[1], cmd[i]))
            {
                return false;
            }
        }
        
        return true;
    }
    else if (command == "mkdir")
    {
        if (cmd.size() != 2)
        {
            return false;
        }

        return createDir(cmd[1]);
    }
    else if (command == "rmdir")
    {
        if (cmd.size() == 3)
        {
            return deleteDir(cmd[2], cmd[1] == "-r");
        }
        else if(cmd.size() == 2)
        {
            return deleteDir(cmd[1], false);
        }
        
        return false;
    }
    else if (command == "cd")
    {
        if (cmd.size() != 2)
        {
            return false;
        }

        return changeDir(cmd[1]);
    }
    else if (command == "ls")
    {
        listCurrentDir();
        return true;
    }
    else if (command == "pwd")
    {
        std::cout << getCurrentPath() << std::endl;
    }
    else if (command == "whoami")
    {
        std::cout << filesystem->getUserName() << std::endl;
    }
    else if (command == "clear")
    {
        #ifdef _WIN32
		        system("cls");
        #else
		        system("clear");
        #endif
    }
    else if (command == "help")
    {
        showHelp();
    }
    else
    {
        return false;
    }

    return true;
}

void ClientInterface::processCommand(const string& cmdLine) {
    try 
    {
        std::vector<string> args = parseCommand(cmdLine);
        if (args.empty())
        {
            return;
        }

        bool is_suc = execueCommand(args);
        if (!is_suc)
        {
            std::cerr << "Error: Command execue failed " << cmdLine << std::endl;
        }
    }
    catch (const std::exception e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}

void ClientInterface::showHelp() const {
    // Show help message with all available commands and their usage, no return value
    std::cout << "Available commands:\n"
         << "  create <filename...>      - Create one or more new files\n"
         << "  delete <filename...>      - Delete one or more files\n"
         << "  read <filename...>        - Read content from one or more files\n"
         << "  write <filename> <text>   - Write text to file (supports '\\n' for newline)\n"
         << "  mkdir <dirname>           - Create a new directory\n"
         << "  rmdir [-r] <dirname>      - Remove directory (-r for recursive deletion)\n"
         << "  cd <path>                - Change directory (supports relative/absolute paths)\n"
         << "  ls                       - List current directory contents\n"
         << "  pwd                      - Show current working directory\n"
         << "  whoami                   - Show current user name\n"
         << "  clear                    - clear current command line\n"
         << "  help                     - Show this help message\n"
         << "  exit                     - Logout current user\n"
         << "  quit                     - Exit program\n" << std::endl;
}


bool ClientInterface::createFile(const string& name) {
    if (name.empty())
    {
        std::cout << "name is empty, create file error!" << std::endl;
        return false;
    }

    if (name.find('/') != string::npos || name.find('\\') != string::npos) 
    {
        std::cout << "name is invalid, create file error!" << std::endl;
        return false;
    }

    return filesystem->createFile(name);
}

bool ClientInterface::deleteFile(const string& name) {
    if (name.empty())
    {
        std::cout << "name is empty, delete file error!" << std::endl;
        return false;
    }

    return filesystem->deleteFile(name, username);
}

string ClientInterface::readFile(const string& name) {
    if (name.empty())
    {
        return "";
    }

    FileObj* file_obj = filesystem->resolvePath(name);
    if (file_obj == 0)
    {
        return "";
    }

    File* file = dynamic_cast<File*>(file_obj);
    if (file == 0)
    {
        return "";
    }

    return file->getContent();
}

bool ClientInterface::writeFile(const string& name, const string& data) {
    if (name.empty())
    {
        std::cout << "name is empty, write file error!" << std::endl;
        return false;
    }

    FileObj* file_obj = filesystem->resolvePath(name);
    if (file_obj == 0)
    {
        std::cout << "write file error!" << std::endl;
        return false;
    }

    File* file = dynamic_cast<File*>(file_obj);
    if (file == 0)
    {
        std::cout << "write file error!" << std::endl;
        return false;
    }

    return file->write(data);
}

bool ClientInterface::createDir(const string& name) {
    if (name.empty())
    {
        std::cout << "name is empty, createDir error!" << std::endl;
        return false;
    }

    return filesystem->createDir(name);
}

bool ClientInterface::deleteDir(const string& name, bool recursive) {
    if (name.empty())
    {
        std::cout << "name is empty, deleteDir error!" << std::endl;
        return false;
    }

    return filesystem->deleteDir(name, username, recursive);
}

bool ClientInterface::changeDir(const string& path) {
    if (path.empty())
    {
        std::cout << "name is empty, changeDir error!" << std::endl;
        return false;
    }

    FileObj* target = filesystem->resolvePath(path);
    if (target == 0) 
    {
        std::cout << "changeDir error!" << std::endl;
        return false;
    }

    Directory* target_dir = dynamic_cast<Directory*>(target);
    if (target_dir == 0)
    {
        std::cout << "changeDir error!" << std::endl;
        return false;
    }

    return filesystem->changeDir(target_dir->getInode());
}

void ClientInterface::listCurrentDir() {
    Directory* cur_dir = filesystem->getCurrentDir();
    if (cur_dir == 0)
    {
        return;
    }

    std::vector<FileObj*> all_child;
    all_child = cur_dir->getAll();
    for (size_t i = 0; i < (int)all_child.size(); i++)
    {
        std::cout << all_child[i]->getName() << std::endl;
    }

    return;
}

string ClientInterface::getCurrentPath() const {
    return filesystem->getCurrentPath();
}

string ClientInterface::search(const string& name, const string& type) {
    uint64_t inode = filesystem->search(name, type);
    if (inode == 0)
    {
        return "Not Found: " + name ;
    }

    return "Found: " + name + " (inode: " + std::to_string(inode) + ")";
}
