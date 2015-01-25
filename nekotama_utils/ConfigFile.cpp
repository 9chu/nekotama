#include "ConfigFile.h"

#include <fstream>

using namespace std;
using namespace nekotama;

inline static void trim(string& text)
{
	if (!text.empty())
	{
		auto l = text.find_first_not_of(" \n\r\t");
		if (l != string::npos)
			text.erase(0, l);
		auto r = text.find_last_not_of(" \n\r\t");
		if (r != string::npos)
			text.erase(r + 1);
	}
}

ConfigFile::ConfigFile()
{}

void ConfigFile::parseLine(const std::string& line)
{
	int state = 0;
	std::string key, value;
	for (size_t i = 0; i < line.size(); ++i)
	{
		char c = line[i];
		switch (state)
		{
		case 0:
			if (c == '=')
			{
				trim(key);
				if (key.empty())
					return;
				state = 1;
			}
			else if (c == '#')
				return;
			else
				key.push_back(c);
			break;
		case 1:
			if (c == '#')
			{
				trim(value);
				m_KeyValuePairs[key] = value;
				return;
			}
			else
				value.push_back(c);
			break;
		}
	}
	trim(value);
	m_KeyValuePairs[key] = value;
}

std::string& ConfigFile::operator[](const std::string& key)
{
	return m_KeyValuePairs[key];
}

bool ConfigFile::ContainsKey(const std::string& key)const
{
	return (m_KeyValuePairs.find(key) != m_KeyValuePairs.end());
}

void ConfigFile::Clear()
{
	m_KeyValuePairs.clear();
}

bool ConfigFile::Load(const std::string& filepath, bool bOverride)
{
	if (!bOverride)
		Clear();

	fstream f(filepath, ios::in | ios::binary);
	if (f)
	{
		std::string t;
		while (!f.eof())
		{
			char c = '\0';
			f.read(&c, 1);
			t.push_back(c);
			if (c == '\0' || c == '\n')
			{
				parseLine(t);
				t.clear();
			}
		}
		parseLine(t);
		f.close();
		return true;
	}
	else
		return false;
}

void ConfigFile::Save(const std::string& filepath)const
{
	fstream f(filepath, ios::out);
	if (f)
	{
		for (auto i : m_KeyValuePairs)
		{
			std::string line = i.first + "=" + i.second;
			f << line << endl;
		}
		f.close();
	}
}
