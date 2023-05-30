/*
 * @Author: jiongpaichengxuyuan 570073523@qq.com
 * @Date: 2023-05-23 03:31:56
 * @LastEditors: jiongpaichengxuyuan 570073523@qq.com
 * @LastEditTime: 2023-05-29 08:32:39
 * @FilePath: /piorun/include/core/config.h
 */
#ifndef PIORUN_CORE_CONFIG_H
#define PIORUN_CORE_CONFIG_H

#include <cxxabi.h>
#include <yaml-cpp/yaml.h>

#include <functional>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace pio {

namespace config{
/**
 * @brief 将源格式数据转换为目标格式数据
 * @param SourceType 源格式
 * @param TargetType 目标格式
 */
template <typename TargetType, typename SourceType>
TargetType TypeCast(const SourceType& value) {
  std::stringstream ss;
  TargetType result;
  if (!(ss << value) || !(ss >> result) || !(ss >> std::ws).eof()) {
    throw std::bad_cast();  // 抛出异常表示转换失败
  }
  return result;
}

template <typename SourceType, typename TargetType>
class LexicalCast {
 public:
  /**
   * @brief 类型转换
   * @param value 源类型值
   * @return 返回value转换后的目标类型
   */
  TargetType operator()(const SourceType& value) {
    return TypeCast<TargetType>(value);
  }
};

/**
 * @brief 类型转换模板类片特化(YAML String 转换成 std::vector<T>)
 */
template <typename T>
class LexicalCast<std::string, std::vector<T> > {
 public:
  std::vector<T> operator()(const std::string& value) {
    YAML::Node node = YAML::Load(value);
    typename std::vector<T> vec;
    std::stringstream ss;
    for (size_t i = 0; i < node.size(); ++i) {
      ss.str("");
      ss << node[i];
      vec.push_back(LexicalCast<std::string, T>()(ss.str()));
    }
    return vec;
  }
};

/**
 * @brief 类型转换模板类片特化(std::vector<T> 转换成 YAML String)
 */
template <typename T>
class LexicalCast<std::vector<T>, std::string> {
 public:
  std::string operator()(const std::vector<T>& value) {
    YAML::Node node(YAML::NodeType::Sequence);
    for (auto& i : value) {
      node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

/**
 * @brief 类型转换模板类片特化(YAML String 转换成 std::list<T>)
 */
template <typename T>
class LexicalCast<std::string, std::list<T> > {
 public:
  std::list<T> operator()(const std::string& value) {
    YAML::Node node = YAML::Load(value);
    typename std::list<T> list_;
    std::stringstream ss;
    for (size_t i = 0; i < node.size(); ++i) {
      ss.str("");
      ss << node[i];
      list_.push_back(LexicalCast<std::string, T>()(ss.str()));
    }
    return list_;
  }
};

/**
 * @brief 类型转换模板类片特化(std::list<T> 转换成 YAML String)
 */
template <typename T>
class LexicalCast<std::list<T>, std::string> {
 public:
  std::string operator()(const std::list<T>& value) {
    YAML::Node node(YAML::NodeType::Sequence);
    for (auto& i : value) {
      node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

/**
 * @brief 类型转换模板类片特化(YAML String 转换成 std::set<T>)
 */
template <typename T>
class LexicalCast<std::string, std::set<T> > {
 public:
  std::set<T> operator()(const std::string& value) {
    YAML::Node node = YAML::Load(value);
    typename std::set<T> set_;
    std::stringstream ss;
    for (size_t i = 0; i < node.size(); ++i) {
      ss.str("");
      ss << node[i];
      set_.insert(LexicalCast<std::string, T>()(ss.str()));
    }
    return set_;
  }
};

/**
 * @brief 类型转换模板类片特化(std::set<T> 转换成 YAML String)
 */
template <typename T>
class LexicalCast<std::set<T>, std::string> {
 public:
  std::string operator()(const std::set<T>& value) {
    YAML::Node node(YAML::NodeType::Sequence);
    for (auto& i : value) {
      node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

/**
 * @brief 类型转换模板类片特化(YAML String 转换成 std::unordered_set<T>)
 */
template <typename T>
class LexicalCast<std::string, std::unordered_set<T> > {
 public:
  std::unordered_set<T> operator()(const std::string& value) {
    YAML::Node node = YAML::Load(value);
    typename std::unordered_set<T> hash_set;
    std::stringstream ss;
    for (size_t i = 0; i < node.size(); ++i) {
      ss.str("");
      ss << node[i];
      hash_set.insert(LexicalCast<std::string, T>()(ss.str()));
    }
    return hash_set;
  }
};

/**
 * @brief 类型转换模板类片特化(std::unordered_set<T> 转换成 YAML String)
 */
template <typename T>
class LexicalCast<std::unordered_set<T>, std::string> {
 public:
  std::string operator()(const std::unordered_set<T>& value) {
    YAML::Node node(YAML::NodeType::Sequence);
    for (auto& i : value) {
      node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

/**
 * @brief 类型转换模板类片特化(YAML String 转换成 std::map<std::string, T>)
 */
template <typename T>
class LexicalCast<std::string, std::map<std::string, T> > {
 public:
  std::map<std::string, T> operator()(const std::string& value) {
    YAML::Node node = YAML::Load(value);
    typename std::map<std::string, T> map_;
    std::stringstream ss;
    for (auto it = node.begin(); it != node.end(); ++it) {
      ss.str("");
      ss << it->second;
      map_.insert(std::make_pair(it->first.Scalar(),
                                 LexicalCast<std::string, T>()(ss.str())));
    }
    return map_;
  }
};

/**
 * @brief 类型转换模板类片特化(std::map<std::string, T> 转换成 YAML String)
 */
template <typename T>
class LexicalCast<std::map<std::string, T>, std::string> {
 public:
  std::string operator()(const std::map<std::string, T>& value) {
    YAML::Node node(YAML::NodeType::Map);
    for (auto& i : value) {
      node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

/**
 * @brief 类型转换模板类片特化(YAML String 转换成
 * std::unordered_map<std::string, T>)
 */
template <typename T>
class LexicalCast<std::string, std::unordered_map<std::string, T> > {
 public:
  std::unordered_map<std::string, T> operator()(const std::string& value) {
    YAML::Node node = YAML::Load(value);
    typename std::unordered_map<std::string, T> hash_map;
    std::stringstream ss;
    for (auto it = node.begin(); it != node.end(); ++it) {
      ss.str("");
      ss << it->second;
      hash_map.insert(std::make_pair(it->first.Scalar(),
                                     LexicalCast<std::string, T>()(ss.str())));
    }
    return hash_map;
  }
};

/**
 * @brief 类型转换模板类片特化(std::unordered_map<std::string, T> 转换成 YAML
 * String)
 */
template <typename T>
class LexicalCast<std::unordered_map<std::string, T>, std::string> {
 public:
  std::string operator()(const std::unordered_map<std::string, T>& value) {
    YAML::Node node(YAML::NodeType::Map);
    for (auto& i : value) {
      node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

template <typename T>
const char* TypeToName() {
  static const char* s_name =
      abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
  return s_name;
}

class ConfigVarBase {
 public:
  typedef std::shared_ptr<ConfigVarBase> ptr;
  ConfigVarBase(const std::string& name, const std::string& description = "")
      : name_(name), description_(description) {
    std::transform(name_.begin(), name_.end(), name_.begin(), ::tolower);
  }

  /**
   * @brief 析构函数
   */
  virtual ~ConfigVarBase() {}

  /**
   * @brief 返回配置参数名称
   */
  const std::string& get_name() const { return name_; }

  /**
   * @brief 返回配置参数的描述
   */
  const std::string& get_description() const { return description_; }

  /**
   * @brief 转成字符串
   */
  virtual std::string ToString() = 0;

  /**
   * @brief 从字符串初始化值
   */
  virtual bool FromString(const std::string& value) = 0;

  /**
   * @brief 返回配置参数值的类型名称
   */
  virtual std::string GetTypeName() const = 0;

 protected:
  /// @brief 配置参数的名称
  std::string name_;
  /// @brief 配置参数的描述
  std::string description_;
};

/**
 * @brief 配置参数模板子类,保存对应类型的参数值
 * @details T 参数的具体类型
 *          FromStr 从std::string转换成T类型的仿函数
 *          ToStr 从T转换成std::string的仿函数
 *          std::string 为YAML格式的字符串
 */
template <typename T, typename FromStr = LexicalCast<std::string, T>,
          typename ToStr = LexicalCast<T, std::string> >
class ConfigVar : public ConfigVarBase {
 public:
  // typedef RWMutex RWMutexType;
  typedef std::shared_ptr<ConfigVar> ptr;
  typedef std::function<void(const T& old_value, const T& new_value)>
      on_change_cb;

  /**
   * @brief 通过参数名,参数值,描述构造ConfigVar
   * @param name 参数名称有效字符为[0-9a-z_.]
   * @param default_value 参数的默认值
   * @param description 参数的描述
   */
  ConfigVar(const std::string& name, const T& default_value,
            const std::string& description = "")
      : ConfigVarBase(name, description), value_(default_value) {}

  /**
   * @brief 将参数值转换成YAML String
   * @exception 当转换失败抛出异常
   */
  std::string ToString() override {
    try {
      // RWMutexType::ReadLock lock(_mutex);
      return ToStr()(value_);
    } catch (std::exception& e) {
    }
    return "";
  }

  /**
   * @brief 从YAML String 转成参数的值
   * @exception 当转换失败抛出异常
   */
  bool FromString(const std::string& val) override {
    try {
      set_value(FromStr()(val));
    } catch (std::exception& e) {
    }
    return false;
  }

  /**
   * @brief 获取当前参数的值
   */
  const T get_value() {
    // RWMutexType::ReadLock lock(_mutex);
    return value_;
  }

  /**
   * @brief 设置当前参数的值
   * @details 如果参数的值有发生变化,则通知对应的注册回调函数
   */
  void set_value(const T& v) {
    {
      // RWMutexType::ReadLock lock(_mutex);
      if (v == value_) {
        return;
      }
      for (auto& i : _cbs) {
        i.second(value_, v);
      }
    }
    // RWMutexType::WriteLock lock(_mutex);
    value_ = v;
  }

  /**
   * @brief 返回参数值的类型名称(typeinfo)
   */
  std::string GetTypeName() const override { return TypeToName<T>(); }

  /**
   * @brief 添加变化回调函数
   * @return 返回该回调函数对应的唯一id,用于删除回调
   */
  uint64_t AddListener(on_change_cb cb) {
    static uint64_t s_fun_id = 0;
    // RWMutexType::WriteLock lock(_mutex);
    ++s_fun_id;
    _cbs[s_fun_id] = cb;
    return s_fun_id;
  }

  /**
   * @brief 删除回调函数
   * @param key 回调函数的唯一id
   */
  void DelListener(uint64_t key) {
    // RWMutexType::WriteLock lock(_mutex);
    _cbs.erase(key);
  }

  /**
   * @brief 获取回调函数
   * @param key 回调函数的唯一id
   * @return 如果存在返回对应的回调函数,否则返回nullptr
   */
  on_change_cb GetListener(uint64_t key) {
    // RWMutexType::ReadLock lock(_mutex);
    auto it = _cbs.find(key);
    return it == _cbs.end() ? nullptr : it->second;
  }

  /**
   * @brief 清理所有的回调函数
   */
  void ClearListener() {
    // RWMutexType::WriteLock lock(_mutex);
    _cbs.clear();
  }

 private:
  // RWMutexType _mutex;
  T value_;
  // 变更回调函数组, uint64_t key,要求唯一，一般可以用hash
  std::map<uint64_t, on_change_cb> _cbs;
};

/**
 * @brief ConfigVar的管理类
 * @details 提供便捷的方法创建/访问ConfigVar
 */
class Config {
 public:
  typedef std::unordered_map<std::string, ConfigVarBase::ptr> ConfigVarMap;
  // typedef RWMutex RWMutexType;

  /**
   * @brief 获取/创建对应参数名的配置参数
   * @param name 配置参数名称
   * @param default_value 参数默认值
   * @param description 参数描述
   * @details 获取参数名为name的配置参数,如果存在直接返回
   *          如果不存在,创建参数配置并用default_value赋值
   * @return 返回对应的配置参数,如果参数名存在但是类型不匹配则返回nullptr
   * @exception 如果参数名包含非法字符[^0-9a-z_.] 抛出异常 std::invalid_argument
   */
  template <typename T>
  static typename ConfigVar<T>::ptr Lookup(
      const std::string& name, const T& default_value,
      const std::string& description = "") {
    // RWMutexType::WriteLock lock(GetMutex());
    auto it = GetDatas().find(name);
    if (it != GetDatas().end()) {
      auto tmp = std::dynamic_pointer_cast<ConfigVar<T> >(it->second);
      if (tmp) {
        return tmp;
      } else {
        return nullptr;
      }
    }

    if (name.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._012345678") !=
        std::string::npos) {
      throw std::invalid_argument(name);
    }

    typename ConfigVar<T>::ptr v(
        new ConfigVar<T>(name, default_value, description));
    GetDatas()[name] = v;
    return v;
  }

  /**
   * @brief 查找配置参数
   * @param name 配置参数名称
   * @return 返回配置参数名为name的配置参数
   */
  template <typename T>
  static typename ConfigVar<T>::ptr Lookup(const std::string& name) {
    // RWMutexType::ReadLock lock(GetMutex());
    auto it = GetDatas().find(name);
    if (it == GetDatas().end()) {
      return nullptr;
    }
    return std::dynamic_pointer_cast<ConfigVar<T> >(it->second);
  }

  /**
   * @brief 使用YAML::Node初始化配置模块
   */
  static void LoadFromYaml(const YAML::Node& root);

  /**
   * @brief 加载path文件夹里面的配置文件
   */
  static void LoadFromConfDir(const std::string& path, bool force = false);

  /**
   * @brief 查找配置参数,返回配置参数的基类
   * @param name 配置参数名称
   */
  static ConfigVarBase::ptr LookupBase(const std::string& name);

  /**
   * @brief 遍历配置模块里面所有配置项
   * @param cb 配置项回调函数
   */
  static void Visit(std::function<void(ConfigVarBase::ptr)> cb);

 private:
  /**
   * @brief 返回所有的配置项
   */
  static ConfigVarMap& GetDatas() {
    static ConfigVarMap s_datas;
    return s_datas;
  }

  /**
   * @brief 配置项的RWMutex
   */
  // static RWMutexType& GetMutex() {
  //   static RWMutexType s_mutex;
  //   return s_mutex;
  // }
};

}

}  // namespace piorun

#endif