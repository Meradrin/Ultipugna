#pragma once

#include <map>
#include <string>
#include <vector>

class Config
{
public:
    static Config& Instance() { static Config Instance; return Instance; }

    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    [[nodiscard]] std::string& operator[](const std::string& Key) { return Data[Key]; }
    [[nodiscard]] const std::string& operator[](const std::string& Key) const { return Data.at(Key); }
    [[nodiscard]] bool HasKey(const std::string& Key) const { return Data.contains(Key); }
    [[nodiscard]] const std::string& Get(const std::string& Key, const std::string& DefaultValue) const { return HasKey(Key) ? Data.at(Key) : DefaultValue; }

    void SetArray(const std::string& Key, const std::vector<std::string>& Values);
    void GetArray(const std::string& Key, std::vector<std::string>& Values) const;

    void Save() const;
    bool Load();

private:
    Config();

    std::map<std::string, std::string> Data;
    std::string FilePath;
};