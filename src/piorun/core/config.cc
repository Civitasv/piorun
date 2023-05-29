/*
 * @Date: 2023-05-23 03:32:07
 * @LastEditors: jiongpaichengxuyuan 570073523@qq.com
 * @LastEditTime: 2023-05-29 08:31:26
 * @FilePath: /piorun/src/piorun/core/config.cc
 */
#include "core/config.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace piorun {

ConfigVarBase::ptr Config::LookupBase(const std::string& name) {
  // RWMutexType::ReadLock lock(GetMutex());
  auto it = GetDatas().find(name);
  return it == GetDatas().end() ? nullptr : it->second;
}

static void ListAllMember(
    const std::string& prefix, const YAML::Node& node,
    std::list<std::pair<std::string, const YAML::Node> >& output) {
  if (prefix.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._012345678") !=
      std::string::npos) {
    return;
  }
  output.push_back(std::make_pair(prefix, node));
  if (node.IsMap()) {
    for (auto it = node.begin(); it != node.end(); ++it) {
      ListAllMember(prefix.empty() ? it->first.Scalar()
                                   : prefix + "." + it->first.Scalar(),
                    it->second, output);
    }
  }
}

void Config::LoadFromYaml(const YAML::Node& root) {
  std::list<std::pair<std::string, const YAML::Node> > all_nodes;
  ListAllMember("", root, all_nodes);

  for (auto& i : all_nodes) {
    std::string key = i.first;
    if (key.empty()) {
      continue;
    }

    std::transform(key.begin(), key.end(), key.begin(), ::tolower);
    ConfigVarBase::ptr var = LookupBase(key);

    if (var) {
      if (i.second.IsScalar()) {
        var->FromString(i.second.Scalar());
      } else {
        std::stringstream ss;
        ss << i.second;
        var->FromString(ss.str());
      }
    }
  }
}

static std::map<std::string, uint64_t> s_file2modifytime;
// static piorun::Mutex s_mutex;

void Config::LoadFromConfDir(const std::string& path, bool force) {}

void Config::Visit(std::function<void(ConfigVarBase::ptr)> cb) {
  //   RWMutexType::ReadLock lock(GetMutex());
  ConfigVarMap& m = GetDatas();
  for (auto it = m.begin(); it != m.end(); ++it) {
    cb(it->second);
  }
}

}  // namespace piorun
