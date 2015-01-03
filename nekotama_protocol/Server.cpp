#include "Server.h"

#include <unordered_map>
#include <chrono>

using namespace std;
using namespace nekotama;

// ��С�ļ�ʱʱ�䲽
#define TIMETICKSTEP 16

////////////////////////////////////////////////////////////////////////////////

Server::Server(ISocketFactory* pFactory, ILogger* pLogger, uint16_t port, uint16_t maxClient)
	: m_pFactory(pFactory), m_pLogger(pLogger), m_Port(port), m_Clients(maxClient),
	m_stopFlag(false), m_bRunning(false)
{
	// ����Socket����
	m_Socket = m_pFactory->Create(SocketType::TCP);

	// ��ʼ��Socket
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

	// ������Ϣ�����߳�
	m_tMsgWork = make_shared<thread>(std::bind(&Server::messageThreadLoop, this));

	// ! һ��ʺ ����д
	{
		// �ͻ�����Ϣ
		unordered_map<SocketHandle, ClientSessionHandle> tSessions;
		SocketHandleSet tAllHandles;

		// ����select���Եľ��
		SocketHandleSet tReadTestHandles;
		SocketHandleSet tWriteTestHandles;
		SocketHandleSet tErrorTestHandles;

		auto tStartTick = std::chrono::system_clock::now();  // ��ʱ��
		while (!m_stopFlag)  // ִ��Socketѭ����tickѭ��
		{
			try
			{
				// ׼��Select����
				tReadTestHandles = tAllHandles;
				tWriteTestHandles = tAllHandles;
				tErrorTestHandles = tAllHandles;
				tReadTestHandles.insert(m_Socket);

				// ִ��Select
				if (m_pFactory->Select(&tReadTestHandles, &tWriteTestHandles, &tErrorTestHandles, TIMETICKSTEP))
				{
					// �����Խ������ݵ�socket
					for (auto i : tReadTestHandles)
					{
						if (i == m_Socket)  // �¿ͻ��˵���
						{
							string tIP;
							uint16_t tPort;
							SocketHandle tHandle;
							try
							{
								if (i->Accept(tIP, tPort, tHandle))
								{
									// ����һ���Ự
									ClientSessionHandle tSession = make_shared<ClientSession>(tHandle, tIP, tPort, m_pLogger);
									tAllHandles.insert(tHandle);
									tSessions.insert(pair<SocketHandle, ClientSessionHandle>(tHandle, tSession));

									// ��¼��־
									m_pLogger->Log(
										StringFormat(
											"client arrived on '%s:%u'.",
											tIP.c_str(),
											tPort
										),
										LogType::Infomation
									);

									// ����һ����Ϣ
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
						else  // ֪ͨ�Ự������Ϣ
						{	
							auto s = tSessions.find(i);
							if (s == tSessions.end())  // �쳣���
							{
								// ɾ�������ڵ�Socket
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
									if (s->second->RecvData())  // ���������ݰ�
									{
										// ����һ����Ϣ
										m_MsgQueue.Push(Message(MessageType::ClientPackageArrival, s->second.get()));
									}
								}
								catch (const std::exception& e)
								{
									// �Ƴ������Socket
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
					// �����Է������ݵ�socket
					for (auto i : tWriteTestHandles)
					{
						auto s = tSessions.find(i);
						if (s == tSessions.end())  // �쳣���
						{
							// ɾ�������ڵ�Socket
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
								s->second->SendData();  // ֪ͨ��������
							}
							catch (const std::exception& e)
							{
								// �Ƴ������Socket
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
					// ��鷢�������socket
					for (auto i : tErrorTestHandles)
					{
						auto s = tSessions.find(i);
						if (s == tSessions.end())  // �쳣���
						{
							// ɾ�������ڵ�Socket
							tAllHandles.erase(i);
							m_pLogger->Log(
								"dealing error on a session which is not existed, there're may have some error on server side.",
								LogType::Warning
								);
						}
						else
						{
							// �Ƴ������Socket
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

				// ���������
				auto i = tAllHandles.begin();
				while (i != tAllHandles.end())
				{
					if ((*i)->TestIfClosed())
					{
						auto s = tSessions.find(*i);
						if (s == tSessions.end())  // �쳣���
						{
							m_pLogger->Log(
								"dealing close message on a session which is not existed, there're may have some error on server side.",
								LogType::Warning
							);
						}
						else
						{
							// �Ƴ�Socket
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

				// ʱ�䲽����
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

	// ֪ͨ��Ϣ�߳�ֹͣ����
	m_MsgQueue.Push(Message(MessageType::WorkStopped));

	// �ȴ���Ϣ�߳�ֹͣ����
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
			return;  // ��������
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
