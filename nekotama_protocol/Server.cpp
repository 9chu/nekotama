#include "Server.h"

// 最小的计时时间步
#define TIMETICKSTEP 16

using namespace std;
using namespace nekotama;

////////////////////////////////////////////////////////////////////////////////

Server::Server(ISocketFactory* pFactory, ILogger* pLogger, const std::string& server_name, uint16_t maxClient, uint16_t port)
	: m_pFactory(pFactory), m_pLogger(pLogger), m_uPort(port), m_uMaxClients(maxClient), m_sServerName(server_name),
	m_stopFlag(false), m_bRunning(false)
{
	// 创建Socket对象
	m_pLogger->Log("Server初始化: 创建Socket对象...");
	m_srvSocket = m_pFactory->Create(SocketType::TCP);
	m_srvSocket->SetBlockingMode(false);

	// 初始化Socket
	m_pLogger->Log(StringFormat("Server初始化: 绑定于0.0.0.0:%u...", m_uPort));
	m_srvSocket->Bind("0.0.0.0", m_uPort);
	m_pLogger->Log("Server初始化: 启动Socket连接侦听...");
	m_srvSocket->Listen();
}

void Server::Start()
{
	if (m_bRunning)
		throw logic_error("server is running.");
	else
	{
		m_bRunning = true;
		m_threadMain = make_shared<thread>(std::bind(&Server::mainThreadLoop, this));
	}
}

void Server::Stop()
{
	if (m_bRunning)
		m_stopFlag = true;
	else
		throw logic_error("server is not running.");
}

void Server::Wait()
{
	if (m_bRunning)
		m_threadMain->join();
}

bool Server::IsRunning()const NKNOEXCEPT
{
	return m_bRunning;
}

////////////////////////////////////////////////////////////////////////////////

void Server::mainThreadLoop()NKNOEXCEPT
{
	m_stopFlag = false;
	
	SocketHandleSet tAllHandles;
	SocketHandleSet tReadHandles;
	SocketHandleSet tWriteHandles;
	SocketHandleSet tErrorHandles;
	unordered_set<ClientSessionHandle> tInvalidHandles;

	while (!m_stopFlag)
	{
		auto tLast = std::chrono::system_clock::now();

		// 统计所有的会话
		tAllHandles.clear();
		tInvalidHandles.clear();
		tWriteHandles.clear();
		for (auto i : m_mpClients)
		{
			tAllHandles.insert(i.first);
			if (i.second->HasData())
				tWriteHandles.insert(i.first);
		}	
		tReadHandles = tErrorHandles = tAllHandles;
		tReadHandles.insert(m_srvSocket);

		// 进行select
		try
		{	
			if (m_pFactory->Select(&tReadHandles, &tWriteHandles, &tErrorHandles, TIMETICKSTEP))
			{
				// 检查可读性
				for (auto i : tReadHandles)
				{
					if (i == m_srvSocket)
					{
						// 接受连接
						std::string tIP;
						uint16_t tPort;
						SocketHandle tSocket;

						if (m_srvSocket->Accept(tIP, tPort, tSocket))
						{
							// 创建Session并加入管理
							try
							{
								m_mpClients[tSocket] = ClientSessionHandle(new ClientSession(this, tSocket, tIP, tPort, m_mpClients.size()));
							}
							catch (const std::exception& e)
							{
								m_pLogger->Log(StringFormat("mainThread: 创建session时失败。(%s:%u: %s)", tIP.c_str(), tPort, e.what()), LogType::Error);

								auto i = m_mpClients.find(tSocket);
								if (i != m_mpClients.end())
									m_mpClients.erase(i);
							}
						}
					}
					else
					{
						ClientSessionHandle p = m_mpClients[i];
						if (tInvalidHandles.find(p) == tInvalidHandles.end())
						{
							try
							{
								p->recv();
							}
							catch (const std::exception& e)
							{
								m_pLogger->Log(StringFormat("session: 执行recv时发生错误，移除session。(%s:%u: %s)", p->GetIP().c_str(), p->GetPort(), e.what()), LogType::Error);
								tInvalidHandles.insert(p);
							}
						}
					}
				}
				// 通知所有可读接口
				for (auto i : tWriteHandles)
				{
					ClientSessionHandle p = m_mpClients[i];
					if (tInvalidHandles.find(p) == tInvalidHandles.end())
					{
						try
						{
							p->send();
						}
						catch (const std::exception& e)
						{
							m_pLogger->Log(StringFormat("session: 执行send时发生错误，移除session。(%s:%u: %s)", p->GetIP().c_str(), p->GetPort(), e.what()), LogType::Error);
							tInvalidHandles.insert(p);
						}
					}
				}
				// 通知所有错误接口
				for (auto i : tErrorHandles)
				{
					ClientSessionHandle p = m_mpClients[i];
					if (tInvalidHandles.find(p) == tInvalidHandles.end())
					{
						m_pLogger->Log(StringFormat("mainThread: 由于socket错误，将移除session。(%s:%u)", p->GetIP().c_str(), p->GetPort()), LogType::Error);
						tInvalidHandles.insert(p);
					}
				}
			}
		}
		catch (const std::exception& e)
		{
			m_pLogger->Log(StringFormat("mainThread: 发生了未预料的错误，终止运行。(%s)", e.what()), LogType::Error);
			m_stopFlag = true;
			break;
		}
		
		auto tCur = std::chrono::system_clock::now();
		chrono::milliseconds tTick = chrono::milliseconds::zero();
		tTick = chrono::duration_cast<chrono::milliseconds>(tCur - tLast);
		tLast = tCur;

		// 刷新计数器并检查客户端是否存活
		for (auto i : m_mpClients)
		{
			auto p = i.second;
			if (tInvalidHandles.find(p) == tInvalidHandles.end())
			{
				if (p->ShouldBeClosed())
				{
					tInvalidHandles.insert(p);
				}
				else
				{
					bool bRemoved = false;
					try
					{
						if (i.first->TestIfClosed())
						{
							m_pLogger->Log(StringFormat("mainThread: session未正常退出。(%s:%u)", p->GetIP().c_str(), p->GetPort()), LogType::Error);
							tInvalidHandles.insert(p);
							bRemoved = true;
						}
					}
					catch (const std::exception& e)
					{
						m_pLogger->Log(StringFormat("mainThread: session未正常退出(%s:%u)。(%s)", p->GetIP().c_str(), p->GetPort(), e.what()), LogType::Error);
						tInvalidHandles.insert(p);
						bRemoved = true;
					}
					if (!bRemoved)
					{
						try
						{
							p->update(tTick);
						}
						catch (const std::exception& e)
						{
							m_pLogger->Log(StringFormat("session: 执行update时发生错误，移除session。(%s:%u: %s)", i.second->GetIP().c_str(), i.second->GetPort(), e.what()), LogType::Error);
							tInvalidHandles.insert(i.second);
						}
					}
				}
			}
		}
		
		// 移除所有失效会话
		for (auto i : tInvalidHandles)
		{
			try
			{
				i->invalid();
			}
			catch (const std::exception& e)
			{
				m_pLogger->Log(StringFormat("session: 执行invalid时发生错误。(%s:%u: %s)", i->GetIP().c_str(), i->GetPort(), e.what()), LogType::Warning);
			}
			m_mpClients.erase(m_mpClients.find(i->GetSocket()));
		}
	}

	m_mpClients.clear();
	m_bRunning = false;
}
