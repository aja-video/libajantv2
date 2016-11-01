////////////////////////////////////////////////////////////
//
//	Copyright (C) 2005 AJA Video Systems, Inc.  
//	Proprietary and Confidential information.
//
// This implements a Producer/Consumer algorithm so
// we can be filling up a circular buffer on one end
// and emptying it on the other and not have to worry
// about synchronization
////////////////////////////////////////////////////////////
#ifndef CXenaCircularBuffer_H
#define CXenaCircularBuffer_H


#define TRUE  true
#define FALSE false
#define WAIT_TIMEOUT ETIMEDOUT
#define DWORD unsigned int
#define WAIT_ABANDONED true
#define WAIT_OBJECT_0  true
#define TRACE 0
//#define TRACE 1

#include <pthread.h>
#include <vector>
#include <string>
#include <sys/time.h>
#include <errno.h>
#include <stdlib.h>

typedef std::vector<pthread_mutex_t> XenaBufferMutexList;
typedef std::vector<std::string> XenaStringList;

#define WAIT_TIMEOUT_MS  100

template<typename T>
class CXenaCircularBuffer {
public:
	CXenaCircularBuffer()
		: _head(0),
		_tail(0),
		_fillIndex(0),
		_emptyIndex(0),
		_circBufferCount(0),
		_notFullEvent(),
		_notEmptyEvent(),
		_dataBufferMutex(),
		_eventMutex(),
		_pAbortFlag(NULL)
	{
		pthread_mutex_init(&_dataBufferMutex, NULL);
		pthread_mutex_init(&_eventMutex, NULL);
		pthread_cond_init(&_notFullEvent, NULL);
		pthread_cond_init(&_notEmptyEvent, NULL);
	}

	virtual ~CXenaCircularBuffer()
	{
		pthread_mutex_destroy(&_dataBufferMutex);
		pthread_mutex_destroy(&_eventMutex);
		pthread_cond_destroy(&_notFullEvent);
		pthread_cond_destroy(&_notEmptyEvent);
		XenaBufferMutexList::iterator mutIter;
		for ( mutIter = _xenaBufferMutexList.begin(); mutIter != _xenaBufferMutexList.end(); mutIter++ )
		{
			pthread_mutex_destroy(&(*mutIter));
		}
	}
	
	void Init(int* abortFlag=NULL)
	{
		_pAbortFlag = abortFlag;
		_xenaBufferList.clear();
		_xenaBufferMutexList.clear();
		_head = 0;
		_tail = 0;
		_fillIndex = 0;
		_emptyIndex = 0;
		_circBufferCount = 0;
	}

	bool Add(T buffer)
	{
		_xenaBufferList.push_back(buffer);
		pthread_mutex_t *mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
		if ( mutex == NULL ) {
			fprintf(stderr, "%s: malloc of mutex failed\n",__FUNCTION__);
			return false;
		}
		pthread_mutex_init(mutex, NULL);
		_xenaBufferMutexList.push_back(*mutex);
		return true;
	}

	T StartProduceNextBuffer()
	{
		int waitStatus = 0;
		int rc;

		if (TRACE)
			fprintf(stderr, "%s: ct %d\n",__FUNCTION__, _circBufferCount);
		
		while (1) 
		{
			// aquire mutex
			if (TRACE)
				fprintf(stderr, "%s: trying to lock dataBuffer\n", __FUNCTION__);
			rc = pthread_mutex_lock( &_dataBufferMutex );
			if (TRACE)
				fprintf(stderr, "%s: locked dataBuffer rc(%d)\n", __FUNCTION__, rc);

			if ( _circBufferCount == _xenaBufferList.size() ) 
			{
				struct timeval  now;
				struct timespec then;
				gettimeofday(&now, NULL);
				then.tv_sec  = now.tv_sec;
				then.tv_nsec = now.tv_usec * 1000;
				then.tv_nsec += (WAIT_TIMEOUT_MS * 1000);
				
				if (TRACE)
					fprintf(stderr, "%s: waiting for free buffer\n",__FUNCTION__);
				waitStatus = pthread_cond_timedwait( &_notFullEvent, &_dataBufferMutex, &then );
				rc = pthread_mutex_unlock(&_dataBufferMutex);

				switch(waitStatus) {
				case 0:
				{
					if (TRACE)
						fprintf(stderr, "%s: _notFullEvent signaled\n",__FUNCTION__);
					break;
				}
				case ETIMEDOUT:
				{
					if (TRACE)
						fprintf(stderr, "%s: wait timed out\n",__FUNCTION__);

					if (_pAbortFlag)
					{
						if(*_pAbortFlag) 
						{
							if (TRACE)
								fprintf(stderr, "%s:  aborting cuz flag is set\n",__FUNCTION__);
							pthread_mutex_unlock(&_dataBufferMutex);
							return NULL;
						}
					}
					break;
				}
				case EINVAL:
				{
					if (TRACE)
						fprintf(stderr, "%s: wait returned EINVAL\n", __FUNCTION__);
					break;
				}
				case EPERM:
				{
					if (TRACE)
						fprintf(stderr, "%s: wait returned EPERM\n", __FUNCTION__);
					break;
				}
				default:
				{
					if (TRACE)
						fprintf(stderr, "%s: WTF? %d\n",__FUNCTION__, waitStatus);
					break;
				}
				} // switch
			}
			else
			{
				if (TRACE)
					fprintf(stderr, "%s: onward to fill the buffer\n",__FUNCTION__);
				break;
			}
		}
//		fprintf(stderr, "%s: \n", __FUNCTION__);

		if (TRACE)
			fprintf(stderr, "%s: trying to lock %d\n", __FUNCTION__, _head);
		rc = pthread_mutex_lock( &_xenaBufferMutexList[_head] );

		if (TRACE)
			fprintf(stderr, "%s: locked %d rc(%d)\n", __FUNCTION__, _head, rc);
		_fillIndex = _head;
		_head = (_head+1)%(_xenaBufferList.size());
		_circBufferCount++;
		rc = pthread_mutex_unlock(&_dataBufferMutex);
		if (TRACE)
			fprintf(stderr, "%s: unlocked dataBuffer rc(%d)\n", __FUNCTION__, rc);

		return _xenaBufferList[_fillIndex];
	}

	void EndProduceNextBuffer()
	{
		int rc;

		if (TRACE)
			fprintf(stderr, "%s: ct %d head %d fill %d\n",__FUNCTION__, _circBufferCount, _head, _fillIndex);
		rc = pthread_mutex_unlock(&_xenaBufferMutexList[_fillIndex]);
		if (TRACE)
			fprintf(stderr, "%s: unlocked %d rc(%d)\n", __FUNCTION__, _fillIndex, rc);

		// tell someone about it
		rc = pthread_cond_signal(&_notEmptyEvent);
		if (TRACE)
			fprintf(stderr, "%s: signal notEmptyEvent rc(%d)\n", __FUNCTION__, rc);
	}


	T StartConsumeNextBuffer()
	{
		int waitStatus = 0;
		int rc;
		if (TRACE)
			fprintf(stderr, "%s: ct %d\n",__FUNCTION__, _circBufferCount);

		while(1) 
		{
			// get the mutex
			if (TRACE)
				fprintf(stderr, "%s: trying to lock dataBuffer\n", __FUNCTION__);
			rc = pthread_mutex_lock( &_dataBufferMutex );
			if (TRACE)
				fprintf(stderr, "%s: locked dataBuffer rc(%d)\n", __FUNCTION__, rc);

			if ( _circBufferCount == 0 ) 
			{
				if (TRACE)
					fprintf(stderr, "%s: waiting for buffer\n",__FUNCTION__);
				// wait for the _notEmptyEvent
				struct timeval  now;
				struct timespec then;
				gettimeofday(&now, NULL);
				then.tv_sec  = now.tv_sec;
				then.tv_nsec = now.tv_usec * 1000;
				then.tv_nsec += (WAIT_TIMEOUT_MS * 1000);
				
				waitStatus = pthread_cond_timedwait( &_notEmptyEvent, &_dataBufferMutex, &then );
				rc = pthread_mutex_unlock(&_dataBufferMutex);
				
				switch(waitStatus) {
				case 0:
				{
					if (TRACE)
						fprintf(stderr, "%s: _notEmptyEvent signaled\n",__FUNCTION__);
					break;
				}
				case ETIMEDOUT:
				{
					if (TRACE)
						fprintf(stderr, "%s: wait timed out\n",__FUNCTION__);
					if (_pAbortFlag) 
					{
						if(*_pAbortFlag) 
						{
							if (TRACE)
								fprintf(stderr, "%s:  aborting cuz flag is set\n",__FUNCTION__);
							pthread_mutex_unlock(&_dataBufferMutex);
							return NULL;
						}
					}
					break;
				}
				case EINVAL:
				{
					if (TRACE)
						fprintf(stderr, "%s: wait returned EINVAL\n", __FUNCTION__);
					break;
				}
				case EPERM:
				{
					if (TRACE)
						fprintf(stderr, "%s: wait returned EPERM\n", __FUNCTION__);
					break;
				}
				default:
				{
					if (TRACE)
						fprintf(stderr, "%s: WTF? %d\n",__FUNCTION__, waitStatus);
					break;
				}
				} // switch
			}
			else
			{
				if (TRACE)
					fprintf(stderr, "%s: off to use the buffer\n", __FUNCTION__);
				break;
			}
		}

		if (TRACE)
			fprintf(stderr, "%s: trying to lock %d\n", __FUNCTION__, _tail);
		rc = pthread_mutex_lock( &_xenaBufferMutexList[_tail] );
		if (TRACE)
			fprintf(stderr, "%s: locked %d rc(%d)\n", __FUNCTION__, _tail, rc);
		_emptyIndex = _tail;
		_tail = (_tail+1)%(_xenaBufferList.size());
		_circBufferCount--;
		rc = pthread_mutex_unlock(&_dataBufferMutex);
		if (TRACE)
			fprintf(stderr, "%s: unlocked dataBuffer rc(%d)\n", __FUNCTION__, rc);

		return _xenaBufferList[_emptyIndex];
	}
	
	void EndConsumeNextBuffer()
	{	
		int rc;

		if (TRACE)
			fprintf(stderr, "%s: ct %d tail %d empty %d\n",__FUNCTION__, _circBufferCount, _tail, _emptyIndex);
		rc = pthread_mutex_unlock(&_xenaBufferMutexList[_emptyIndex]);
		if (TRACE)
			fprintf(stderr, "%s: unlocked %d rc(%d)\n", __FUNCTION__, _emptyIndex, rc);

		rc = pthread_cond_signal(&_notFullEvent);
		if (TRACE)
			fprintf(stderr, "%s: signal notFullEvent rc(%d)\n", __FUNCTION__, rc);
	}



protected:


	std::vector<T>							_xenaBufferList;
	XenaBufferMutexList						_xenaBufferMutexList;

	unsigned int							_head;
	unsigned int							_tail;
	unsigned int							_fillIndex;
	unsigned int							_emptyIndex;
	unsigned int							_circBufferCount;
	pthread_cond_t                          _notFullEvent; 
	pthread_cond_t                          _notEmptyEvent;
	pthread_mutex_t	                        _dataBufferMutex;
	pthread_mutex_t	                        _eventMutex;
	const int*								_pAbortFlag;

//	int*								_pAbortFlag;
};


#endif
