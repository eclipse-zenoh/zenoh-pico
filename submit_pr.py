#!/usr/bin/env python3
"""
RT-Thread 适配 PR 提交脚本
自动化 Pull Request 提交流程
"""

import os
import sys
import subprocess
import shutil
from pathlib import Path

def run_command(cmd, cwd=None, check=True):
    """运行命令并返回结果"""
    try:
        result = subprocess.run(cmd, shell=True, cwd=cwd, check=check, 
                              capture_output=True, text=True, encoding='utf-8')
        return result.stdout.strip(), result.stderr.strip()
    except subprocess.CalledProcessError as e:
        print(f"❌ 命令执行失败: {cmd}")
        print(f"错误信息: {e.stderr}")
        return None, e.stderr

def check_git_installed():
    """检查 Git 是否已安装"""
    stdout, stderr = run_command("git --version", check=False)
    if stdout and "git version" in stdout:
        print(f"✅ Git 已安装: {stdout}")
        return True
    else:
        print("❌ Git 未安装或不在 PATH 中")
        return False

def get_user_input(prompt, default=None):
    """获取用户输入"""
    if default:
        user_input = input(f"{prompt} (默认: {default}): ").strip()
        return user_input if user_input else default
    else:
        return input(f"{prompt}: ").strip()

def main():
    """主函数"""
    print("🚀 RT-Thread 适配 PR 提交脚本")
    print("=" * 50)
    
    # 检查 Git
    if not check_git_installed():
        print("\n请先安装 Git: https://git-scm.com/download/windows")
        return 1
    
    # 获取用户信息
    print("\n📝 请提供以下信息:")
    github_username = 'clolckliang'
    if not github_username:
        print("❌ GitHub 用户名不能为空")
        return 1
    
    fork_url = f"https://github.com/{github_username}/zenoh-pico.git"
    
    # 确认信息
    print(f"\n📋 确认信息:")
    print(f"GitHub 用户名: {github_username}")
    print(f"Fork URL: {fork_url}")
    
    confirm = get_user_input("确认无误? (y/N)", "N").lower()
    if confirm != 'y':
        print("❌ 已取消")
        return 1
    
    # 创建工作目录
    work_dir = Path("E:/Programing/Embedded_Drivers_And_Tools/modules/zenoh-pico-pr")

    print(f"\n📁 工作目录: {work_dir}")
    
    if work_dir.exists():
        print("⚠️  工作目录已存在")
        overwrite = get_user_input("是否覆盖? (y/N)", "N").lower()
        if overwrite == 'y':
            shutil.rmtree(work_dir)
        else:
            print("❌ 已取消")
            return 1
    
    work_dir.mkdir(parents=True, exist_ok=True)
    
    try:
        # 克隆你的 fork
        print("\n📥 克隆你的 fork...")
        stdout, stderr = run_command(f"git clone {fork_url} .", cwd=work_dir)
        if stderr and "fatal" in stderr:
            print(f"❌ 克隆失败: {stderr}")
            print("请确保你已经 fork 了 zenoh-pico 项目")
            print("访问: https://github.com/eclipse-zenoh/zenoh-pico")
            return 1
        print("✅ 克隆成功")
        
        # 添加上游仓库
        print("\n🔗 添加上游仓库...")
        run_command("git remote add upstream https://github.com/eclipse-zenoh/zenoh-pico.git", cwd=work_dir)
        
        # 获取最新代码
        print("\n🔄 获取最新代码...")
        run_command("git fetch upstream", cwd=work_dir)
        run_command("git checkout main", cwd=work_dir)
        run_command("git merge upstream/main", cwd=work_dir)
        
        # 创建功能分支
        print("\n🌿 创建功能分支...")
        run_command("git checkout -b feature/rtthread-support", cwd=work_dir)
        
        # 复制适配文件
        print("\n📋 复制适配文件...")
        source_dir = Path(__file__).parent
        
        # 复制文件列表
        files_to_copy = [
            # 核心实现文件
            ("include/zenoh-pico/system/platform/rtthread.h", "include/zenoh-pico/system/platform/rtthread.h"),
            ("src/system/rtthread/system.c", "src/system/rtthread/system.c"),
            ("src/system/rtthread/network.c", "src/system/rtthread/network.c"),
            ("include/zenoh-pico/config_rtthread.h", "include/zenoh-pico/config_rtthread.h"),
            
            # 配置文件
            ("SConscript", "SConscript"),
            ("Kconfig", "Kconfig"),
            ("package.json", "package.json"),
            
            # 示例程序
            ("examples/rtthread/z_pub.c", "examples/rtthread/z_pub.c"),
            ("examples/rtthread/z_sub.c", "examples/rtthread/z_sub.c"),
            ("examples/rtthread/z_get.c", "examples/rtthread/z_get.c"),
            ("examples/rtthread/z_put.c", "examples/rtthread/z_put.c"),
            ("examples/rtthread/z_test.c", "examples/rtthread/z_test.c"),
            
            # 文档
            ("README_RTTHREAD.md", "README_RTTHREAD.md"),
        ]
        
        for src_file, dst_file in files_to_copy:
            src_path = source_dir / src_file
            dst_path = work_dir / dst_file
            
            if src_path.exists():
                # 创建目标目录
                dst_path.parent.mkdir(parents=True, exist_ok=True)
                # 复制文件
                shutil.copy2(src_path, dst_path)
                print(f"✅ 复制: {src_file}")
            else:
                print(f"⚠️  文件不存在: {src_file}")
        
        # 手动更新的文件提示
        print("\n⚠️  需要手动更新以下文件:")
        print("1. include/zenoh-pico/config.h - 添加 RT-Thread 配置支持")
        print("2. include/zenoh-pico/system/common/platform.h - 添加 RT-Thread 平台检测")
        
        # 显示需要添加的内容
        print("\n📝 需要在 config.h 中添加:")
        print("#elif defined(ZENOH_RTTHREAD)")
        print("#include \"zenoh-pico/config_rtthread.h\"")
        
        print("\n📝 需要在 platform.h 中添加:")
        print("#elif defined(ZENOH_RTTHREAD)")
        print("#include \"zenoh-pico/system/platform/rtthread.h\"")
        
        input("\n按 Enter 继续，请先手动更新上述文件...")
        
        # 添加文件到 Git
        print("\n📦 添加文件到 Git...")
        run_command("git add .", cwd=work_dir)
        
        # 检查状态
        stdout, _ = run_command("git status --porcelain", cwd=work_dir)
        if not stdout:
            print("⚠️  没有检测到文件更改，请确保已正确复制文件")
            return 1
        
        # 提交更改
        print("\n💾 提交更改...")
        commit_message = """feat: Add RT-Thread platform support

- Implement platform abstraction layer for RT-Thread RTOS
- Add network layer with TCP/UDP unicast/multicast support  
- Integrate with RT-Thread build system (SCons, Kconfig)
- Provide comprehensive examples and documentation
- Support RT-Thread 4.0+ with lwIP network stack
- Memory optimized for embedded environments (16-32KB RAM)

Features implemented:
- Multi-threading with RT-Thread primitives
- Memory management using RT-Thread allocators
- Network communication (TCP/UDP unicast/multicast)
- Time management and sleep functions
- Synchronization primitives (mutexes, condition variables)
- Random number generation
- MSH command integration for examples
- Kconfig-based configuration
- SCons build system integration

Testing completed:
- Basic functionality (memory, threads, mutexes, time)
- Network operations (TCP/UDP sockets, multicast)
- Zenoh protocol (sessions, pub/sub, query/reply)
- Integration with RT-Thread build system"""
        
        run_command(f'git commit -m "{commit_message}"', cwd=work_dir)
        
        # 推送到 fork
        print("\n🚀 推送到你的 fork...")
        run_command("git push origin feature/rtthread-support", cwd=work_dir)
        
        # 成功信息
        print("\n" + "=" * 50)
        print("🎉 文件已成功推送到你的 fork!")
        print("\n📋 下一步:")
        print("1. 访问你的 fork:")
        print(f"   https://github.com/{github_username}/zenoh-pico")
        print("2. 点击 'Compare & pull request' 按钮")
        print("3. 使用以下信息填写 PR:")
        print("   标题: Add RT-Thread platform support")
        print("   描述: 使用 PR_DESCRIPTION.md 中的内容")
        print("\n🔗 原项目地址:")
        print("   https://github.com/eclipse-zenoh/zenoh-pico")
        
        return 0
        
    except Exception as e:
        print(f"\n❌ 发生错误: {e}")
        return 1

if __name__ == "__main__":
    sys.exit(main())