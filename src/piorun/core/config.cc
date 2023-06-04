/**
 * @file config.cc
 * @author jiongpaichengxuyuan (570073523@qq.com)
 * @brief
 * @version 0.1
 * @date 2023-06-03
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "core/config.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "core/util.h"

namespace pio {

ConfigVarBase::ptr Config::LookupBase(const std::string& name) {
  std::shared_lock lock(get_mutex());
  auto it = get_datas().find(name);
  return it == get_datas().end() ? nullptr : it->second;
}

static void ListAllMember(
    const std::string& prefix, const YAML::Node& node,
    std::list<std::pair<std::string, const YAML::Node> >& output) {
  if (prefix.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._012345678") !=
      std::string::npos) {
    logger->Error("Config invalid name: " + prefix);
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
static std::mutex s_mutex;

void Config::LoadFromConfDir(const std::string& path, bool force) {
  std::vector<std::string> files;
  pio::ListAllFile(files, path, ".yml");

  for (auto& i : files) {
    {
      struct stat st;
      lstat(i.c_str(), &st);
      std::lock_guard lock(s_mutex);
      if (!force && s_file2modifytime[i] == (uint64_t)st.st_mtime) {
        continue;
      }
      s_file2modifytime[i] = st.st_mtime;
    }
    try {
      YAML::Node root = YAML::LoadFile(i);
      LoadFromYaml(root);
      logger->Info("LoadConfFile file=" + std::string(i) + "ok");
    } catch (...) {
      logger->Error("LoadConfFile file=" + std::string(i) + "failed");
    }
  }
}

void Config::Visit(std::function<void(ConfigVarBase::ptr)> cb) {
  std::shared_lock lock(get_mutex());
  ConfigVarMap& m = get_datas();
  for (auto it = m.begin(); it != m.end(); ++it) {
    cb(it->second);
  }
}

}  // namespace pio
