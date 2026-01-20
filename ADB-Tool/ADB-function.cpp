#include "include/ADB-function.h"
#include <cstdio>
#include <memory>
#include <array>
#include <sstream>
#include <algorithm>
#include <future>
#include <filesystem>
#include<iostream>
namespace fs = std::filesystem;


// --- 底层驱动：静默执行命令并捕获输出 ---
std::string ADB_function::run_cmd(const std::string& command) {
    std::array<char, 128> buffer;
    std::string result;

#ifdef _WIN32
    // Windows 下使用 _popen，避免弹出黑窗口（GUI环境下通常是静默的）
    auto pipe = _popen(command.c_str(), "r");
    auto close_pipe = _pclose;
#else
    auto pipe = popen(command.c_str(), "r");
    auto close_pipe = pclose;
#endif

    if (!pipe) return "";

    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        result += buffer.data();
    }
    close_pipe(pipe);
    return result;
}

// --- 设备管理：异步并发获取 ---
std::vector<ADB_Device> ADB_function::getDevice() {
    std::vector<ADB_Device> devices;
    std::string output = run_cmd("adb devices");
    std::stringstream ss(output);
    std::string line;

    // 跳过第一行 "List of devices attached"
    std::getline(ss, line);

    struct BasicInfo { std::string sn; std::string st; };
    std::vector<BasicInfo> targets;

    while (std::getline(ss, line)) {
        if (line.empty() || line.find("*") != std::string::npos) continue;
        std::stringstream ls(line);
        std::string sn, st;
        if (ls >> sn >> st) targets.push_back({ sn, st });
    }

    // 开启异步任务并行查询属性
    std::vector<std::future<ADB_Device>> tasks;
    for (auto& target : targets) {
        tasks.push_back(std::async(std::launch::async, [target]() {
            ADB_Device dev;
            dev.serial = target.sn;
            dev.status = target.st;

            if (target.st == "device") {
                // 合并查询指令：一次 shell 获取两个属性，大幅提速
                std::string cmd = "adb -s " + target.sn + " shell \"getprop ro.build.version.release && getprop ro.product.model\"";
                std::string info = run_cmd(cmd);
                std::stringstream is(info);

                std::string ver, mdl;
                std::getline(is, ver);
                std::getline(is, mdl);

                // Lambda 清理换行符
                auto trim = [](std::string& s) {
                    s.erase(std::remove(s.begin(), s.end(), '\r'), s.end());
                    s.erase(std::remove(s.begin(), s.end(), '\n'), s.end());
                    };
                trim(ver); trim(mdl);

                dev.version = ver.empty() ? "Unknown" : ver;
                dev.model = mdl.empty() ? "Unknown Device" : mdl;
            }
            else {
                dev.version = "N/A";
                dev.model = "N/A";
            }
            return dev;
            }));
    }

    for (auto& t : tasks) devices.push_back(t.get());
    return devices;
}

// --- 无线连接模块 ---
bool ADB_function::connectWireless_11(const std::string& ip, const std::string& port, const std::string& code) {
    // Android 11 配对命令
    std::string res = run_cmd("adb pair " + ip + ":" + port + " " + code);
    return res.find("Successfully paired") != std::string::npos;
}

bool ADB_function::connectWireless(const std::string& ip, const std::string& port) {
    std::string res = run_cmd("adb connect " + ip + ":" + port);
    return res.find("connected to") != std::string::npos;
}

bool ADB_function::disconnectWireless(const std::string& ip, const std::string& port) {
    std::string res = run_cmd("adb disconnect " + ip + ":" + port);
    return res.find("disconnected") != std::string::npos;
}

// --- 文件与应用管理 ---
bool ADB_function::installAPK(const std::string& serial, const std::string& apkPath) {
    std::string res = run_cmd("adb -s " + serial + " install -r " + quotePath(apkPath));
    return res.find("Success") != std::string::npos;
}

bool ADB_function::uninstallAPK(const std::string& serial, const std::string& packageName) {
    std::string res = run_cmd("adb -s " + serial + " uninstall " + packageName);
    return res.find("Success") != std::string::npos;
}

bool ADB_function::pushFile(const std::string& serial, const std::string& localPath, const std::string& remotePath) {
    std::string res = run_cmd("adb -s " + serial + " push " + quotePath(localPath) + " " + quotePath(remotePath));
    // 检查是否有错误提示
    return res.find("error") == std::string::npos && res.find("skipped") == std::string::npos;
}

bool ADB_function::pullFile(const std::string& serial, const std::string& remotePath, const std::string& localPath) {
    std::string res = run_cmd("adb -s " + serial + " pull " + quotePath(remotePath) + " " + quotePath(localPath));
    return res.find("error") == std::string::npos;
}

// --- 系统工具与截图 ---
bool ADB_function::screenshot(const std::string& serial, const std::string& savePath) {
    std::string remote = "/sdcard/screen_cap.png";
    run_cmd("adb -s " + serial + " shell screencap -p " + remote);
    bool ok = pullFile(serial, remote, savePath);
    run_cmd("adb -s " + serial + " shell rm " + remote);
    return ok;
}

bool ADB_function::reboot(const std::string& serial) {
    run_cmd("adb -s " + serial + " reboot");
    return true;
}

bool ADB_function::ToRec(const std::string& serial) {
    run_cmd("adb -s " + serial + " reboot recovery");
    return true;
}

bool ADB_function::ToFastboot(const std::string& serial) {
    run_cmd("adb -s " + serial + " reboot bootloader");
    return true;
}

// --- Rec 线刷模块 ---
bool ADB_function::sideloadZIP(const std::string& serial, const std::string& zipPath) {
    // 命令格式：adb -s <serial> sideload <zip路径>
    // 手机必须处于 Recovery 模式并开启了 "Apply update from ADB"
    std::string cmd = "adb -s " + serial + " sideload " + quotePath(zipPath);

    std::string res = run_cmd(cmd);

    // Sideload 成功通常会输出 "Total xfer: 1.00x"
    // 如果失败会输出 "cannot read", "failed", "closed" 等关键词
    if (res.find("Total xfer") != std::string::npos ||
        (res.find("error") == std::string::npos && !res.empty())) {
        return true;
    }
    return false;
}

// --- Fastboot 刷机模块 ---
bool ADB_function::flashRec(const std::string& serial, const std::string& recPath) {
    std::string cmd = "fastboot " + (serial.empty() ? "" : "-s " + serial) + " flash recovery " + quotePath(recPath);
    return run_cmd(cmd).find("Finished") != std::string::npos;
}

bool ADB_function::flashBoot(const std::string& serial, const std::string& bootPath) {
    std::string cmd = "fastboot " + (serial.empty() ? "" : "-s " + serial) + " flash boot " + quotePath(bootPath);
    return run_cmd(cmd).find("Finished") != std::string::npos;
}

bool ADB_function::flashGSI(const std::string& serial, const std::string& gsiPath) {
    std::string cmd = "fastboot " + (serial.empty() ? "" : "-s " + serial) + " flash system " + quotePath(gsiPath);
    return run_cmd(cmd).find("Finished") != std::string::npos;
}

// --- ADB 可用性检测 ---
bool ADB_function::checkADB() {
    // 1. 检查当前目录下是否存在 adb.exe
    if (fs::exists("adb.exe")) {
        return true;
    }

    // 2. 尝试调用系统 adb 
    std::string versionInfo = run_cmd("adb --version");
    if (versionInfo.find("Android Debug Bridge") != std::string::npos) {
        return true;
    }

    return false;
}

// --- ADB 下载与部署 ---
bool ADB_function::downloadADB() {
    std::cout << "[*] 正在准备下载 ADB 组件 (来自 Google 官方源)..." << std::endl;

    // Google 官方 Platform Tools Windows 版下载地址
    std::string url = "https://dl.google.com/android/repository/platform-tools-latest-windows.zip";

    // 使用 PowerShell 执行下载和解压的命令
    // 1. 下载到 temp.zip
    // 2. 解压到当前目录
    // 3. 删除 temp.zip
    std::string psCmd = "powershell -Command \"Invoke-WebRequest -Uri '" + url + "' -OutFile 'pt.zip'; "
        "Expand-Archive -Path 'pt.zip' -DestinationPath './temp_adb' -Force; "
        "Move-Item -Path './temp_adb/platform-tools/*' -Destination './' -Force; "
        "Remove-Item -Path './temp_adb' -Recurse; "
        "Remove-Item -Path 'pt.zip'\"";

    std::cout << "[*] 正在下载，请稍候 (取决于网络速度)..." << std::endl;
    run_cmd(psCmd);

    // 验证下载结果
    if (fs::exists("adb.exe")) {
        std::cout << "[+] ADB 组件部署成功！" << std::endl;
        return true;
    }
    return false;
}