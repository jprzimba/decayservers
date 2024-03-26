////////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
////////////////////////////////////////////////////////////////////////
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
////////////////////////////////////////////////////////////////////////
#include "otpch.h"
#if defined WINDOWS
#include <winerror.h>
#endif

#include "protocol.h"
#include "scheduler.h"

#include "connection.h"
#include "outputmessage.h"

#include "tools.h"
#include "rsa.h"

void Protocol::onSendMessage(OutputMessage_ptr msg)
{
	#ifdef __DEBUG_NET_DETAIL__
	std::clog << "Protocol::onSendMessage" << std::endl;
	#endif
	if(!m_rawMessages)
	{
		msg->writeMessageLength();
		if(m_encryptionEnabled)
		{
			#ifdef __DEBUG_NET_DETAIL__
			std::clog << "Protocol::onSendMessage - encrypt" << std::endl;
			#endif
			XTEA_encrypt(*msg);
		}
	}

	if(msg == m_outputBuffer)
		m_outputBuffer.reset();
}

void Protocol::onRecvMessage(NetworkMessage& msg)
{
	#ifdef __DEBUG_NET_DETAIL__
	std::clog << "Protocol::onRecvMessage" << std::endl;
	#endif
	if(m_encryptionEnabled)
	{
		#ifdef __DEBUG_NET_DETAIL__
		std::clog << "Protocol::onRecvMessage - decrypt" << std::endl;
		#endif
		XTEA_decrypt(msg);
	}

	parsePacket(msg);
}

OutputMessage_ptr Protocol::getOutputBuffer()
{
	if(m_outputBuffer)
		return m_outputBuffer;

	if(m_connection)
	{
		m_outputBuffer = OutputMessagePool::getInstance()->getOutputMessage(this);
		return m_outputBuffer;
	}

	return OutputMessage_ptr();
}

void Protocol::releaseProtocol()
{
	if(m_refCount > 0)
		Scheduler::getInstance().addEvent(createSchedulerTask(SCHEDULER_MINTICKS, boost::bind(&Protocol::releaseProtocol, this)));
	else
		deleteProtocolTask();
}

void Protocol::deleteProtocolTask()
{
	//dispather thread
	assert(!m_refCount);
	setConnection(NULL);

	delete this;
}

void Protocol::XTEA_encrypt(OutputMessage& msg)
{
	uint32_t k[4];
	k[0] = m_key[0];
	k[1] = m_key[1];
	k[2] = m_key[2];
	k[3] = m_key[3];

	int32_t messageLength = msg.getMessageLength();

	//add bytes until reach 8 multiple
	uint32_t n;
	if((messageLength % 8) != 0)
	{
		n = 8 - (messageLength % 8);
		msg.AddPaddingBytes(n);
		messageLength = messageLength + n;
	}

	int read_pos = 0;
	uint32_t* buffer = (uint32_t*)msg.getBodyBuffer();
	while(read_pos < messageLength / 4)
	{
		uint32_t v0 = buffer[read_pos], v1 = buffer[read_pos + 1];
		uint32_t delta = 0x61C88647;
		uint32_t sum = 0;

		for(int32_t i = 0; i < 32; i++)
		{
			v0 += ((v1 << 4 ^ v1 >> 5) + v1) ^ (sum + k[sum & 3]);
			sum -= delta;
			v1 += ((v0 << 4 ^ v0 >> 5) + v0) ^ (sum + k[sum>>11 & 3]);
		}
		buffer[read_pos] = v0; buffer[read_pos + 1] = v1;
		read_pos = read_pos + 2;
	}
	msg.addCryptoHeader();
}

bool Protocol::XTEA_decrypt(NetworkMessage& msg)
{
	if((msg.getMessageLength() - 2) % 8 != 0)
	{
		std::clog << "Failure: [Protocol::XTEA_decrypt]. Not valid encrypted message size" << std::endl;
		return false;
	}

	//
	uint32_t k[4];
	k[0] = m_key[0];
	k[1] = m_key[1];
	k[2] = m_key[2];
	k[3] = m_key[3];

	uint32_t* buffer = (uint32_t*)msg.getBodyBuffer();
	int read_pos = 0;
	int32_t messageLength = msg.getMessageLength();
	while(read_pos < messageLength / 4)
	{
		uint32_t v0 = buffer[read_pos], v1 = buffer[read_pos + 1];
		uint32_t delta = 0x61C88647;
		uint32_t sum = 0xC6EF3720;

		for(int32_t i = 0; i < 32; i++)
		{
			v1 -= ((v0 << 4 ^ v0 >> 5) + v0) ^ (sum + k[sum>>11 & 3]);
			sum += delta;
			v0 -= ((v1 << 4 ^ v1 >> 5) + v1) ^ (sum + k[sum & 3]);
		}
		buffer[read_pos] = v0; buffer[read_pos + 1] = v1;
		read_pos = read_pos + 2;
	}
	//

	int tmp = msg.GetU16();
	if(tmp > msg.getMessageLength() - 4)
	{
		std::clog << "Failure: [Protocol::XTEA_decrypt]. Not valid unencrypted message size" << std::endl;
		return false;
	}

	msg.setMessageLength(tmp);
	return true;
}

bool Protocol::RSA_decrypt(RSA* rsa, NetworkMessage& msg)
{
	if(msg.getMessageLength() - msg.getReadPos() != 128)
	{
		std::clog << "Warning: [Protocol::RSA_decrypt]. Not valid packet size" << std::endl;
		return false;
	}

	if(!rsa->decrypt((char*)(msg.getBuffer() + msg.getReadPos()), 128))
		return false;

	if(msg.GetByte() != 0)
	{
		std::clog << "Warning: [Protocol::RSA_decrypt]. First byte != 0" << std::endl;
		return false;
	}

	return true;
}

uint32_t Protocol::getIP() const
{
	if(getConnection())
		return getConnection()->getIP();

	return 0;
}
