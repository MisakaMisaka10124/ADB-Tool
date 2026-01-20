#include "include/ADB-function.h"
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>

// 格式化打印设备列表
void printDeviceList(const std::vector<ADB_Device>& devices) {
    if (devices.empty()) {
        std::cout << "\n[!] 未发现任何设备，请检查 USB 连接或驱动。" << std::endl;
        return;
    }
    std::cout << "\n" << std::left
        << std::setw(20) << "序列号"
        << std::setw(15) << "状态"
        << std::setw(10) << "版本"
        << "型号" << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    for (const auto& d : devices) {
        std::cout << std::left
            << std::setw(20) << d.serial
            << std::setw(15) << d.status
            << std::setw(10) << d.version
            << d.model << std::endl;
    }
}

void showMenu() {
    std::cout << "\n================= ADB 工具测试终端 =================" << std::endl;
    std::cout << "1. 刷新设备列表" << std::endl;
    std::cout << "2. 无线调试: 传统连接 (IP:Port)" << std::endl;
    std::cout << "3. 无线调试: 安卓11+ 配对 (IP:Port + Code)" << std::endl;
    std::cout << "4. 应用管理: 安装 APK" << std::endl;
    std::cout << "5. 应用管理: 卸载 APK (包名)" << std::endl;
    std::cout << "6. 文件传输: 推送文件 (Push)" << std::endl;
    std::cout << "7. 文件传输: 拉取文件 (Pull)" << std::endl;
    std::cout << "8. 系统工具: 屏幕截图 (保存至当前目录)" << std::endl;
    std::cout << "9. 重启选项: 重启手机 / Recovery / Fastboot" << std::endl;
    std::cout << "10. 线刷功能: Rec Sideload (.zip)" << std::endl;
    std::cout << "11. 线刷功能: Fastboot 刷入 (Rec/Boot/GSI)" << std::endl;
    std::cout << "0. 退出程序" << std::endl;
    std::cout << "====================================================" << std::endl;
    std::cout << "请选择操作: ";
}

int main() {

    // 启动自检逻辑
    if (!ADB_function::checkADB()) {
        std::cout << "--- 运行环境自检 ---" << std::endl;
        std::cout << "[!] 未发现 ADB 核心组件。" << std::endl;
        std::cout << "[>] 是否自动从 Google 官方下载最新版组件？(y/n): ";
        char confirm;
        std::cin >> confirm;

        if (confirm == 'y' || confirm == 'Y') {
            if (!ADB_function::downloadADB()) {
                std::cout << "[X] 自动下载失败，请检查网络或手动下载 platform-tools。" << std::endl;
                system("pause");
                return -1;
            }
        }
        else {
            std::cout << "[!] 缺少组件，程序退出。" << std::endl;
            return -1;
        }
    }

    int choice;
    std::string serial, ip, port, code, path, remote;

	//循环主菜单
    while (true) {
        showMenu();
        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(1000, '\n');
            continue;
        }

        if (choice == 0) break;

        switch (choice) {
        case 1: {
            auto list = ADB_function::getDevice();
            printDeviceList(list);
            break;
        }
        case 2: {
            std::string ip, port;
            std::cout << "输入 IP: "; std::cin >> ip;
            std::cout << "输入端口 (1-65535): "; std::cin >> port;

            try {
                int p = std::stoi(port); // 尝试转为数字
                if (p < 1 || p > 65535) {
                    std::cout << "[!] 错误：端口范围无效！" << std::endl;
                }
                else {
                    if (ADB_function::connectWireless(ip, port))
                        std::cout << "[+] 连接成功" << std::endl;
                }
            }
            catch (...) {
                std::cout << "[!] 错误：端口必须是数字！" << std::endl;
            }
            break;
        }

        case 3: {
            std::string inputIP, inputPort, code;

            // 第一步：获取配对用的 IP 和 端口
            std::cout << "1. 请输入手机'配对设备'弹窗显示的地址 (IP:Port 或仅 IP): ";
            std::cin >> inputIP;

            size_t colonPos = inputIP.find(':');
            if (colonPos != std::string::npos) {
                inputPort = inputIP.substr(colonPos + 1);
                inputIP = inputIP.substr(0, colonPos);
                std::cout << "[*] 自动识别配对端口: " << inputPort << std::endl;
            }
            else {
                std::cout << "   请输入配对端口号: ";
                std::cin >> inputPort;
            }

            // 第二步：获取配对码
            std::cout << "2. 请输入 6 位配对码: ";
            std::cin >> code;

            // 执行配对逻辑
            std::cout << "[*] 正在尝试配对 " << inputIP << ":" << inputPort << " ..." << std::endl;

            if (ADB_function::connectWireless_11(inputIP, inputPort, code)) {
                std::cout << "[+] 配对成功！" << std::endl;

                // --- 核心优化：配对成功后立即引导连接 ---
                std::cout << "\n[?] 注意：配对端口已失效。请看手机'无线调试'主界面" << std::endl;
                std::cout << "[?] 此时显示的'IP地址和端口'是多少？" << std::endl;

                std::string newPort;
                std::cout << "请输入新的连接端口: ";
                std::cin >> newPort;

                std::cout << "[*] 正在尝试最终连接 " << inputIP << ":" << newPort << " ..." << std::endl;
                if (ADB_function::connectWireless(inputIP, newPort)) {
                    std::cout << "[+++] 无线连接已彻底建立！现在可以使用其他功能了。" << std::endl;
                }
                else {
                    std::cout << "[-] 连接失败。请手动尝试选项 2 进行连接。" << std::endl;
                }
            }
            else {
                std::cout << "[-] 配对失败。请检查：1.手机电脑是否在同一Wi-Fi 2.配对码是否正确。" << std::endl;
            }
            break;
        }

        case 4:
            std::cout << "输入设备序列号和 APK 路径: ";
            std::cin >> serial >> path;
            if (ADB_function::installAPK(serial, path)) std::cout << "[+] 安装成功" << std::endl;
            else std::cout << "[-] 安装失败" << std::endl;
            break;

        case 8:
            std::cout << "输入目标设备序列号: ";
            std::cin >> serial;
            if (ADB_function::screenshot(serial, "screenshot.png"))
                std::cout << "[+] 截图已保存为 screenshot.png" << std::endl;
            else std::cout << "[-] 截图失败" << std::endl;
            break;

        case 10: {
            std::cout << "输入设备序列号和 ZIP 包路径: ";
            std::cin >> serial >> path;

            // 刷机前的安全性检查
            auto list = ADB_function::getDevice();
            bool isSideload = false;
            for (auto& d : list) {
                if (d.serial == serial && d.status == "sideload") isSideload = true;
            }

            if (!isSideload) {
                std::cout << "[!] 警告：设备未处于 sideload 模式，请先重启到 Rec 并开启 Sideload。" << std::endl;
            }
            else {
                std::cout << "[*] 正在刷入，请勿拔出数据线..." << std::endl;
                if (ADB_function::sideloadZIP(serial, path)) std::cout << "[+] 刷入成功！" << std::endl;
                else std::cout << "[-] 刷入过程中出现错误。" << std::endl;
            }
            break;
        }

        case 11:
            std::cout << "1.刷入Rec  2.刷入Boot  3.刷入GSI(System)\n请选择: ";
            int sub; std::cin >> sub;
            std::cout << "输入镜像路径: "; std::cin >> path;
            // 注意：Fastboot 模式下 serial 可能变化，建议直接刷入或先通过 fastboot devices 确认
            if (sub == 1) ADB_function::flashRec("", path);
            else if (sub == 2) ADB_function::flashBoot("", path);
            else if (sub == 3) ADB_function::flashGSI("", path);
            std::cout << "[*] Fastboot 命令已发送。" << std::endl;
            break;

        default:
            std::cout << "[!] 无效选项" << std::endl;
            break;
        }
    }

    return 0;
}