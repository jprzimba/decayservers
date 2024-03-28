//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//////////////////////////////////////////////////////////////////////
#include "otpch.h"

#include "protocol.h"
#include "outputmessage.h"
#include "protocolgame.h"
#include "protocollogin.h"
#include "status.h"
#include "tasks.h"
#include "scheduler.h"
#include "connection.h"
#include "configmanager.h"

#include <boost/bind.hpp>

extern ConfigManager g_config;

ConnectionManager::ConnectionManager()
{
	maxLoginTries = (uint32_t)g_config.getNumber(ConfigManager::LOGIN_TRIES);
	retryTimeout = (uint32_t)g_config.getNumber(ConfigManager::RETRY_TIMEOUT) / 1000;
	loginTimeout = (uint32_t)g_config.getNumber(ConfigManager::LOGIN_TIMEOUT) / 1000;
}

Connection* ConnectionManager::createConnection(boost::asio::io_service& io_service)
{
	#ifdef __DEBUG_NET_DETAIL__
	std::clog << "Create new Connection" << std::endl;
	#endif

	boost::recursive_mutex::scoped_lock lockClass(m_connectionManagerLock);
	Connection* connection = new Connection(io_service);
	m_connections.push_back(connection);
	return connection;
}

void ConnectionManager::releaseConnection(Connection* connection)
{
	#ifdef __DEBUG_NET_DETAIL__
	std::clog << "Releasing connection" << std::endl;
	#endif

	boost::recursive_mutex::scoped_lock lockClass(m_connectionManagerLock);
	std::list<Connection*>::iterator it =
		std::find(m_connections.begin(), m_connections.end(), connection);

	if(it != m_connections.end())
		m_connections.erase(it);
	else
		std::clog << "Error: [ConnectionManager::releaseConnection] Connection not found" << std::endl;
}

void ConnectionManager::closeAll()
{
	#ifdef __DEBUG_NET_DETAIL__
	std::clog << "Closing all connections" << std::endl;
	#endif
	boost::recursive_mutex::scoped_lock lockClass(m_connectionManagerLock);
	std::list<Connection*>::iterator it = m_connections.begin();
	while(it != m_connections.end())
	{
		boost::system::error_code error;
		(*it)->m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
		(*it)->m_socket.close(error);
		++it;
	}
	m_connections.clear();
}


bool ConnectionManager::isDisabled(uint32_t clientip)
{
	if(maxLoginTries == 0)
		return false;

	if(clientip != 0)
	{
		uint32_t currentTime = time(NULL);
		IpLoginMap::const_iterator it = ipLoginMap.find(clientip);
		if(it != ipLoginMap.end())
		{
			if((it->second.numberOfLogins >= maxLoginTries) && (currentTime < it->second.lastLoginTime + loginTimeout))
				return true;
		}
	}

	return false;
}

void ConnectionManager::addLoginAttempt(uint32_t clientip, bool isSuccess)
{
	if(clientip != 0)
	{
		uint32_t currentTime = time(NULL);

		IpLoginMap::iterator it = ipLoginMap.find(clientip);
		if(it == ipLoginMap.end())
		{
			LoginBlock lb;
			lb.lastLoginTime = 0;
			lb.numberOfLogins = 0;

			ipLoginMap[clientip] = lb;
			it = ipLoginMap.find(clientip);
		}

		if(it->second.numberOfLogins >= maxLoginTries)
			it->second.numberOfLogins = 0;

		if(!isSuccess || (currentTime < it->second.lastLoginTime + retryTimeout))
			++it->second.numberOfLogins;
		else
			it->second.numberOfLogins = 0;

		it->second.lastLoginTime = currentTime;
	}
}

//*****************

void Connection::close()
{
	//any thread
	#ifdef __DEBUG_NET_DETAIL__
	std::clog << "Connection::close" << std::endl;
	#endif

	boost::recursive_mutex::scoped_lock lockClass(m_connectionLock);
	if(m_closeState != CLOSE_STATE_NONE)
		return;

	m_closeState = CLOSE_STATE_REQUESTED;

	Dispatcher::getInstance().addTask(
		createTask(boost::bind(&Connection::closeConnectionTask, this)));
}

void Connection::closeConnectionTask()
{
	//dispatcher thread
	#ifdef __DEBUG_NET_DETAIL__
	std::clog << "Connection::closeConnectionTask" << std::endl;
	#endif

	m_connectionLock.lock();
	if(m_closeState != CLOSE_STATE_REQUESTED)
	{
		std::clog << "Error: [Connection::closeConnectionTask] m_closeState = " << m_closeState << std::endl;
		m_connectionLock.unlock();
		return;
	}

	m_closeState = CLOSE_STATE_CLOSING;

	if(m_protocol)
    {
		Dispatcher::getInstance().addTask(
			createTask(boost::bind(&Protocol::releaseProtocol, m_protocol)));
		m_protocol->setConnection(NULL);
		m_protocol = NULL;
	}

	if(!closingConnection())
		m_connectionLock.unlock();
}

bool Connection::closingConnection()
{
	//any thread
	#ifdef __DEBUG_NET_DETAIL__
	std::clog << "Connection::closingConnection" << std::endl;
	#endif

	if(m_pendingWrite == 0 || m_writeError == true)
	{
		if(!m_socketClosed)
		{
			#ifdef __DEBUG_NET_DETAIL__
			std::clog << "Closing socket" << std::endl;
			#endif

			boost::system::error_code error;
			m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
			if(error)
			{
				if(error == boost::asio::error::not_connected)
				{
					//Transport endpoint is not connected.
				}
				else
					PRINT_ASIO_ERROR("Shutdown");
			}
			m_socket.close(error);
			m_socketClosed = true;
			if(error)
				PRINT_ASIO_ERROR("Close");
		}

		if(m_pendingRead == 0)
		{
			#ifdef __DEBUG_NET_DETAIL__
			std::clog << "Deleting Connection" << std::endl;
			#endif

			m_connectionLock.unlock();

			Dispatcher::getInstance().addTask(
				createTask(boost::bind(&Connection::releaseConnection, this)));

			return true;
		}
	}
	return false;
}

void Connection::releaseConnection()
{
	if(m_refCount > 0)
	{
		//Reschedule it and try again.
		Scheduler::getInstance().addEvent( createSchedulerTask(SCHEDULER_MINTICKS,
			boost::bind(&Connection::releaseConnection, this)));
	}
	else
		deleteConnectionTask();
}

void Connection::deleteConnectionTask()
{
	//dispather thread
	assert(m_refCount == 0);
	delete this;
}

void Connection::accept()
{
	// Read size of te first packet
	m_pendingRead++;
	boost::asio::async_read(m_socket,
		boost::asio::buffer(m_msg.getBuffer(), NetworkMessage::headerLength),
		boost::bind(&Connection::parseHeader, this, boost::asio::placeholders::error));
}

void Connection::parseHeader(const boost::system::error_code& error)
{
	m_connectionLock.lock();
	m_pendingRead--;
	if(m_closeState == CLOSE_STATE_CLOSING)
	{
		if(!closingConnection())
			m_connectionLock.unlock();

		return;
	}

	int32_t size = m_msg.decodeHeader();
	if(!error && size > 0 && size < NETWORKMESSAGE_MAXSIZE - 16)
	{
		// Read packet content
		m_pendingRead++;
		m_msg.setMessageLength(size + NetworkMessage::headerLength);
		boost::asio::async_read(m_socket, boost::asio::buffer(m_msg.getBodyBuffer(), size),
			boost::bind(&Connection::parsePacket, this, boost::asio::placeholders::error));
	}
	else
		handleReadError(error);

	m_connectionLock.unlock();
}

void Connection::parsePacket(const boost::system::error_code& error)
{
	m_connectionLock.lock();
	m_pendingRead--;
	if(m_closeState == CLOSE_STATE_CLOSING)
	{
		if(!closingConnection())
			m_connectionLock.unlock();

		return;
	}

	if(!error)
	{
		// Protocol selection
		if(!m_protocol)
		{
			// Protocol depends on the first byte of the packet
			uint8_t protocolId = m_msg.GetByte();
			switch(protocolId)
			{
				case 0x01: // Login server protocol
					m_protocol = new ProtocolLogin(this);
					break;
				case 0x0A: // World server protocol
					m_protocol = new ProtocolGame(this);
					break;
				case 0xFF: // Status protocol
					m_protocol = new ProtocolStatus(this);
					break;
				default:
				// No valid protocol
				close();
				m_connectionLock.unlock();
				return;
				break;
			}
			m_protocol->onRecvFirstMessage(m_msg);
		}
		else
		{
			// Send the packet to the current protocol
			m_protocol->onRecvMessage(m_msg);
		}

		// Wait to the next packet
		m_pendingRead++;
		boost::asio::async_read(m_socket,
			boost::asio::buffer(m_msg.getBuffer(), NetworkMessage::headerLength),
			boost::bind(&Connection::parseHeader, this, boost::asio::placeholders::error));
	}
	else
		handleReadError(error);

	m_connectionLock.unlock();
}

void Connection::handleReadError(const boost::system::error_code& error)
{
	#ifdef __DEBUG_NET_DETAIL__
	PRINT_ASIO_ERROR("Reading - detail");
	#endif
	if(error == boost::asio::error::operation_aborted)
	{
		//Operation aborted because connection will be closed
		//Do NOT call close() from here
	}
	else if(error == boost::asio::error::eof)
	{
		//No more to read
		close();
	}
	else if(error == boost::asio::error::connection_reset ||
			error == boost::asio::error::connection_aborted)
	{
		//Connection closed remotely
		close();
	}
	else
	{
		PRINT_ASIO_ERROR("Reading");
		close();
	}
	m_readError = true;
}

bool Connection::send(OutputMessage_ptr msg)
{
	m_connectionLock.lock();
	if(m_closeState == CLOSE_STATE_CLOSING || m_writeError)
	{
		m_connectionLock.unlock();
		return false;
	}

	msg->getProtocol()->onSendMessage(msg);
	if(m_pendingWrite == 0)
	{
      #ifdef __DEBUG_NET_DETAIL__
		std::clog << "Connection::send " << msg->getMessageLength() << std::endl;
	  #endif
		internalSend(msg);
	}
	else
	{
	  #ifdef __DEBUG_NET__
		std::clog << "Connection::send Adding to queue " << msg->getMessageLength() << std::endl;
	  #endif
		m_outputQueue.push_back(msg);
	}
	m_connectionLock.unlock();
	return true;
}

void Connection::internalSend(OutputMessage_ptr msg)
{
    m_pendingWrite++;
	boost::asio::async_write(m_socket,
		boost::asio::buffer(msg->getOutputBuffer(), msg->getMessageLength()),
		boost::bind(&Connection::onWriteOperation, this, msg, boost::asio::placeholders::error));
}

uint32_t Connection::getIP() const
{
	//Ip is expressed in network byte order
	boost::system::error_code error;
	const boost::asio::ip::tcp::endpoint endpoint = m_socket.remote_endpoint(error);
	if(!error)
		return htonl(endpoint.address().to_v4().to_ulong());
	else
	{
		PRINT_ASIO_ERROR("Getting remote ip");
		return 0;
	}
}

void Connection::onWriteOperation(OutputMessage_ptr msg, const boost::system::error_code& error)
{
  #ifdef __DEBUG_NET_DETAIL__
	std::clog << "onWriteOperation" << std::endl;
  #endif

	msg.reset();

	m_connectionLock.lock();

	if(!error)
	{
		if(m_pendingWrite > 0)
		{
			if(!m_outputQueue.empty())
			{
				OutputMessage_ptr msg = m_outputQueue.front();
				m_outputQueue.pop_front();
				internalSend(msg);
			  #ifdef __DEBUG_NET_DETAIL__
				std::clog << "Connection::onWriteOperation send " << msg->getMessageLength() << std::endl;
			  #endif
			}
			
			m_pendingWrite--;
		}
		else
		{
			std::clog << "Error: [Connection::onWriteOperation] Getting unexpected notification!" << std::endl;
			// Error. Pending operations counter is 0, but we are getting a
			// notification!!
		}
	}
	else
	{
        m_pendingWrite--;
		handleWriteError(error);
	}

	if(m_closeState == CLOSE_STATE_CLOSING)
	{
		if(!closingConnection())
			m_connectionLock.lock();
		return;
	}

	m_connectionLock.unlock();
}

void Connection::handleWriteError(const boost::system::error_code& error)
{
	#ifdef __DEBUG_NET_DETAIL__
	PRINT_ASIO_ERROR("Writing - detail");
	#endif
	if(error == boost::asio::error::operation_aborted)
	{
		//Operation aborted because connection will be closed
		//Do NOT call close() from here
	}
	else if(error == boost::asio::error::eof)
	{
		//No more to read
		close();
	}
	else if(error == boost::asio::error::connection_reset ||
			error == boost::asio::error::connection_aborted){
		//Connection closed remotely
		close();
	}
	else
	{
		PRINT_ASIO_ERROR("Writting");
		close();
	}
	m_writeError = true;
}
