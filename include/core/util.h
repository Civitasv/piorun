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

void ListAllFile(std::vector<std::string>& files, const std::string& path,
                 const std::string& subfix);

}  // namespace pio

#endif