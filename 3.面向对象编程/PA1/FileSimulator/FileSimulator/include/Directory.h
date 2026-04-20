#pragma once

#include <unordered_map>
#include "FileObj.h"

class Directory: public FileObj{
private:
    std::unordered_map<uint64_t, FileObj*> children;

public:
    Directory(const string& name, const string& owner, const uint64_t& inode, FileObj* parent);
    ~Directory() {
        for (auto& pair : children) {
            delete pair.second;
        }
        children.clear();
    }

    // add, remove
    bool add(FileObj* child);
    bool remove(uint64_t inode);
    bool removeDir(uint64_t inode);
    
    // Getters
    FileObj* getChild(uint64_t inode);
    std::vector<FileObj*> getAll() const;
    size_t getCount() const;

    // helper function
    bool isEmpty() const;
    void display() const override;
};