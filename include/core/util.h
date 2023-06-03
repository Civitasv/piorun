/**
 * @file util.h
 * @author jiongpaichengxuyuan (570073523@qq.com)
 * @brief
 * @version 0.1
 * @date 2023-06-03
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef PIORUN_CORE_UTIL_H
#define PIORUN_CORE_UTIL_H

#include <dirent.h>
#include <string.h>
#include <unistd.h>

#include <string>
#include <vector>

namespace pio {
namespace util {

void ListAllFile(std::vector<std::string>& files, const std::string& path,
                 const std::string& subfix) {
  if (access(path.c_str(), 0) != 0) {
    return;
  }
  DIR* dir = opendir(path.c_str());
  if (dir == nullptr) {
    return;
  }
  struct dirent* dp = nullptr;
  while ((dp = readdir(dir)) != nullptr) {
    if (dp->d_type == DT_DIR) {
      if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..")) {
        continue;
      }
      ListAllFile(files, path + "/" + dp->d_name, subfix);
    } else if (dp->d_type == DT_REG) {
      std::string filename(dp->d_name);
      if (subfix.empty()) {
        files.push_back(path + "/" + filename);
      } else {
        if (filename.size() < subfix.size()) {
          continue;
        }
        if (filename.substr(filename.length() - subfix.size()) == subfix) {
          files.push_back(path + "/" + filename);
        }
      }
    }
  }
  closedir(dir);
}

}  // namespace util
}  // namespace pio

#endif