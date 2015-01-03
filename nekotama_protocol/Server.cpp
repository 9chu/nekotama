#include "Server.h"

#include <unordered_map>
#include <chrono>

using namespace std;
using namespace nekotama;

// 最小的计时时间步
#define TIMETICKSTEP 16

////////////////////////////////////////////////////////////////////////////////

Server::Server(ISocketFactory* pFactory, ILogger* pLogger, uint16_t port, uint16_t maxClient)
	: m_pFactory(pFactory), m_pLogger(pLogger), m_Port(port), m_Clients(maxClient),
	m_stopFlag(false), m_bRunning(false)
{
	// 创建Socket对象
	m_Socket = m_pFactory->Create(SocketType::TCP);

	// 初始化Socket
	m_Socket->Bind("0.0.0.0", m_Port);
	m_Socket->Listen();

	m_Socket->SetBlockingMode(false);
}

void Server::Start()
{
	if (m_bRunning)
		throw logic_error("server is running.");
	else
	{
		m_bRunning = true;
		m_tSocket = make_shared<thread>(std::bind(&Server::socketThreadLoop, this));
	}
}

void Server::End()
{
	if (m_bRunning)
		m_stopFlag = true;
	else
		throw logic_error("server is not running.");
}

void Server::Wait()
{
	if (m_bRunning)
		m_tSocket->join();
}

bool Server::IsRunning()const throw()
{
	return m_bRunning;
}

////////////////////////////////////////////////////////////////////////////////

#define REMOVEIFHAS(socket, container)  \
	if (container.find(socket) != container.end())  \
	{  \
		container.erase(socket);  \
	}

void Server::socketThreadLoop()NKNOEXCEPT
{
	m_stopFlag = false;
	m_MsgQueue.Clear();

	// 创建消息处理线程
	m_tMsgWork = make_shared<thread>(std::bind(&Server::messageThreadLoop, this));

	// ! 一坨屎 等重写
	{
		// 客户端信息
		unordered_map<SocketHandle, ClientSessionHandle> tSessions;
		SocketHandleSet tAllHandles;

		// 用于select测试的句柄
		SocketHandleSet tReadTestHandles;
		SocketHandleSet tWriteTestHandles;
		SocketHandleSet tErrorTestHandles;

		auto tStartTick = std::chrono::system_clock::now();  // 计时器
		while (!m_stopFlag)  // 执行Socket循环与tick循环
		{
			try
			{
				// 准备Select参数
				tReadTestHandles = tAllHandles;
				tWriteTestHandles = tAllHandles;
				tErrorTestHandles = tAllHandles;
				tReadTestHandles.insert(m_Socket);

				// 执行Select
				if (m_pFactory->Select(&tReadTestHandles, &tWriteTestHandles, &tErrorTestHandles, TIMETICKSTEP))
				{
					// 检查可以接受数据的socket
					for (auto i : tReadTestHandles)
					{
						if (i == m_Socket)  // 新客户端到来
						{
							string tIP;
							uint16_t tPort;
							SocketHandle tHandle;
							try
							{
								if (i->Accept(tIP, tPort, tHandle))
								{
									// 创建一个会话
									ClientSessionHandle tSession = make_shared<ClientSession>(tHandle, tIP, tPort, m_pLogger);
									tAllHandles.insert(tHandle);
									tSessions.insert(pair<SocketHandle, ClientSessionHandle>(tHandle, tSession));

									// 记录日志
									m_pLogger->Log(
										StringFormat(
											"client arrived on '%s:%u'.",
											tIP.c_str(),
											tPort
										),
										LogType::Infomation
									);

									// 发送一个消息
									m_MsgQueue.Push(Message(MessageType::ClientArrival, tSession.get()));
								}
							}
							catch (const std::exception& e)
							{
								m_pLogger->Log(
									StringFormat(
										"exception caught when accept an income client. (%s)", 
										e.what()), 
									LogType::Error
								);
							}
						}
						else  // 通知会话接收消息
						{	
							auto s = tSessions.find(i);
							if (s == tSessions.end())  // 异常情况
							{
								// 删除不存在的Socket
								tAllHandles.erase(i);
								REMOVEIFHAS(i, tWriteTestHandles);
								REMOVEIFHAS(i, tErrorTestHandles);
								m_pLogger->Log(
									"receiving data on a session which is not existed, there're may have some error on server side.", 
									LogType::Warning
								);
							}
							else
							{
								try
								{
									if (s->second->RecvData())  // 产生了数据包
									{
										// 发送一个消息
										m_MsgQueue.Push(Message(MessageType::ClientPackageArrival, s->second.get()));
									}
								}
								catch (const std::exception& e)
								{
									// 移除错误的Socket
									m_pLogger->Log(
										StringFormat("error occurred when receiving data on '%s:%u', client removed. (%s)", 
											s->second->GetIP().c_str(), 
											s->second->GetPort(), 
											e.what()), 
										LogType::Error
									);
									m_MsgQueue.Push(Message(MessageType::ClientRemoved, s->second.get()));
									tAllHandles.erase(s->first); 
									tSessions.erase(s);
									REMOVEIFHAS(i, tWriteTestHandles);
									REMOVEIFHAS(i, tErrorTestHandles);
								}
							}
						}
					}
					// 检查可以发送数据的socket
					for (auto i : tWriteTestHandles)
					{
						auto s = tSessions.find(i);
						if (s == tSessions.end())  // 异常情况
						{
							// 删除不存在的Socket
							tAllHandles.erase(i);
							REMOVEIFHAS(i, tErrorTestHandles);
							m_pLogger->Log(
								"sending data on a session which is not existed, there're may have some error on server side.",
								LogType::Warning
							);
						}
						else
						{
							try
							{
								s->second->SendData();  // 通知发送数据
							}
							catch (const std::exception& e)
							{
								// 移除错误的Socket
								m_pLogger->Log(
									StringFormat("error occurred when sending data on '%s:%u', client removed. (%s)",
										s->second->GetIP().c_str(),
										s->second->GetPort(),
										e.what()),
									LogType::Error
								);
								m_MsgQueue.Push(Message(MessageType::ClientRemoved, s->second.get()));
								tAllHandles.erase(s->first);
								tSessions.erase(s);
								REMOVEIFHAS(i, tErrorTestHandles);
							}
						}
					}
					// 检查发生错误的socket
					for (auto i : tErrorTestHandles)
					{
						auto s = tSessions.find(i);
						if (s == tSessions.end())  // 异常情况
						{
							// 删除不存在的Socket
							tAllHandles.erase(i);
							m_pLogger->Log(
								"dealing error on a session which is not existed, there're may have some error on server side.",
								LogType::Warning
								);
						}
						else
						{
							// 移除错误的Socket
							m_pLogger->Log(
								StringFormat("error occurred on '%s:%u', client removed.",
									s->second->GetIP().c_str(),
									s->second->GetPort()),
								LogType::Error
							);
							m_MsgQueue.Push(Message(MessageType::ClientRemoved, s->second.get()));
							tAllHandles.erase(s->first);
							tSessions.erase(s);
						}
					}
					tReadTestHandles.clear();
					tWriteTestHandles.clear();
					tErrorTestHandles.clear();
				}

				// 检查掉线情况
				auto i = tAllHandles.begin();
				while (i != tAllHandles.end())
				{
					if ((*i)->TestIfClosed())
					{
						auto s = tSessions.find(*i);
						if (s == tSessions.end())  // 异常情况
						{
							m_pLogger->Log(
								"dealing close message on a session which is not existed, there're may have some error on server side.",
								LogType::Warning
							);
						}
						else
						{
							// 移除Socket
							m_pLogger->Log(
								StringFormat(
									"client left on '%s:%u'.",
									s->second->GetIP().c_str(),
									s->second->GetPort()),
								LogType::Infomation
							);
							m_MsgQueue.Push(Message(MessageType::ClientRemoved, s->second.get()));
							tSessions.erase(s);
						}
						i = tAllHandles.erase(i);
					}
					else
						++i;
				}

				// 时间步计数
				auto tEndTick = chrono::system_clock::now();
				auto tTickInMs = chrono::duration_cast<chrono::microseconds>(tEndTick - tStartTick);
				tStartTick = tEndTick;
				for (auto i : tSessions)
				{
					i.second->Tick((uint32_t)tTickInMs.count());
				}
			}
			catch (const std::exception& e)
			{
				m_pLogger->Log(
					StringFormat(
						"critical error occured. (%s)",
						e.what()),
					LogType::Error
				);	
				break;
			}
		}
	}

	// 通知消息线程停止工作
	m_MsgQueue.Push(Message(MessageType::WorkStopped));

	// 等待消息线程停止工作
	m_tMsgWork->join();

	m_bRunning = false;
}

void Server::messageThreadLoop()NKNOEXCEPT
{
	while (true)
	{
		Message tMsg;
		m_MsgQueue.Pop(tMsg);
		switch (tMsg.Type)
		{
		case MessageType::WorkStopped:
			return;  // 结束工作
		case MessageType::ClientArrival:
			break;
		case MessageType::ClientRemoved:
			break;
		case MessageType::ClientPackageArrival:
			break;
		default:
			break;
		}
	}
}
