# super-super-super tiny web server

一个用 C++ 实现的轻量级 Web 服务器，支持图片和小视频。

## 📚 学习资源

**新手入门？** 查看 [学习指南 (LEARNING_GUIDE.md)](LEARNING_GUIDE.md) 了解：
- 如何使用本项目学习 C++ Web 开发
- C++ 网络编程和多线程编程
- 如果您想学习 Go 语言的推荐项目和资源

## 功能特性

- ✅ HTTP 服务器基础实现
- ✅ 静态资源服务（图片、视频）
- ✅ MySQL 数据库集成
- ✅ 线程池支持
- ✅ C++23 标准

## 前置要求

需要先安装以下依赖：
- MySQL 数据库
- [MySQL Connector/C++](https://dev.mysql.com/downloads/connector/cpp/)
- CMake (>= 3.10)
- Clang 编译器

## 快速开始

### 构建
```bash
cmake -S . -B build
cmake --build build
```

### Debug 构建（推荐学习时使用）
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

### 运行
```bash
./build/simple_web_server
```

## 性能测试

```bash
./benchmark.sh
```

## 项目结构

- `include/` - 头文件
- `src/` - 源代码
- `assets/` - 静态资源（HTML、图片、视频）

详细说明请参考 [学习指南](LEARNING_GUIDE.md)。

## 许可证

详见 [LICENSE](LICENSE) 文件。
