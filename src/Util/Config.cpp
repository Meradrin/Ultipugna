#include "Config.h"

#include <fstream>
#include <sstream>
#include "SDL.h"

void Config::SetArray(const std::string& Key, const std::vector<std::string>& Values)
{
    for (std::size_t Index = 0; Index < Values.size(); ++Index)
    {
        std::stringstream KeyStream;
        KeyStream << Key << "[" << Index << "]";
        Data[KeyStream.str()] = Values[Index];
    }
}

void Config::GetArray(const std::string& Key, std::vector<std::string>& Values) const
{
    using IndexAndKey = std::tuple<std::size_t, std::string>;

    auto SetKeyIndex = [&](std::size_t Index) -> IndexAndKey
    {
        std::stringstream KeyStream;
        KeyStream << Key << "[" << Index << "]";
        return { Index, KeyStream.str() };
    };

    Values.clear();

    for (IndexAndKey IndexKey = SetKeyIndex(0); HasKey(std::get<1>(IndexKey)); IndexKey = SetKeyIndex(std::get<0>(IndexKey) + 1))
    {
        Values.push_back(Data.at(std::get<1>(IndexKey)));
    }
}

void Config::Save() const
{
    if (std::ofstream ConfigStream { FilePath, std::ios::trunc })
    {
        std::string_view CurrentSection;

        for (const auto& [FullKey, Value] : Data)
        {
            std::string_view Section = "";
            std::string_view Key = FullKey;

            if (const std::size_t Index = FullKey.rfind('.'); Index != std::string::npos)
            {
                Section = Key.substr(0, Index);
                Key = Key.substr(Index + 1);
            }

            if (Section != CurrentSection)
            {
                CurrentSection = Section;
                ConfigStream << "\n[" << Section << "]\n";
            }

            ConfigStream << Key << "=" << Value << "\n";
        }
    }
}

bool Config::Load()
{
    if (std::ifstream ConfigFile { FilePath })
    {
        Data.clear();

        std::string CurrentSection;

        for (std::string Line; std::getline(ConfigFile, Line);)
        {
            if (Line.empty() || Line[0] == ';' || Line[0] == '#')
                continue;

            if (Line.front() == '[' && Line.back() == ']')
            {
                CurrentSection = Line.substr(1, Line.size() - 2);
            }
            else
            {
                if (const std::size_t EqualIndex = Line.find('='); EqualIndex != std::string::npos)
                {
                    std::string Key = CurrentSection;
                    Key += '.';
                    Key += std::string_view(Line).substr(0, EqualIndex);
                    Data[Key] = std::string_view(Line).substr(EqualIndex + 1);
                }
                else
                {
                    Data[CurrentSection + "." + Line] = "";
                }
            }
        }

        return true;
    }

    return false;
}

Config::Config()
{
    if (char* PrePath = SDL_GetPrefPath("MeraCorp", "Ultipugna"))
    {
        FilePath = std::string(PrePath) + "Config.ini";
        SDL_free(PrePath);
    }
}
