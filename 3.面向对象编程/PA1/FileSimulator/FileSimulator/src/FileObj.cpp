#include "FileObj.h"

FileObj::FileObj(const string& name, const string& path, const string& type, 
                 const string& owner, const uint64_t& inode, FileObj* parent)
    : name(name), path(path), type(type), owner(owner), inode(inode), parent(parent) {
}