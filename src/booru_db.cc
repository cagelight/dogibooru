#include "booru.hh"

static void postgres_notice (void *, char const *) {
	
}

postgres_result::postgres_result(PGresult * res) : res(res) {
	if (res) status = PQresultStatus(res);
	else status = PGRES_BAD_RESPONSE;
}

postgres_connection::postgres_connection(std::string db) {
	std::string constr = "user=postgres dbname=" + db;
	con = PQconnectdb(constr.c_str());
	if (PQstatus(con) != CONNECTION_OK) {
		srcprintf_error("failed to connect to database \"%s\" (status %i)", db.c_str(), PQstatus(con));
		return;
	}
	PQsetNoticeProcessor(con, postgres_notice, nullptr);
	ok_ = true;
}


postgres_connection::~postgres_connection() {
	if (con) PQfinish(con);
}

postgres_pool::postgres_pool(std::string const & dbname, unsigned int num_cons) {
	for (unsigned int i = 0; i < num_cons; i++) {
		cons.emplace_back(new postgres_pool_con {dbname, cvm, cv});
		if (!cons.back()->con.ok()) return;
	}
	ok_ = true;
}

postgres_pool::~postgres_pool() {
	
}

postgres_conview postgres_pool::try_acquire() {
	for (auto & pq : cons) {
		if (pq->in_use.test_and_set()) continue;
		return pq;
	}
	return {nullptr};
}

postgres_conview postgres_pool::acquire() {
	while (true) {
		for (auto & pq : cons) {
			if (pq->in_use.test_and_set()) continue;
			return pq;
		}
		std::unique_lock<std::mutex> lk(cvm);
		cv.wait_for(lk, std::chrono::milliseconds(5));
	}
}
