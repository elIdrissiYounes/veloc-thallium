#include "common/config.hpp"
#include "common/command.hpp"
#include "common/ipc_queue.hpp"

#include "modules/module_manager.hpp"

#include <queue>
#include <future>
#define __DEBUG
#include "common/debug.hpp"
const unsigned int MAX_PARALLELISM = 64;
typedef std::function<int (void)> ft;
int main(int argc, char *argv[]) {
	bool ec_active = true;
	if (argc < 2 || argc > 3) {
		veloc_ipc::cleanup();
		std::cout << "Usage: " << argv[0] << " <veloc_config> [--disable-ec]" << std::endl;
		return 1;
	}

	config_t cfg(argv[1]);
	if (cfg.is_sync()) {
		ERROR("configuration requests sync mode, backend is not needed");
		return 3;
	}
	if (argc == 3 && std::string(argv[2]) == "--disable-ec") {
		INFO("EC module disabled by commmand line switch");
		ec_active = false;
	}

	if (ec_active) {
	}

	veloc_ipc::cleanup();
	tl::engine myServ("tcp://127.0.0.1:1234",THALLIUM_SERVER_MODE);
	uint16_t provider_id=22;
	std::cout<<"Server before running at"<<myServ.self()<<std::endl; 
	veloc_ipc::shm_queue_t<command_t> command_queue(myServ,provider_id);
	int hah=command_queue.get();
	std::cout<<"non RPC call"<<hah<<std::endl;	


	auto backend=[&command_queue,&argc,&argv](const config_t& cfg,bool ec_active){

		int rank;
		MPI_Init(&argc, &argv);
		MPI_Comm_rank(MPI_COMM_WORLD, &rank);
		DBG("Active backend rank = " << rank);
		module_manager_t modules;
		modules.add_default_modules(cfg, MPI_COMM_WORLD, ec_active);
		std::queue<std::future<void> > work_queue;
		command_t c;
		while (true) {
			std::cout<<"I am executing inside the loop"<<std::endl;
			auto f=	command_queue.dequeue_any(c);
			work_queue.push(std::async(std::launch::async, [=, &modules] {
						f(modules.notify_command(c));
						}));
			if (work_queue.size() > MAX_PARALLELISM) {
				work_queue.front().wait();
				work_queue.pop();
			}
		}



	};
	std::thread back(backend,cfg,ec_active);
	back.detach();
	//		if (ec_active) {
	//		MPI_Finalize();
	//	}

	return 0;
}
