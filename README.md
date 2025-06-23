# super-super-super tiny web server

支持图片和小视频

需要先安装mysql和[MySQL Connector/C++](https://dev.mysql.com/downloads/connector/cpp/)

## 构建
```bash
cmake -S . -B build
cmake --build build
```
或使用DEBUG构建
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```
