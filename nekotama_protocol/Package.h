#pragma once
#include <stdexcept>
#include <string>

#include <ISocket.h>
#include <Bencode.h>

#define NK_PROTOCOL_MAJOR 0  ///< @brief 协议主版本号
#define NK_PROTOCOL_MINOR 1  ///< @brief 协议次版本号

namespace nekotama
{
	/*
		数据包类型：
			1. 欢迎
				name        服务器名称
				pmaj        协议主版本号
				pmin        协议次版本号
			2. 踢出
				why         对应KickReason，指明被服务器踢出的原因
			3. 登陆
				nick        昵称
			4. 登陆确认
				nick        登陆后实际昵称
				addr        从服务端获得的虚拟ip地址
				port        游戏端口，用于建立房间
			5. PING
			6. PONG
			7. 登出
	*/

	/// @brief 数据包类型
	enum class PackageType
	{
		Welcome = 1,
		Kicked = 2,
		Login = 3,
		LoginConfirm = 4,
		Ping = 5,
		Pong = 6,
		Logout = 7
	};

	/// @brief 客户端会话踢出原因
	enum class KickReason
	{
		ServerIsFull,
		ServerClosed,
		LoginFailed
	};

	namespace PackageHelper
	{
		template<typename T>
		static T GetPackageField(const Bencode::Value& v, const std::string& field);

		template<>
		static int GetPackageField(const Bencode::Value& v, const std::string& field)
		{
			if (v.Type != Bencode::ValueType::Dictionary)
				throw std::logic_error("invalid package format.");
			auto tField = v.VDict.find(field);
			if (tField == v.VDict.end())
				throw std::logic_error(StringFormat("invalid package format, field '%s' required.", field.c_str()));
			if (tField->second->Type != Bencode::ValueType::Int)
				throw std::logic_error(StringFormat("invalid package format, field '%s' should be an integer.", field.c_str()));
			return tField->second->VInt;
		}

		template<>
		static const std::string& GetPackageField(const Bencode::Value& v, const std::string& field)
		{
			if (v.Type != Bencode::ValueType::Dictionary)
				throw std::logic_error("invalid package format.");
			auto tField = v.VDict.find(field);
			if (tField == v.VDict.end())
				throw std::logic_error(StringFormat("invalid package format, field '%s' required.", field.c_str()));
			if (tField->second->Type != Bencode::ValueType::String)
				throw std::logic_error(StringFormat("invalid package format, field '%s' should be a string.", field.c_str()));
			return tField->second->VString;
		}
	}
}
