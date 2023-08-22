#include "helperfuncs.h"
#include<string>
void HelperFuncs::ReadFile(const char *filename, std::vector<char>& bytes)
{
    std::ifstream ifs;
    ifs.open(filename,std::ios_base::ate|std::ios_base::binary);
    if(!ifs.is_open()){
        std::string errinfo = "failed to load file ";
        errinfo += std::string(filename) + '!';
        throw std::runtime_error(errinfo);
    }
    uint32_t size = ifs.tellg();
    ifs.seekg(0);
    bytes.resize(size);
    ifs.read(bytes.data(),size);
    ifs.close();
}