/**
	@file		ntv2macmutex.h
	@brief		Declares and implements the Mac-only PMutex and PThreadLocker classes, a simple
				posix-based mutex locker. Designed to be created on the stack. Allows thread to
				do multiple locks on the same mutex while avoiding race conditions.
	@copyright	(C) 2003-2016 AJA Video Systems, Inc.	Proprietary and confidential information.
	@details	Usage example:
	@code{.cpp}
		static PMutex  sMutex;
		. . .
		void MyClass::MyMethod (void)
		{
			PThreadLocker  lock (sMutex);	//	Lock release on Method exit
			//	Critical section - one thread at a time
			. . .
		}
	@endcode
**/

#ifndef _PTHREADLOCKER_
#define _PTHREADLOCKER_

#include <pthread.h>
#include <assert.h>


class PMutex 
{
	public:
		PMutex ()
			:	mOwningThread	(NULL),
				mLockCount		(0)
		{
			::pthread_mutex_init (&mMutex, NULL);
		}

		virtual	~PMutex ()
		{
			::pthread_mutex_destroy (&mMutex);
		}


	private:
		friend class PThreadLocker;

		bool Lock (void)
		{
			if (mOwningThread && ::pthread_equal (mOwningThread, ::pthread_self ()))
				mLockCount++;
			else
			{
				::pthread_mutex_lock (&mMutex);	//	Block until mutex is available, then grab it
				assert (mLockCount == 0 && "Lock count expected to be zero");	//	This better be zero!
				mLockCount = 1;
				mOwningThread = ::pthread_self ();
				assert (mOwningThread && "pthread_self returned a zero thread ID!");	//	My owning thread should be non-zero
			}
			return true;
		}

		bool Unlock (void)
		{
			if (::pthread_equal (mOwningThread, ::pthread_self ()))
			{
				assert (mLockCount && "Lock count expected to be greater than zero in Unlock()");
				mLockCount--;
				if (mLockCount == 0)
				{
					mOwningThread = NULL;
					::pthread_mutex_unlock (&mMutex);
				}
			}
			else if (mOwningThread)
			{
				assert (false && "Unlock called from non-owning thread!");
				return false;
			}
			else
			{
				assert (false && "Unlock called without prior call to Lock!");
				return false;
			}
			return true;
		}

	private:
		pthread_t			mOwningThread;
		unsigned			mLockCount;
		pthread_mutex_t		mMutex;

};	//	PMutex


class PThreadLocker
{
	public:
		PThreadLocker (PMutex * pInMutex)
			:	mpMutex	(pInMutex)
		{
			assert (mpMutex && "PThreadLocker being constructed with NULL pointer!");
			if (mpMutex)
				mpMutex->Lock ();
		}

		~PThreadLocker ()
		{
			if (mpMutex)
				mpMutex->Unlock ();
		}

	private:
		PMutex *	mpMutex;

};	//	PThreadLocker


#endif // _PTHREADLOCKER_
