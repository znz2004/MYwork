#pragma once

#include <cstdint>

class InodeFactory{
public:
    static uint64_t generateInode(){
        static uint64_t nextInode = 1;
        return nextInode++;
    }
};