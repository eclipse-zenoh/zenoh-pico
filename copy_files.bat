@echo off
echo 🚀 RT-Thread 适配文件复制脚本
echo ================================

set SOURCE_DIR=E:\Programing\Embedded_Drivers_And_Tools\modules\zenoh-pico
set TARGET_DIR=%1

if "%TARGET_DIR%"=="" (
    echo ❌ 请提供目标目录路径
    echo 用法: copy_files.bat "C:\Users\James\zenoh-pico-pr"
    pause
    exit /b 1
)

if not exist "%TARGET_DIR%" (
    echo ❌ 目标目录不存在: %TARGET_DIR%
    pause
    exit /b 1
)

echo 📁 源目录: %SOURCE_DIR%
echo 📁 目标目录: %TARGET_DIR%
echo.

echo 📋 创建目录结构...
mkdir "%TARGET_DIR%\include\zenoh-pico\system\platform" 2>nul
mkdir "%TARGET_DIR%\src\system\rtthread" 2>nul
mkdir "%TARGET_DIR%\examples\rtthread" 2>nul

echo ✅ 复制核心实现文件...
copy "%SOURCE_DIR%\include\zenoh-pico\system\platform\rtthread.h" "%TARGET_DIR%\include\zenoh-pico\system\platform\" >nul
copy "%SOURCE_DIR%\src\system\rtthread\system.c" "%TARGET_DIR%\src\system\rtthread\" >nul
copy "%SOURCE_DIR%\src\system\rtthread\network.c" "%TARGET_DIR%\src\system\rtthread\" >nul
copy "%SOURCE_DIR%\include\zenoh-pico\config_rtthread.h" "%TARGET_DIR%\include\zenoh-pico\" >nul

echo ✅ 复制构建文件...
copy "%SOURCE_DIR%\SConscript" "%TARGET_DIR%\" >nul
copy "%SOURCE_DIR%\Kconfig" "%TARGET_DIR%\" >nul
copy "%SOURCE_DIR%\package.json" "%TARGET_DIR%\" >nul

echo ✅ 复制示例程序...
copy "%SOURCE_DIR%\examples\rtthread\z_pub.c" "%TARGET_DIR%\examples\rtthread\" >nul
copy "%SOURCE_DIR%\examples\rtthread\z_sub.c" "%TARGET_DIR%\examples\rtthread\" >nul
copy "%SOURCE_DIR%\examples\rtthread\z_get.c" "%TARGET_DIR%\examples\rtthread\" >nul
copy "%SOURCE_DIR%\examples\rtthread\z_put.c" "%TARGET_DIR%\examples\rtthread\" >nul
copy "%SOURCE_DIR%\examples\rtthread\z_test.c" "%TARGET_DIR%\examples\rtthread\" >nul

echo ✅ 复制文档...
copy "%SOURCE_DIR%\README_RTTHREAD.md" "%TARGET_DIR%\" >nul

echo.
echo 🎉 文件复制完成！
echo.
echo ⚠️  还需要手动更新以下文件:
echo 1. %TARGET_DIR%\include\zenoh-pico\config.h
echo    添加: #elif defined(ZENOH_RTTHREAD)
echo          #include "zenoh-pico/config_rtthread.h"
echo.
echo 2. %TARGET_DIR%\include\zenoh-pico\system\common\platform.h
echo    添加: #elif defined(ZENOH_RTTHREAD)
echo          #include "zenoh-pico/system/platform/rtthread.h"
echo.
echo 📋 下一步:
echo 1. 手动更新上述两个文件
echo 2. 在目标目录中运行: git add .
echo 3. 提交更改: git commit -m "feat: Add RT-Thread platform support"
echo 4. 推送: git push origin feature/rtthread-support
echo.
pause