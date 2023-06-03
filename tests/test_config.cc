/**
 * @file test_config.cc
 * @author jiongpaichengxuyuan (570073523@qq.com)
 * @brief
 * @version 0.1
 * @date 2023-06-03
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <iostream>

#include "core/config.h"

pio::config::ConfigVar<int>::ptr g_int_value_config =
    pio::config::Config::Lookup("system.port", (int)8080, "system port");

pio::config::ConfigVar<float>::ptr g_float_value_config =
    pio::config::Config::Lookup("system.value", (float)10.2f, "system value");

void print_yaml(const YAML::Node& node, int level) {
  if (node.IsScalar()) {
    std::cout << std::string(level * 4, ' ') << node.Scalar() << " - "
              << node.Type() << " - " << level;
  } else if (node.IsNull()) {
    std::cout << std::string(level * 4, ' ') << "NULL - " << node.Type()
              << " - " << level;
  } else if (node.IsMap()) {
    for (auto it = node.begin(); it != node.end(); ++it) {
      std::cout << std::string(level * 4, ' ') << it->first << " - "
                << it->second.Type() << " - " << level;
      print_yaml(it->second, level + 1);
    }
  } else if (node.IsSequence()) {
    for (size_t i = 0; i < node.size(); ++i) {
      std::cout << std::string(level * 4, ' ') << i << " - " << node[i].Type()
                << " - " << level;
      print_yaml(node[i], level + 1);
    }
  }
}

void TestYaml() {
  YAML::Node root = YAML::LoadFile("/home/liuzh/src/piorun/bin/conf/log.yml");
  print_yaml(root, 0);
}

class Person {
 public:
  Person(){};
  std::string m_name;
  int m_age = 0;
  bool m_sex = 0;

  std::string ToString() const {
    std::stringstream ss;
    ss << "[Person name=" << m_name << " age=" << m_age << " sex=" << m_sex
       << "]";
    return ss.str();
  }

  bool operator==(const Person& oth) const {
    return m_name == oth.m_name && m_age == oth.m_age && m_sex == oth.m_sex;
  }
};

namespace pio {

namespace config {
template <>
class LexicalCast<std::string, Person> {
 public:
  Person operator()(const std::string& v) {
    YAML::Node node = YAML::Load(v);
    Person p;
    p.m_name = node["name"].as<std::string>();
    p.m_age = node["age"].as<int>();
    p.m_sex = node["sex"].as<bool>();
    return p;
  }
};

template <>
class LexicalCast<Person, std::string> {
 public:
  std::string operator()(const Person& p) {
    YAML::Node node;
    node["name"] = p.m_name;
    node["age"] = p.m_age;
    node["sex"] = p.m_sex;
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

}  // namespace config

}  // namespace pio

static pio::config::ConfigVar<Person>::ptr g_person =
    pio::config::Config::Lookup("class.person", Person(), "system person");

static pio::config::ConfigVar<std::map<std::string, Person> >::ptr
    g_person_map = pio::config::Config::Lookup(
        "class.map", std::map<std::string, Person>(), "system person");

static pio::config::ConfigVar<std::map<std::string, std::vector<Person> > >::ptr
    g_person_vec_map = pio::config::Config::Lookup(
        "class.vec_map", std::map<std::string, std::vector<Person> >(),
        "system person");

static auto s_person = g_person->get_value();

void TestClass() {
  std::cout << "before: " << g_person->get_value().ToString() << " - "
            << g_person->ToString();

#define XX_PM(g_var, prefix)                                                  \
  {                                                                           \
    auto m = g_person_map->get_value();                                       \
    for (auto& i : m) {                                                       \
      std::cout << prefix << ": " << i.first << " - " << i.second.ToString(); \
    }                                                                         \
    std::cout << prefix << ": size=" << m.size();                             \
  }

  g_person->AddListener([](const Person& old_value, const Person& new_value) {
    std::cout << "old_value=" << old_value.ToString()
              << " new_value=" << new_value.ToString();
  });

  XX_PM(g_person_map, "class.map before");
  std::cout << "before: " << g_person_vec_map->ToString();

  Person p2 = Person();
  p2.m_name = "你好";

  g_person->set_value(p2);
  std::map<std::string, Person> p_map;
  p_map["test"] = p2;

  g_person_map->set_value(p_map);

  std::cout << "after: " << g_person->get_value().ToString() << " - "
            << g_person->ToString();
  XX_PM(g_person_map, "class.map after");
  std::cout << "after: " << g_person_vec_map->ToString();
}

void TestCallBack() {
  g_person->AddListener(
      [](const Person& ov, const Person& nv) { s_person = nv; });

  std::cout << "befor: " << s_person.ToString();
  Person p3 = Person();
  p3.m_name = "hello";
  g_person->set_value(p3);
  std::cout << "after: " << s_person.ToString();
}

int main() {
  std::cout << "I'm a test" << '\n';

  // std::cout << g_int_value_config->get_value();
  // std::cout << g_float_value_config->get_value();
  //   std::vector<int> v{1, 2, 3, 4};
  //   auto t = piorun::LexicalCast<std::vector<int>, std::string>();
  //   std::string s = t(v);
  //   auto t2 = piorun::LexicalCast<std::string, std::vector<int>>();
  //   auto v2 = t2(s);
  //   for (auto i : v2) std::cout << i;
  //   std::unordered_set<int> u_s;
  //   u_s.insert(1);
  //   u_s.insert(2);
  //   u_s.insert(3);
  //   auto t3 = piorun::LexicalCast<std::unordered_set<int>, std::string>();
  //   s = t3(u_s);
  //   std::cout << s;
  TestYaml();
  // TestClass();
  // TestCallBack();
}