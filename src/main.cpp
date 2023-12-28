#include <Windows.h>
#include <filesystem>
#include <iostream>
#include <vector>
#include "inject.h"

using namespace std;

vector<unsigned char> load_file(const string &filename) {
    ifstream file(filename, ios::binary | ios::ate);

    if(!file.is_open())
        throw runtime_error("unable to open file");

    auto size = file.tellg();
    file.seekg(0, ios::beg);

    vector<unsigned char> data(size);

    if(!file.read(reinterpret_cast<char *>(data.data()), size))
        throw runtime_error("unable to read file");

    file.close();

    return data;
}

bool choose_file(char *path, const char *title) {
    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrTitle = title;
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = "Executable Files\0*.exe;*.dll\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    WINBOOL result = GetOpenFileNameA(&ofn);
    return result;
}

int main(int argc, char **argv) try {
    const char *config_path;

    {
        string argv0 = argv[0];
        argv0 = argv0.erase(argv0.length() - strlen(".exe"));
        config_path = strdup((filesystem::current_path().string() + "\\" + argv0 + ".ini").c_str());
    }
    char game_path[MAX_PATH]{ 0 };
    char dll_path[MAX_PATH]{ 0 };

    const char *ini_path = "Path";
    const char *ini_game_path = "GamePath";
    const char *ini_dll_path = "DllPath";
    auto bytesRead = GetPrivateProfileStringA(ini_path, ini_game_path, "", game_path, MAX_PATH, config_path);
    auto bytesRead2 = GetPrivateProfileStringA(ini_path, ini_dll_path, "", dll_path, MAX_PATH, config_path);
    if(bytesRead == 0 || bytesRead2 == 0) {
        // 配置不存在
        if(!choose_file(game_path, "选择游戏") || !choose_file(dll_path, "选择dll")) {
            throw std::runtime_error("游戏启动失败");
        }
        WritePrivateProfileStringA(ini_path, ini_game_path, game_path, config_path);
        WritePrivateProfileStringA(ini_path, ini_dll_path, dll_path, config_path);
    }
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    if(!CreateProcessA(NULL, (char *)game_path, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi))
        throw std::runtime_error("游戏启动失败");

    auto data = load_file(dll_path);
    if(!inject::ManualMapDll(pi.hProcess, data.data(), data.size()))
        throw std::runtime_error("unable to inject dll");
    ResumeThread(pi.hThread);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return 0;
} catch(std::runtime_error &err) {
    MessageBoxA(0, err.what(), "Fatal error", 0);
}