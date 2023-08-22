#ifndef HELPERFUNCS_H
#define HELPERFUNCS_H
#include<fstream>
#include<vector>
class HelperFuncs{
public:
    static void ReadFile(const char* filename,std::vector<char>& bytes);
};
#endif