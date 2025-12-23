# 学习指南

## 关于本项目

本项目是一个用 **C++** 编写的轻量级 Web 服务器，非常适合用来学习以下内容：

### C++ 学习方向

本项目涵盖了以下 C++ 开发的核心概念：

1. **网络编程**
   - Socket 编程
   - HTTP 协议实现
   - 多客户端连接处理

2. **多线程编程**
   - 线程池实现 (`thread_pool.h/cpp`)
   - 并发控制
   - 任务调度

3. **数据库操作**
   - MySQL Connector/C++ 使用
   - SQL 查询和数据处理 (`sql.h/cpp`)

4. **现代 C++ 特性**
   - 使用 C++23 标准
   - 智能指针
   - RAII 模式
   - 模板编程

### 快速开始学习步骤

#### 1. 环境准备
```bash
# 安装必要的依赖
# - MySQL 数据库
# - MySQL Connector/C++
# - CMake (>= 3.10)
# - Clang 编译器
```

#### 2. 构建项目
```bash
# 克隆仓库
git clone https://github.com/w2807/webServer.git
cd webServer

# 构建（Release 模式）
cmake -S . -B build
cmake --build build

# 或使用 Debug 模式（推荐学习时使用）
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

#### 3. 学习路线建议

**初级阶段：**
- 阅读 `src/main.cpp` - 理解程序入口和基本流程
- 学习 `include/thread_pool.h` - 理解线程池的设计模式
- 研究 `src/server.cpp` - 学习 Socket 编程基础

**中级阶段：**
- 分析 HTTP 请求处理流程
- 理解静态资源（图片、视频）的处理
- 学习数据库连接和查询优化

**高级阶段：**
- 优化性能（使用 `benchmark.sh` 进行基准测试）
- 添加新功能（如 HTTPS 支持、WebSocket）
- 代码重构和设计模式应用

### 项目结构说明

```
webServer/
├── include/           # 头文件
│   ├── server.h      # Web 服务器主类
│   ├── sql.h         # 数据库操作
│   └── thread_pool.h # 线程池实现
├── src/              # 源代码
│   ├── main.cpp      # 程序入口
│   ├── server.cpp    # 服务器实现
│   ├── sql.cpp       # 数据库实现
│   └── thread_pool.cpp # 线程池实现
├── assets/           # 静态资源
│   ├── images/       # 图片文件
│   ├── videos/       # 视频文件
│   └── *.html        # HTML 页面
└── CMakeLists.txt    # 构建配置
```

### 学习资源推荐

#### C++ 学习资源
- [C++ Primer](https://www.amazon.com/Primer-5th-Stanley-B-Lippman/dp/0321714113) - C++ 入门经典
- [Effective Modern C++](https://www.oreilly.com/library/view/effective-modern-c/9781491908419/) - 现代 C++ 最佳实践
- [cppreference.com](https://zh.cppreference.com/) - C++ 标准库参考
- [Learn CPP](https://www.learncpp.com/) - 免费在线教程

#### 网络编程资源
- [Unix Network Programming](https://www.amazon.com/Unix-Network-Programming-Volume-Sockets/dp/0131411551) - 网络编程圣经
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/) - 免费网络编程指南

### 实践建议

1. **从简单开始**：先运行项目，通过浏览器访问，理解其基本功能
2. **逐步深入**：逐个模块学习，建议顺序：main.cpp → thread_pool → server → sql
3. **动手修改**：尝试添加新功能或修改现有功能
4. **调试学习**：使用 GDB 或 LLDB 调试器跟踪代码执行
5. **性能测试**：使用 `benchmark.sh` 测试性能，学习优化方法

---

## 如果您想学习 Go 语言

如果您的目标是学习 **Go 语言**而非 C++，这里有一些推荐的 Go 语言学习项目：

### Go Web 开发入门项目推荐

1. **[gin-gonic/gin](https://github.com/gin-gonic/gin)**
   - Go 最流行的 Web 框架之一
   - 文档完善，示例丰富
   - 适合快速构建 Web 应用

2. **[go-web-example](https://github.com/gowebexamples/gowebexamples)**
   - Go Web 开发示例集合
   - 包含路由、模板、数据库等常见场景
   - 适合初学者

3. **[go-gin-example](https://github.com/eddycjy/go-gin-example)**
   - 基于 Gin 框架的完整项目示例
   - 包含 JWT 认证、日志、配置等
   - 中文文档友好

4. **[build-web-application-with-golang](https://github.com/astaxie/build-web-application-with-golang)**
   - 一本开源的 Go Web 编程书籍
   - 从基础到进阶，内容全面
   - 中文原创教程

### Go 学习资源

- [Go 官方教程](https://go.dev/tour/) - 交互式学习
- [Go by Example](https://gobyexample.com/) - 通过例子学习
- [The Go Programming Language](https://www.gopl.io/) - Go 语言圣经
- [Effective Go](https://go.dev/doc/effective_go) - Go 最佳实践官方指南

### 从本项目到 Go 的迁移

如果您想用 Go 重写本 Web 服务器项目作为学习练习，可以：

1. **使用 Go 标准库 `net/http`** 实现基本的 HTTP 服务器
2. **使用 Goroutine 和 Channel** 替代 C++ 的线程池
3. **使用 `database/sql` 或 ORM（如 GORM）** 进行数据库操作
4. **使用 Gin 或 Echo 框架** 简化路由和中间件处理

这将是一个很好的对比学习机会，帮助您理解两种语言在 Web 开发中的不同实现方式。

---

## 贡献

如果您在学习过程中发现问题或有改进建议，欢迎提交 Issue 或 Pull Request！

## 许可证

查看 [LICENSE](LICENSE) 文件了解详情。
