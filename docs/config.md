# 配置模块

## 模块简介
* 利用yaml格式进行配置，通过yaml-cpp进行序列与反序列
* 支持解析vector、set、map、unordered_set、unordered_map等STL容器
* 支持自定义类型的支持，需要实现序列化和反序列化方法，例如：
```c++
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
```
* 可通过添加回调函数实现在更新配置之后同步更新相关变量，例如：
```c++
static pio::config::ConfigVar<Person>::ptr g_person =
    pio::config::Config::Lookup("class.person", Person(), "system person");
void TestCallBack() {
  g_person->AddListener(
      [](const Person& ov, const Person& nv) { s_person = nv; });

  std::cout << "befor: " << s_person.ToString();
  Person p3 = Person();
  p3.m_name = "hello";
  g_person->set_value(p3);
  std::cout << "after: " << s_person.ToString();
}
```
结果：
```
befor: [Person name= age=0 sex=0]after: [Person name=hello age=0 sex=0]
```
* 可以通过加载.log文件直接更新配置
```c++
  /**
   * @brief 加载path文件夹里面的配置文件
   */
  static void LoadFromConfDir(const std::string& path, bool force = false);
```