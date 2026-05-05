#ifndef APPPACK_INSTALLER_HEADER_READER_H
#define APPPACK_INSTALLER_HEADER_READER_H

#include <string>
#include "common.h"

// 从自解压安装包中读取并解析包头部
// 文件结构: [安装器运行时] [PackageHeader] [压缩数据]
PackageHeader readHeader(const std::string& self_path);

#endif // APPPACK_INSTALLER_HEADER_READER_H
