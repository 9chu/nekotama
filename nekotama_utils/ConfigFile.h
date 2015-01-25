#pragma once
#include <unordered_map>
#include <string>

namespace nekotama
{
	/// @brief ÅäÖÃÎÄ¼ş
	class ConfigFile
	{
	private:
		std::unordered_map<std::string, std::string> m_KeyValuePairs;
	private:
		void parseLine(const std::string& line);
	public:
		std::string& operator[](const std::string& key);
		bool ContainsKey(const std::string& key)const;
		void Clear();
		bool Load(const std::string& filepath, bool bOverride=false);
		void Save(const std::string& filepath)const;
	public:
		ConfigFile();
	};
}
