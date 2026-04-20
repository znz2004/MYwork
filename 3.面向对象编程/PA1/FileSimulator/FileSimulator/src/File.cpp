#include "File.h"
#include <fstream>
#include <sstream>

File::File(const string& name, const string& type, const string& owner, const uint64_t& inode, FileObj* parent)
    :FileObj(name, (parent ? parent->getPath() + name + "/" : "/"), type, owner, inode, parent), content("") {
}

// read and write
string File::read() const {
    std::ifstream  file(getPath());
    if (!file.is_open())
    {
        fprintf(stderr, "Error: File::read() Open File Error!\n");
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
}

bool File::write(const string &data) { 
    string tmp = data;
    size_t pos = 0;
    pos = tmp.find("\n", pos);
    while (pos != string::npos) {
        tmp.replace(pos, 1, "\\n");
        pos += 3;
        pos = tmp.find("\n", pos);
    }

    content += tmp;
    return true;
}

string File::getContent() const {
    return content;
}

// helper function
void File::display() const {
    std::cout << "[File: " << getName() << "] Size: " << content.length() << " bytes" << std::endl;
}
