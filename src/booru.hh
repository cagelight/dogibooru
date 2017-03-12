#pragma once

#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>

#include <com.hh>
#include <libpq-fe.h>

inline std::string & sql_sanitize(std::string & str) {
	std::string::iterator i = str.begin();
	while (i != str.end()) {
		if (*i == '\'') {
			i = str.insert(i, '\'');
			i++;
		}
		i++;
	}
	return str;
}

struct postgres_result {
	postgres_result() {}
	postgres_result(PGresult * res);
	postgres_result(postgres_result const &) = delete;
	postgres_result(postgres_result && other) : res(other.res), status(other.status) {
		other.res = nullptr;
		other.status = PGRES_BAD_RESPONSE;
	}
	~postgres_result() { if (res) PQclear(res); }
	
	inline int num_fields() const { return PQnfields(res); }
	inline int num_rows() const { return PQntuples(res); }
	inline std::string get_value (int row, int field) { auto c = PQgetvalue(res, row, field); return c ? c : ""; }
	inline ExecStatusType get_status() const { return status; }
	inline bool cmd_ok() const { return status == PGRES_COMMAND_OK; }
	inline bool tuples_ok() const { return status == PGRES_TUPLES_OK; }
	inline std::string get_error() const { return PQresultErrorMessage(res); }
	
	postgres_result & operator = (postgres_result const & other) = delete;
	postgres_result & operator = (postgres_result && other) {
		this->res = other.res;
		this->status = other.status;
		other.res = nullptr;
		other.status = PGRES_BAD_RESPONSE;
		return *this;
	}
private:
	PGresult * res = nullptr;
	ExecStatusType status = PGRES_BAD_RESPONSE;
};

struct postgres_connection {
	postgres_connection() = delete;
	postgres_connection(std::string db);
	postgres_connection(postgres_connection const &) = delete;
	postgres_connection(postgres_connection &&) = delete;
	~postgres_connection();
	
	postgres_result exec (std::string const & cmd) { return PQexec(con, cmd.c_str()); }
	inline bool cmd(std::string const & cmd) { postgres_result res = exec(cmd); if (res.cmd_ok()) return true; else { srcprintf(SQL_ERROR, res.get_error().c_str()); return false; } }
	inline bool ok() {return ok_;}
	
private:
	bool ok_ = false;
	PGconn * con = nullptr;
};

struct postgres_pool {
	
	struct postgres_pool_con {
		postgres_pool_con() = delete;
		postgres_pool_con(std::string dbname, std::mutex & cvm, std::condition_variable & cv) : con(dbname), in_use(false), cvm(cvm), cv(cv) {}
		~postgres_pool_con() = default;
		void reset() {
			in_use.clear();
			cv.notify_one();
		}
		postgres_connection con;
		std::atomic_flag in_use;
	private:
		std::mutex & cvm;
		std::condition_variable & cv;
	};
	
	struct postgres_conview {
		postgres_conview() = delete;
		postgres_conview(std::shared_ptr<postgres_pool_con> ptr) : ptr(ptr) {}
		~postgres_conview() {
			if (in_transaction_block) cmd("ROLLBACK");
			ptr->reset();
		}
		
		inline bool ok () { return ptr && ptr->con.ok(); }
		inline postgres_result exec (std::string const & cmd) { return ptr->con.exec(cmd); }
		inline bool cmd (std::string const & cmd) { return ptr->con.cmd(cmd); }
		inline void begin() { cmd("BEGIN"); in_transaction_block = true; }
		inline void commit() { cmd("COMMIT"); in_transaction_block = false; }
		inline void rollback() { cmd("ROLLBACK"); in_transaction_block = false; }
	private:
		std::shared_ptr<postgres_pool_con> ptr;
		bool in_transaction_block = false;
	};
	
	postgres_pool(std::string const & dbname, unsigned int num_cons = std::thread::hardware_concurrency());
	~postgres_pool();
	
	inline bool ok() { return ok_; }
	postgres_conview try_acquire(); // check ok() before using conview, ALWAYS
	postgres_conview acquire(); // blocks until available, no need to check ok()
private:
	bool ok_ = false;
	std::mutex cvm;
	std::condition_variable cv;
	std::vector<std::shared_ptr<postgres_pool_con>> cons;
};

using postgres_conview = postgres_pool::postgres_conview;
