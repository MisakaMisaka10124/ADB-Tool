#pragma once
#include <string>
#include <vector>

//定义设备，包含序列号和当前状态
struct ADB_Device {
	std::string serial;  //序列号
	std::string status;  //状态
	std::string version; //安卓版本 
	std::string model;   //设备型号
};

//ADB功能
class ADB_function {
public:

	//获取设备列表
	static std::string run_cmd(const std::string& command);
	static std::vector<ADB_Device> getDevice();

	//无线调试连接（高于等于安卓11）
	static bool connectWireless_11(const std::string& ip, const std::string& port = "5555", const std::string& code = "");

	//无线调试连接（低于安卓11）
	static bool connectWireless(const std::string& ip, const std::string& port = "5555");

	//断开无线调试连接
	static bool disconnectWireless(const std::string& ip, const std::string& port = "5555");

	//安装APK
	static bool installAPK(const std::string& serial, const std::string& apkPath);

	//卸载APK
	static bool uninstallAPK(const std::string& serial, const std::string& packageName);

	//粘贴文件到手机
	static bool pushFile(const std::string& serial, const std::string& localPath, const std::string& remotePath);

	//从手机拉取文件
	static bool pullFile(const std::string& serial, const std::string& remotePath, const std::string& localPath);

	//截图并保存到电脑
	static bool screenshot(const std::string& serial, const std::string& savePath);

	//进入rec
	static bool ToRec(const std::string& serial);

	//进入fastboot
	static bool ToFastboot(const std::string& serial);

	//重启设备
	static bool reboot(const std::string& serial);

	//刷入rec
	static bool flashRec(const std::string& serial, const std::string& recPath);

	//刷入boot
	static bool flashBoot(const std::string& serial, const std::string& bootPath);

	//刷入GSI
	static bool flashGSI(const std::string& serial, const std::string& gsiPath);

	//Rec线刷
	static bool sideloadZIP(const std::string& serial, const std::string& zipPath);

	// 检测 ADB 是否可用
	static bool checkADB(); 

	// 下载并解压 ADB (需要联网)
	static bool downloadADB();   

private:

	static std::string quotePath(const std::string& path) {

		//空路径
		if (path.empty()) return "\"\"";

		// 已经有引号
		if (path.front() == '"' && path.back() == '"') {
			return path;
		}
		return "\"" + path + "\"";
	}
};

