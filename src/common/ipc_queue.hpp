#ifndef __IPC_QUEUE_HPP
#define __IPC_QUEUE_HPP

#include "status.hpp"

#include<thallium.hpp>
#include<thallium/serialization/stl/string.hpp>

#include<thallium/serialization/stl/list.hpp>
#include<list>
#include <functional>
using namespace std::placeholders;
namespace tl=thallium;
//#define __DEBUG
#include "common/debug.hpp"
#include<unordered_map>

#include<mutex>
#include<condition_variable>
namespace veloc_ipc {

typedef std::function<void (int)> completion_t;

inline void cleanup() {

}
    
template <class T> class shm_queue_t:public tl::provider<shm_queue_t<T>> {
	static const size_t MAX_SIZE = 1 << 20;

	struct container_t {
		std::mutex mutex_;
		std::condition_variable cond_;
			int status = VELOC_SUCCESS;
			std::list<T> pending, progress;
			container_t() : pending(), progress() { }

	};

	typedef typename std::list<T>::iterator list_iterator_t;
	std::unordered_map<std::string,container_t*> segment;
	container_t* data;
	std::mutex pending_mutex;
	std::condition_variable pending_cond;
	container_t *find_non_empty_pending() {
		if(!segment.empty()){
			for (auto it = segment.cbegin(); it != segment.cend(); ++it) {
				container_t *result = it->second;
				if (!result->pending.empty())
					return result;
			}}
		return NULL;
	}
	bool check_completion() {
		// this is a predicate intended to be used for condition variables only
		return data->pending.empty() && data->progress.empty();
	}
//ok
	int wait_completion(bool reset_status=true) {
		std::unique_lock<std::mutex> cond_lock(data->mutex_);
		while (!check_completion())
			data->cond_.wait(cond_lock);
		int ret = data->status;
		if (reset_status)
			data->status = VELOC_SUCCESS;
		return ret;
	}
	//ok
	void enqueue(const T &e) {
		std::cout<<"this is the command file name"<<e.name<<std::endl;
		// enqueue an element and notify the consumer
		std::cout<<"enqueue is called"<<std::endl;
		std::unique_lock<std::mutex> queue_lock(data->mutex_);
		data->pending.push_back(e);
		std::cout<<"emplaced"<<std::endl;
		queue_lock.unlock();
		//	if(wasEmpty) pending_cond.notify_one();
		//added
		std::unique_lock<std::mutex> cond_lock(pending_mutex);
		pending_cond.notify_one();
		DBG("enqueued element " << e);
	}
	//ok
	void init(std::string id)
	{
		std::cout<<"init called"<<std::endl;
		data=new container_t();
		segment.emplace(id,data);
	}


	public:
	shm_queue_t(tl::engine& e,uint16_t provider_id=1) : 
		tl::provider<shm_queue_t<T>>(e,provider_id)
	{
		std::unique_lock<std::mutex> cond_lock(pending_mutex);
		//I will define here the methods
		this-> define("enqueue",&shm_queue_t::enqueue,tl::ignore_return_value());
		this-> define("init",&shm_queue_t::init,tl::ignore_return_value());
		this-> define("wait_completion",&shm_queue_t::wait_completion);
		this-> define("get",&shm_queue_t::get);
	}
	int get(){

		return 42; 

	}
	void set_completion(container_t *q,const list_iterator_t &it,int status) {
		// delete the element from the progress queue and notify the producer
		std::unique_lock<std::mutex> queue_lock(q->mutex_);
		DBG("completed element " << *it);
		q->progress.erase(it);
		if (q->status < 0 || status < 0)
			q->status = std::min(q->status, status);
		else
			q->status = std::max(q->status, status);
		q->cond_.notify_one();
	}
	//I am dequeueuing
	completion_t dequeue_any(T &e) {
		// wait until at least one pending queue has at least one element
		container_t *first_found;
		std::unique_lock<std::mutex> cond_lock(pending_mutex);
		while ((first_found = find_non_empty_pending()) == NULL){
			pending_cond.wait(cond_lock);
		}
		// remove the head of the pending queue and move it to the progress queue
		std::unique_lock<std::mutex> queue_lock(first_found->mutex_);
		e = first_found->pending.front();
		first_found->pending.pop_front();
		first_found->progress.push_back(e);
		std::cout<<"dequeued perfect"<<std::endl;
		DBG("dequeued element from pending and put in progress" << e);

		return std::bind(&shm_queue_t<T>::set_completion, this,first_found,std::prev(first_found->progress.end()), _1);
	}
	size_t get_num_queues() {
			return segment.size();
		}
	};

};


#endif // __IPC_QUEUE_HPP
