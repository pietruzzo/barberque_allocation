/**
 *       @file  threadpool.h
 *      @brief  The Threadpool BarbequeRTRM library
 *
 * Description: This library provides thread pool support for bbque integrated
 *		applications
 *
 *     @author  Simone Libutti (slibutti), simone.libutti@polimi.it
 *
 *     Company  Politecnico di Milano
 *   Copyright  Copyright (c) 2014, Simone Libutti
 *
 * This source code is released for free distribution under the terms of the
 * GNU General Public License as published by the Free Software Foundation.
 * =====================================================================================
 */

#include <functional>
#include <vector>

/**
 * @class Semaphore
 *
 * Exploited for thread syncronization.
 */
class Semaphore {
	private:
		// Mutex and condition variable for the semaphore
		std::mutex sem_mtx;
		std::condition_variable sem_cv;
		// Countdown value for the semaphore
		int value = 0;
	public:
		Semaphore(){}
		/** Setting the semaphore value. If a thread waits on the semaphore,
		 * it will wait until this value reaches 0 */
		void set(int count){
			value = count;
		}
		/** Decreasing the semaphore value. If it reaches 0, the waiting
		 * threads are notified
		 */
		void signal(int count = 1){
			// Taking the lock
			std::unique_lock<std::mutex> lock(sem_mtx);
			// Decreasing the value
			value -= count;
			if (value != 0)
				return;
			lock.unlock();
			sem_cv.notify_all();
		}
		/** Waiting on the semaphore. If the value if greater than 0,
		 * the thread will wait until the value reaches 0
		 */
		void wait(){
			std::unique_lock<std::mutex> lock(sem_mtx);
			if (value > 0)
				sem_cv.wait(lock);
		}
};

/**
 * @class ThreadPool
 *
 * Exploited for thread syncronization.
 */
class ThreadPool {

	/* True if the computation has ended */
	bool finished = false;
	/**
	 * Maximum parallelism level for the run function. It will be updated
	 * in the setup method
	 */
	int max_parallelism = 1;

	/* Semaphores needed to sync threads start and finish */
	Semaphore sem_f, sem_s;

	/* The function each thread will run */
	std::function<void ()> f;

	/* A mutex exploited for threads sync */
	std::mutex cv_mtx;

	/* The thread pool */
	std::vector<std::thread> trd_pool;

	/**
	 * Assigning each thread to a different condition variable, I avoid
	 * to wake all the threads during the notify call. Remember that,
	 * even in the case of notify_one, all threads are woken up. Then, one
	 * thread will execute and the others will return to sleep. This way,
	 * I explictly negate resource consumption to the threads whose run
	 * is not expected.
	 */
	std::vector<std::condition_variable *> cv_pool;

public:

	/**
	 * @brief Updates the function called by each thread
	 *
	 * @param fn Function to replace, already bound
	 */
	void Configure(std::function<void ()> fn){

		/* Changing the method called by each thread */
		f = fn;

	}

	/**
	 * @brief ThreadPool setup
	 *
	 * @param max_p Maximum parallelism, e.g. the pool threads number
	 * @param fn Function to be run by each thread, already bound
	 */
	void Setup(int max_p, std::function<void ()> fn){

		/* Initializing the maximum thread number */
		max_parallelism = max_p;

		/* Resizing the structures */
		trd_pool.resize(max_parallelism);
		cv_pool.resize(max_parallelism);

		/**
		 * Setting the threads function. I assume the binding has
		 * already been performed.
		 */
		f = fn;

		/* Initialization */
		for (int i = 0; i < max_parallelism; ++i) {
			/* Initializing thread number i */
			trd_pool[i] = std::thread(&ThreadPool::Run, this, i);
			/* Initializing cvar number i */
			cv_pool[i] = new std::condition_variable();
		}
	}

	/**
	 * @brief Starts the specified number of threads
	 *
	 * @param threads_number is the number of threads to start
	 */
	void Start(int threads_number){

		int t = threads_number;

		/* Can not wake more threads than the ones I have in the pool */
		if (t > max_parallelism)
			t = max_parallelism;

		/* I expect to run and then wait for 't' threads */
		sem_s.set(t);
		sem_f.set(t);

		/* Waking up the threads */
		for (int i = 0; i < t; ++i){
			/* One less thread to wait before starting */
			sem_s.signal();
			cv_pool[i]->notify_one();
		}

		/* Waiting waking threads before starting */
		sem_f.wait();

		std::unique_lock<std::mutex> lck(cv_mtx);
		lck.unlock();

	}

	/**
	 * @brief Releasing the threads
	 */
	void Release(){

		/* The workload is finished */
		finished = true;

		/* notify all the threads to wake */
		for(auto &cv : cv_pool)
			cv->notify_one();

		/* Waiting all the threads to finish */
		for(auto &trd : trd_pool)
			trd.join();

	}

	/**
	 * @brief Life cycle of a thread
	 */
	void Run(int id){

		while (true) {

			std::unique_lock<std::mutex> lck(cv_mtx);

			/* Signal that the thread is not executing code */
			sem_f.signal();

			/* Wait to be woken up */
			cv_pool[id]->wait(lck);
			lck.unlock();

			/* If there is no more work to do */
			if(finished) {
				break;
			}

			/* Else, wait all the threads to be ready */
			sem_s.wait();

			/* Execute the workload */
			f();
		}
	}

};
