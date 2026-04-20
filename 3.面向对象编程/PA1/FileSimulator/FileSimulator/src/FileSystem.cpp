#include "FileSystem.h"
#include <sstream>
#include <string>

FileSystem::FileSystem(const string& username, const uint64_t& inode)
    : root(new Directory("", username, inode, nullptr)), cur(root), username(username) {
    // no change
    users.insert(username);
}

// Navigation
bool FileSystem::changeDir(const uint64_t& inode) {
    FileObj* target = cur->getChild(inode);
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

    cur = target_dir;
    return true;
}

// File operation "local"
File* FileSystem::createFile(const string& name) {
    std::vector<FileObj*> all_child = cur->getAll();
    for (int i = 0; i < (int)all_child.size(); ++i)
    {
        if (all_child[i]->getName() == name)
        {
            std::cout << "name is exist, create file error!" << std::endl;
            return 0;
        }
    }

    uint64_t inode = InodeFactory::generateInode();
    File* new_file = new File(name, "file", username, inode, cur);
    if (new_file == 0)
    {
        std::cout << "create file error!" << std::endl;
        return 0;
    }

    if (!cur->add(new_file))
    {
        delete new_file;
        return 0;
    }
    config_table[new_file->getPath()] = inode;

    return new_file;
}

bool FileSystem::deleteFile(const string& name, const string& user){
    std::vector<FileObj*> all_child = cur->getAll();
    for (int i = 0; i < (int)all_child.size(); ++i)
    {
        if (all_child[i]->getName() != name)
        {
            continue;
        }

        File* file = dynamic_cast<File*>(all_child[i]);
        if (file == 0)
        {
            std::cout << "delete file error!" << std::endl;
            return false;
        }

        if (file->getOwner() != user)
        {
            std::cout << "file owner limit, delete file error!" << std::endl;
            return false;
        }

        string path = file->getPath();
        if (!cur->remove(file->getInode()))
        {
            std::cout << "delete file error!" << std::endl;
            return false;
        }

        config_table.erase(path);
        return true;
    }

    std::cout << "file is not find, delete file error!" << std::endl;
    return false;
}

// Dir operation "local"
Directory* FileSystem::createDir(const string& name) {
    std::vector<FileObj*> all_child = cur->getAll();
    for (int i = 0; i < (int)all_child.size(); ++i)
    {
        if (all_child[i]->getName() == name)
        {
            std::cout << "name is exist, createDir error!" << std::endl;
            return 0;
        }
    }

    uint64_t inode = InodeFactory::generateInode();
    Directory* new_dir = new Directory(name, username, inode, cur);
    if (new_dir == 0)
    {
        std::cout << "createDir error!" << std::endl;
        return 0;
    }

    if (!cur->add(new_dir))
    {
        delete new_dir;
        return 0;
    }

    config_table[new_dir->getPath()] = inode;
    return new_dir;
}

bool FileSystem::deleteDir(const string& name,const string& user, bool recursive){
    FileObj* target = 0;
    std::vector<FileObj*> all_child = cur->getAll();
    for (int i = 0; i < (int)all_child.size(); ++i)
    {
        if (all_child[i]->getName() == name)
        {
            target = all_child[i];
            break;
        }
    }

    if (target == 0)
    {
        std::cout << "deleteDir error!" << std::endl;
        return false;
    }

    Directory* target_dir = dynamic_cast<Directory*>(target);
    if (!target_dir)
    {
        std::cout << "deleteDir error!" << std::endl;
        return false;
    }

    if (target_dir->getOwner() != user)
    {
        std::cout << "owner limit, deleteDir error!" << std::endl;
        return false;
    }

    if (recursive)
    {
        std::vector<FileObj*> all_target_child = target_dir->getAll();
        for (int i = 0; i < (int)all_target_child.size(); ++i)
        {
            if (Directory* sub_dir = dynamic_cast<Directory*>(all_target_child[i]))
            {
                if (!deleteDir(sub_dir->getName(), user, true))
                {
                    return false;
                }
            }
            else 
            {
                deleteFile(all_target_child[i]->getName(), user);
            }
        }
    }
    else
    {
        if (!target_dir->isEmpty())
        {
            std::cout << "deleteDir error!" << std::endl;
            return false;
        }
    }

    if (!cur->removeDir(target_dir->getInode()))
    {
        std::cout << "deleteDir error!" << std::endl;
        return false;
    }

    config_table.erase(target_dir->getPath());
    delete target_dir;
    return true;
}

// File and Dir operation "Global"
uint64_t FileSystem::search(const string& name, const string& type) {
    std::unordered_map<string, uint64_t>::iterator iter_file = config_table.find(name);
    if (iter_file != config_table.end())
    {
        return iter_file->second;
    }

    std::string relative_path = cur->getPath() + "/" + name;
    iter_file = config_table.find(relative_path);
    if (iter_file != config_table.end())
    {
        return iter_file->second;
    }

    return 0;
}

// Getters
string FileSystem::getCurrentPath() const{
    if (cur == root)
    {
        return "/";
    }

    string path;
    const Directory* tmp = cur;
    while (tmp != root)
    {
        path = "/" + tmp->getName() + path;
        tmp = (Directory *)tmp->getParent();
    }

    return path;
}

string FileSystem::getUserName() const {
    return username;
}

// Setters
bool FileSystem::setUser(const string& username) {
    std::set<string>::iterator iter_user = users.find(username);
    if (iter_user == users.end())
    {
        return false;
    }

    this->username = username;
    return true;
}

bool FileSystem::setCurrentDir(Directory* newDir){
    if (newDir == 0)
    {
        return false;
    }

    this->cur = newDir;
    return true;
}

// User management methods
bool FileSystem::hasUser(const string& username) const {
    std::set<string>::iterator iter_user = users.find(username);
    if (iter_user == users.end())
    {
        return false;
    }

    return true;
}

bool FileSystem::registerUser(const string& username) {
    if (username.empty())
    {
        return false;
    }

    if (hasUser(username))
    {
        return false;
    }

    users.insert(username);
    return true;
}

// helper function
FileObj* FileSystem::resolvePath(const string& path) {
    if (path.empty())
    {
        return 0;
    }

    Directory* start_dir = (path[0] == '/') ? root : cur;
    if (start_dir == 0)
    {
        return 0;
    }

    std::istringstream path_stream(path);
    string token;
    Directory* cur_dir = start_dir;
    FileObj* cur_file_obj = start_dir;
    while (std::getline(path_stream, token, '/'))
    {
        if (token.empty() || token == ".")
        {
            continue;
        }

        if (token == "..")
        {
            if (cur_dir->getParent())
            {
                cur_file_obj = cur_dir->getParent();
                cur_dir = dynamic_cast<Directory*>(cur_dir);
            }

            continue;
        }

        std::string path = cur->getPath() + token + '/';
        cur_file_obj = cur_dir->getChild(config_table[path]);
        if (!cur_file_obj)
        {
            return 0;
        }

        Directory* next_dir = dynamic_cast<Directory*>(cur_file_obj);
        if (next_dir)
        {
            cur_dir = next_dir;
        }
    }

    return cur_file_obj;
}