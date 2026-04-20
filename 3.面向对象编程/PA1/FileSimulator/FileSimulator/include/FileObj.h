#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>
#include <assert.h>
#include <cstdint>
#include <string>
#include <vector>
#include <set>
#include <iostream>
#include <cassert>
#include <stdexcept>
#include "InodeFactory.h"
using string = std::string;

class FileObj{
private:
    string name;
    string path;
    string type;
    string owner;
    uint64_t inode;
    FileObj * parent;

public:
    FileObj(const string& name, const string& path, const string& type, const string& owner, const uint64_t& inode, FileObj* parent);
    virtual ~FileObj() = default;

    //Getters
    string getName() const { return name; }
    string getPath() const { return path; }
    string getType() const { return type; }
    string getOwner() const { return owner; }
    uint64_t getInode() const { return inode; }
    FileObj * getParent() const { return parent; }
    
    //Setters
    void setName(const string& name) { this->name = name; }
    void setPath(const string& path) { this->path = path; }
    void setOwner(const string& owner) { this->owner = owner; }
    void setInode(const uint64_t& inode) { this->inode = inode; }
    void setParent(FileObj * parent) { this->parent = parent; }

    virtual void display() const = 0;
};