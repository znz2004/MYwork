#include "Directory.h"

Directory::Directory(const string& name, const string& owner, const uint64_t& inode, FileObj* parent)
    :FileObj(name, parent ? parent->getPath() + name + "/" : "/", "directory", owner, inode, parent) {
    children.clear();
}

// add, remove
bool Directory::add(FileObj* child) {
    if (child == NULL){
        return false;
    }

    if (children.find(child->getInode()) != children.end()) {
        return false;
    }

    child->setParent(this);
    children[child->getInode()] = child;
    return true;
}

bool Directory::remove(uint64_t inode) {
    std::unordered_map<uint64_t, FileObj*>::iterator iter_child = children.find(inode);
	if (iter_child == children.end()) {
		return false;
	}

    if (iter_child->second == NULL)
    {
        return false;
    }

    if (iter_child->second->getType() == "directory")
    {
        return false;
    }

    delete(iter_child->second);
    children.erase(iter_child);
    return true;
}

bool Directory::removeDir(uint64_t inode){
    FileObj* child = getChild(inode);
    if (child == 0)
    {
        return false;
    }

	if (child->getType() != "directory")
	{
		return false;
	}

    Directory* child_dir = dynamic_cast<Directory*>(child);
    if (child_dir == 0)
    {
        return false;
    }
        
    std::vector<FileObj*> child_dir_file = getAll();
    for (int i = 0; i < (int)child_dir_file.size(); ++i)
    {
        if (child_dir_file[i] == 0)
        {
            continue;
        }

        if (child_dir_file[i]->getPath() == "directory")
        {
            removeDir(child_dir_file[i]->getInode());
        }
        else
        {
            remove(child_dir_file[i]->getInode());
        }
    }

	delete(child);
	children.erase(inode);
    return true;
}

// Getters
FileObj* Directory::getChild(uint64_t inode) {
	std::unordered_map<uint64_t, FileObj*>::iterator iter_child = children.find(inode);
	if (iter_child == children.end()) {
		return nullptr;
	}

    return iter_child->second;
}

std::vector<FileObj*> Directory::getAll() const {
    std::vector<FileObj*> child_list;
    child_list.clear();
    for (std::unordered_map<uint64_t, FileObj*>::const_iterator iter = children.begin(); iter != children.end(); ++iter)
    {
        child_list.push_back(iter->second);
    }

    return child_list;
}

size_t Directory::getCount() const {
    return children.size();
}

// helper function
bool Directory::isEmpty() const {
    return children.empty();
}

// helper function
void Directory::display() const {
    std::cout << "[Directory: " << getName() << "]" << std::endl;
    for (const auto& pair : this->children) {
        std::cout << "- " << pair.second->getType() << ": " 
                 << pair.second->getName() << " with owner: " << pair.second->getOwner() << std::endl;
    }
}