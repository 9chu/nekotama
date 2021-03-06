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
				delay       上一次延时
			6. PONG
			7. 登出

			8. 客户端->服务端数据包
				target      目标ip
				tport       目标端口
				sport       源端口
				data        数据
			9. 服务端->客户端数据包
				source      源ip
				sport       源端口
				tport       目标端口
				data        数据
			10. 房间建立
			11. 房间撤销
			12. 查询房间信息
			13. 房间信息反馈
				hosts       房间列表
					nick    昵称
					target  目标ip
					port    目标端口
					delay   延迟
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
		Logout = 7,
		SendPackage = 8,
		RecvPackage = 9,
		CreateHost = 10,
		DestroyHost = 11,
		QueryGame = 12,
		GameInfo = 13
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
        T GetPackageField(const Bencode::Value& v, const std::string& field);

		template<>
        inline int GetPackageField(const Bencode::Value& v, const std::string& field)
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
        inline const std::string& GetPackageField(const Bencode::Value& v, const std::string& field)
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
