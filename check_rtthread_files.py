#!/usr/bin/env python3
"""
RT-Thread 适配文件检查脚本
用于验证所有必要的文件是否已正确创建
"""

import os
import sys
from pathlib import Path

def check_file_exists(file_path, description):
    """检查文件是否存在"""
    if os.path.exists(file_path):
        print(f"✅ {description}: {file_path}")
        return True
    else:
        print(f"❌ {description}: {file_path} (缺失)")
        return False

def check_directory_exists(dir_path, description):
    """检查目录是否存在"""
    if os.path.exists(dir_path) and os.path.isdir(dir_path):
        print(f"✅ {description}: {dir_path}")
        return True
    else:
        print(f"❌ {description}: {dir_path} (缺失)")
        return False

def check_file_content(file_path, search_text, description):
    """检查文件是否包含特定内容"""
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
            if search_text in content:
                print(f"✅ {description}: 在 {file_path} 中找到")
                return True
            else:
                print(f"❌ {description}: 在 {file_path} 中未找到")
                return False
    except Exception as e:
        print(f"❌ {description}: 无法读取 {file_path} - {e}")
        return False

def main():
    """主检查函数"""
    print("🔍 检查 RT-Thread 适配文件...")
    print("=" * 60)
    
    base_path = Path(__file__).parent
    all_good = True
    
    # 核心实现文件
    print("\n📁 核心实现文件:")
    files_to_check = [
        (base_path / "include/zenoh-pico/system/platform/rtthread.h", "RT-Thread 平台头文件"),
        (base_path / "src/system/rtthread/system.c", "RT-Thread 系统实现"),
        (base_path / "src/system/rtthread/network.c", "RT-Thread 网络实现"),
        (base_path / "include/zenoh-pico/config_rtthread.h", "RT-Thread 配置文件"),
    ]
    
    for file_path, description in files_to_check:
        if not check_file_exists(file_path, description):
            all_good = False
    
    # 配置集成文件
    print("\n⚙️ 配置集成文件:")
    config_files = [
        (base_path / "SConscript", "SCons 构建脚本"),
        (base_path / "Kconfig", "Kconfig 配置文件"),
        (base_path / "package.json", "RT-Thread 软件包配置"),
    ]
    
    for file_path, description in config_files:
        if not check_file_exists(file_path, description):
            all_good = False
    
    # 检查修改的文件
    print("\n🔧 修改的文件:")
    modified_files = [
        (base_path / "include/zenoh-pico/config.h", "ZENOH_RTTHREAD", "config.h 中的 RT-Thread 支持"),
        (base_path / "include/zenoh-pico/system/common/platform.h", "ZENOH_RTTHREAD", "platform.h 中的 RT-Thread 支持"),
    ]
    
    for file_path, search_text, description in modified_files:
        if not check_file_content(file_path, search_text, description):
            all_good = False
    
    # 示例程序
    print("\n📚 示例程序:")
    if not check_directory_exists(base_path / "examples/rtthread", "示例程序目录"):
        all_good = False
    else:
        example_files = [
            (base_path / "examples/rtthread/z_pub.c", "发布者示例"),
            (base_path / "examples/rtthread/z_sub.c", "订阅者示例"),
            (base_path / "examples/rtthread/z_get.c", "GET 查询示例"),
            (base_path / "examples/rtthread/z_put.c", "PUT 操作示例"),
            (base_path / "examples/rtthread/z_test.c", "测试程序"),
        ]
        
        for file_path, description in example_files:
            if not check_file_exists(file_path, description):
                all_good = False
    
    # 文档
    print("\n📖 文档:")
    doc_files = [
        (base_path / "README_RTTHREAD.md", "RT-Thread 使用文档"),
        (base_path / "RTTHREAD_ADAPTATION_SUMMARY.md", "适配工作总结"),
        (base_path / "PR_DESCRIPTION.md", "PR 描述文档"),
        (base_path / "SUBMIT_PR_GUIDE.md", "PR 提交指南"),
    ]
    
    for file_path, description in doc_files:
        if not check_file_exists(file_path, description):
            all_good = False
    
    # 总结
    print("\n" + "=" * 60)
    if all_good:
        print("🎉 所有文件检查通过！RT-Thread 适配已准备就绪。")
        print("\n📋 下一步:")
        print("1. 阅读 SUBMIT_PR_GUIDE.md")
        print("2. Fork zenoh-pico 项目")
        print("3. 复制所有文件到你的 fork")
        print("4. 提交 Pull Request")
        return 0
    else:
        print("⚠️  发现缺失文件，请检查并补充。")
        return 1

if __name__ == "__main__":
    sys.exit(main())