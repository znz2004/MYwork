#pragma once

#include "FileObj.h"

class File: public FileObj{
protected:
    string content;

public:
    File(const string& name, const string& type, const string& owner, const uint64_t& inode, FileObj* parent);
    ~File() override = default;
    
    // read and write
    virtual string read() const;
    virtual bool write(const string &data);

    // Getters
    virtual string getContent() const;

    // helper function
    void display() const override;
};
