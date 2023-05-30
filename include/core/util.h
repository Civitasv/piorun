#ifndef PIORUN_CORE_UTIL_H
#define PIORUN_CORE_UTIL_H

#include <vector>
#include <string>
#include <dirent.h>
#include <string.h>
#include <unistd.h>

namespace pio{
namespace util {

void ListAllFile(std::vector<std::string>& files
                            ,const std::string& path
                            ,const std::string& subfix) {
    if(access(path.c_str(), 0) != 0) {
        return;
    }
    DIR* dir = opendir(path.c_str());
    if(dir == nullptr) {
        return;
    }
    struct dirent* dp = nullptr;
    while((dp = readdir(dir)) != nullptr) {
        if(dp->d_type == DT_DIR) {
            if(!strcmp(dp->d_name, ".")
                || !strcmp(dp->d_name, "..")) {
                continue;
            }
            ListAllFile(files, path + "/" + dp->d_name, subfix);
        } else if(dp->d_type == DT_REG) {
            std::string filename(dp->d_name);
            if(subfix.empty()) {
                files.push_back(path + "/" + filename);
            } else {
                if(filename.size() < subfix.size()) {
                    continue;
                }
                if(filename.substr(filename.length() - subfix.size()) == subfix) {
                    files.push_back(path + "/" + filename);
                }
            }
        }
    }
    closedir(dir);
}

}
}

#endif