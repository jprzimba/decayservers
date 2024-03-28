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
#include "tasks.h"

#include "outputmessage.h"
#if defined __EXCEPTION_TRACER__
#include "exception.h"
#endif

#include "game.h"
extern Game g_game;

Dispatcher::DispatcherState Dispatcher::m_threadState = Dispatcher::STATE_TERMINATED;

Dispatcher::Dispatcher()
{
	m_taskList.clear();
	Dispatcher::m_threadState = Dispatcher::STATE_RUNNING;
	boost::thread(boost::bind(&Dispatcher::dispatcherThread, (void*)this));
}

void Dispatcher::dispatcherThread(void* p)
{
	Dispatcher* dispatcher = (Dispatcher*)p;
	#if defined __EXCEPTION_TRACER__
	ExceptionHandler dispatcherExceptionHandler;
	dispatcherExceptionHandler.InstallHandler();
	#endif
	srand((uint32_t)OTSYS_TIME());

	OutputMessagePool* outputPool = NULL;
	boost::unique_lock<boost::mutex> taskLockUnique(dispatcher->m_taskLock, boost::defer_lock);
	while(Dispatcher::m_threadState != Dispatcher::STATE_TERMINATED)
	{
		Task* task = NULL;
		// check if there are tasks waiting
		taskLockUnique.lock();
		if(dispatcher->m_taskList.empty()) //if the list is empty wait for signal
			dispatcher->m_taskSignal.wait(taskLockUnique);

		if(!dispatcher->m_taskList.empty() && Dispatcher::m_threadState != Dispatcher::STATE_TERMINATED)
		{
			// take the first task
			task = dispatcher->m_taskList.front();
			dispatcher->m_taskList.pop_front();
		}

		taskLockUnique.unlock();
		// finally execute the task...
		if(!task)
			continue;

		if(!task->hasExpired())
		{
			if((outputPool = OutputMessagePool::getInstance()))
				outputPool->startExecutionFrame();

			(*task)();
			if(outputPool)
				outputPool->sendAll();

			g_game.clearSpectatorCache();
		}

		delete task;
	}

	#if defined __EXCEPTION_TRACER__
	dispatcherExceptionHandler.RemoveHandler();
	#endif
}

void Dispatcher::addTask(Task* task, bool front/* = false*/)
{
	bool signal = false;
	m_taskLock.lock();
	if(Dispatcher::m_threadState == Dispatcher::STATE_RUNNING)
	{
		signal = m_taskList.empty();
		if(front)
			m_taskList.push_front(task);
		else
			m_taskList.push_back(task);
	}
	#ifdef __DEBUG_SCHEDULER__
	else
		std::clog << "[Error - Dispatcher::addTask] Dispatcher thread is terminated." << std::endl;
	#endif

	m_taskLock.unlock();
	// send a signal if the list was empty
	if(signal)
		m_taskSignal.notify_one();
}

void Dispatcher::flush()
{
	Task* task = NULL;
	OutputMessagePool* outputPool = OutputMessagePool::getInstance();
	while(!m_taskList.empty())
	{
		task = m_taskList.front();
		m_taskList.pop_front();

		(*task)();
		delete task;
		if(outputPool)
			outputPool->sendAll();

		g_game.clearSpectatorCache();
	}
}

void Dispatcher::stop()
{
	m_taskLock.lock();
	m_threadState = Dispatcher::STATE_CLOSING;
	m_taskLock.unlock();
}

void Dispatcher::shutdown()
{
	m_taskLock.lock();
	m_threadState = Dispatcher::STATE_TERMINATED;

	flush();
	m_taskLock.unlock();
}
