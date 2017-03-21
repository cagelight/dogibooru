// ================================================================================================================================
// --------------------------------------------------------------------------------------------------------------------------------
// ================================================================================================================================

#include "booru.hh"

#include <module.hh>
#include <module_helpers.hh>
#include <seasnake.hh>
#include <json.hh>
#include <opper.hh>
#include <router.hh>
#include <crypto.hh>
#include <algorithm>

#include <fstream>
#include <unordered_set>
#include <unordered_map>
#include <chrono>
#include <map>

#include <experimental/filesystem>
namespace std { // TODO -- this is a temporary hack until std::filesystem is in GCC
	namespace filesystem = experimental::filesystem;
}

#include <QtGui/QImage>

// ================================================================

using namespace blueshift;

// ================================================================================================================================
// --------------------------------------------------------------------------------------------------------------------------------
// ================================================================================================================================

/*
struct path_helper {
	std::vector<std::string> path_split;
	path_helper() = default;
	path_helper(std::string fullpath) {
		path_split = strops::separate(fullpath, std::string{"/"});
	}
	void set(std::string fullpath) {
		path_split = strops::separate(fullpath, std::string{"/"});
	}
	std::string const & at (size_t index) {
		if (index >= path_split.size()) return empty_str;
		return path_split[index];
	}
	std::string const & operator [] (size_t index) { return at(index); }
};
*/

// ================================================================

static constexpr char const * thumbs_root = "./t/";
static constexpr char const * images_root = "./i/";
static constexpr char const * static_root = "./s/";

postgres_pool * pqp = nullptr;

// ================================================================================================================================
// --------------------------------------------------------------------------------------------------------------------------------
// ================================================================================================================================

static int get_cookie_act (postgres_conview & pq, std::string const & act_token) {
	postgres_result lk = pq.exec( strf("SELECT user_id FROM booru.cookie WHERE cookie = '%s'", act_token.c_str()).c_str() );
	if (!lk.tuples_ok()) return 0;
	if (lk.num_fields() != 1) return 0;
	if (lk.num_rows() != 1) return 0;
	return strtoul(lk.get_value(0, 0).c_str(), nullptr, 10);
}

static bool create_thumbnail(postgres_conview & pq, QImage const & src, unsigned int sqsize, int img_id) {
	QImage rs = src.scaled(sqsize, sqsize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	std::string newname = std::to_string(img_id) + "_" + std::to_string(sqsize) + ".jpg";
	rs.save(QString::fromStdString(thumbs_root + newname), "JPEG", 90);
	return pq.cmd(strf("INSERT INTO booru.thumb (img_id, sqsize, ext) VALUES (%i, %u, '%s');", img_id, sqsize, "jpg").c_str());
}

#ifdef DOGIBOORU_DEBUG
#define isemsg(fmt, ...) { std::string err = blueshift::strf("ISE (%s, line %u): %s", __PRETTY_FUNCTION__, __LINE__, blueshift::strf(fmt, ##__VA_ARGS__).c_str()).c_str(); blueshift::print(err); return generic_error(http::status_code::internal_server_error, err); }
#else
#define isemsg(fmt, ...) { return generic_error(http::status_code::internal_server_error); }
#endif

#define pqcmd(fmt, ...) { std::string quer = blueshift::strf(fmt, ##__VA_ARGS__); if (!pq.cmd(quer)) isemsg("postgres exec failed: %s", quer.c_str()); }
#define pqtup(res, fmt, ...) { std::string quer = blueshift::strf(fmt, ##__VA_ARGS__); res = pq.exec(quer); if (!res.tuples_ok()) isemsg("postgres exec failed: %s\n%s", quer.c_str(), res.get_error().c_str()); }

// ================================================================================================================================
// --------------------------------------------------------------------------------------------------------------------------------
// ================================================================================================================================

struct dogibooru_api_image : public basic_route {
	void respond() {
		postgres_conview pq = pqp->acquire();
		
		json post;
		try {
			post = json::parse({body.begin(), body.end()});
		} catch (blueshift::general_exception const & ex) {
			return generic_error(http::status_code::bad_request);
		}
		if (!post.is_ary() || post->ary.size() == 0) return generic_error(http::status_code::bad_request);
		
		std::string in_clause = "(";
		for (size_t i = 0; i < post->ary.size(); i++) {
			json const & j = post[i];
			if (i != 0) in_clause += ",";
			in_clause += (std::string) j;
		}
		in_clause += ")";
		
		std::string quer = strf("SELECT id, name, ext, width, height, size FROM booru.img WHERE id IN %s", in_clause.c_str());
		postgres_result imgi = pq.exec(quer.c_str());
		if (!imgi.tuples_ok()) isemsg("exec failed: %s", quer.c_str());
		if (imgi.num_fields() != 6) isemsg("exec returned unexpected number of fields: %s", quer.c_str());
		
		json ret;
		for (int i = 0; i < imgi.num_rows(); i++) {
			std::string id = imgi.get_value(i, 0);
			json & cur = ret[id] = json::map();
			cur["name"] = imgi.get_value(i, 1) + "." + imgi.get_value(i, 2);
			cur["width"] = strtoul(imgi.get_value(i, 3).c_str(), nullptr, 10);
			cur["height"] = strtoul(imgi.get_value(i, 4).c_str(), nullptr, 10);
			cur["size"] = strtoul(imgi.get_value(i, 5).c_str(), nullptr, 10);
		}
		return response_query->set_body(ret.serialize(), "application/json");
	}
};

// ================================================================================================================================
// --------------------------------------------------------------------------------------------------------------------------------
// ================================================================================================================================

struct dogibooru_api_tags : public basic_route {
	void respond() {
		postgres_conview pq = pqp->acquire();
		
		json post;
		try {
			post = json::parse({body.begin(), body.end()});
		} catch (blueshift::general_exception const & ex) {
			return generic_error(http::status_code::bad_request);
		}
		if (!post.is_map()) return generic_error(http::status_code::bad_request);
		
		json & get = post["get"];
		json & set = post["set"];
		if (set && !get_cookie_act(pq, request->cookie("act"))) return generic_error(http::status_code::forbidden, "editing requires admin authentication"); 
		if (!set && !get) return generic_error(http::status_code::bad_request);
		if (get && !get.is_ary()) return generic_error(http::status_code::bad_request);
		if (set && !set.is_ary()) return generic_error(http::status_code::bad_request);
		
		if (set) {
			pq.begin();
			for (json & v : set->ary) {
				if (!v.is_map()) return generic_error(http::status_code::bad_request);
				std::string img_id = v["id"];
				sql_sanitize(img_id);
				pqcmd("DELETE FROM booru.img_tag WHERE img_id = %s", img_id.c_str());
				pqcmd("DELETE FROM booru.sub WHERE img_id = %s", img_id.c_str());
				json & itags = v["itags"];
				if (itags.is_ary()) {
					for (std::string tag : itags->ary) {
						if (!tag.size()) continue;
						sql_sanitize(tag);
						postgres_result res;
						pqtup(res, "SELECT booru.add_tag_to_img(%s, '%s')", img_id.c_str(), tag.c_str());
					}
				}
				json & subs = v["subs"];
				if (subs) {
					if (!subs.is_ary()) return generic_error(http::status_code::bad_request);
					for (json & tags : subs->ary) {
						if (!tags.is_ary()) return generic_error(http::status_code::bad_request);
						postgres_result res;
						pqtup(res, "INSERT INTO booru.sub (img_id) VALUES (%s) RETURNING id", img_id.c_str());
						std::string sub_id = res.get_value(0, 0);
						for (std::string tag : tags->ary) {
							if (!tag.size()) continue;
							sql_sanitize(tag);
							postgres_result res;
							pqtup(res, "SELECT booru.add_tag_to_sub(%s, '%s')", sub_id.c_str(), tag.c_str());
						}
					}
				}
			}
			pq.commit();
		}
		
		if (get) {
			std::string in_clause {};
			for (size_t i = 0; i < get->ary.size(); i++) {
				json const & j = get[i];
				if (i != 0) in_clause += ",";
				in_clause += (std::string) j;
			}
			
			json ret = json::map();
			postgres_result res;
			
			pqtup(res, "SELECT * FROM booru.get_imgs_tags('%s')", in_clause.c_str());
			for (int i = 0; i < res.num_rows(); i++) {
				std::string img_id = res.get_value(i, 0);
				json & cur = ret[img_id]["itags"];
				if (!cur) cur = json::ary();
				cur->ary.push_back(res.get_value(i, 1));
			}
			
			pqtup(res, "SELECT * FROM booru.get_imgs_subs_tags('%s')", in_clause.c_str());
			std::string last_img {};
			std::string last_sub {};
			int sub_i = 0;
			for (int i = 0; i < res.num_rows(); i++) {
				std::string img_id = res.get_value(i, 0);
				std::string sub_id = res.get_value(i, 1);
				if (!i || img_id != last_img) { 
					sub_i = 0;
					ret[img_id]["subs"][sub_i] = json::ary();
					last_img = img_id; 
					last_sub = sub_id;
				} else if (sub_id != last_sub) {
					ret[img_id]["subs"][++sub_i] = json::ary();
					last_sub = sub_id;
				}
				std::string tag = res.get_value(i, 2);
				ret[img_id]["subs"][sub_i]->ary.push_back(tag);
			}
			
			return response_query->set_body(ret.serialize(), "application/json");
		}
	}
};

// ================================================================================================================================
// --------------------------------------------------------------------------------------------------------------------------------
// ================================================================================================================================

struct dogibooru_api_filter : public basic_route {
	dogibooru_api_filter() {
		max_content_size = 4096; // 4 KiB
	}
	void respond() {
		postgres_conview pq = pqp->acquire();
		
		json post;
		try {
			post = json::parse({body.begin(), body.end()});
		} catch (blueshift::general_exception const & ex) {
			return generic_error(http::status_code::bad_request, ex.what());
		}
		if (!post.is_map()) return generic_error(http::status_code::bad_request);
		
		json & tags = post["tags"];
		json & omit = post["omit"];
		if (!tags && !omit) return generic_error(http::status_code::bad_request);
		if (tags && !tags.is_ary()) return generic_error(http::status_code::bad_request);
		if (omit && !omit.is_ary()) return generic_error(http::status_code::bad_request);
		
		std::string filt_in_clause {};
		std::string omit_in_clause {};
		
		if (tags) for (size_t i = 0; i < tags->ary.size(); i++) {
			if (i != 0) filt_in_clause += ",";
			filt_in_clause += tags[i].to_string();
		}
		
		if (omit) for (size_t i = 0; i < omit->ary.size(); i++) {
			if (i != 0) omit_in_clause += ",";
			omit_in_clause += omit[i].to_string();
		}
		
		postgres_result res;
		pqtup(res, "SELECT img_id FROM booru.filter_imgs_by_tags_txt('%s','%s')", filt_in_clause.c_str(), omit_in_clause.c_str());
		
		json ret = json::ary();
		for (int i = 0; i < res.num_rows(); i++) {
			ret[i] = res.get_value(i, 0);
		}
		return response_query->set_body(ret.serialize(), "application/json");
	}
};

// ================================================================================================================================
// --------------------------------------------------------------------------------------------------------------------------------
// ================================================================================================================================

struct dogibooru_api_tgroups : public basic_route {
	void respond() {
		
		json post;
		try {
			post = json::parse({body.begin(), body.end()});
		} catch (blueshift::general_exception const & ex) {
			return generic_error(http::status_code::bad_request, ex.what());
		}
		if (!post.is_map()) return generic_error(http::status_code::bad_request);
		
		postgres_conview pq = pqp->acquire();
		
		json & get = post["get"];
		json & set = post["set"];
		json & del = post["del"];
		
		if (!set && !get && !del) return generic_error(http::status_code::bad_request);
		if ((set || del) && !get_cookie_act(pq, request->cookie("act"))) return generic_error(http::status_code::forbidden, "editing requires admin authentication"); 
		if (get && !get.is_map()) return generic_error(http::status_code::bad_request);
		if (set && !set.is_map()) return generic_error(http::status_code::bad_request);
		if (del && !del.is_map()) return generic_error(http::status_code::bad_request);
		
		postgres_result res;
		
		if (del) {
			json & del_grps = del["groups"];
			json & del_tags = del["tags"];
			if (!del_grps && !del_tags) return generic_error(http::status_code::bad_request);
			if (del_grps && !del_grps.is_ary()) return generic_error(http::status_code::bad_request);
			if (del_tags && !del_tags.is_ary()) return generic_error(http::status_code::bad_request);
			if (del_tags) {
				std::string in_clause {};
				for (size_t i = 0; i < del_tags->ary.size(); i++) {
					if (i != 0) in_clause += ",";
					in_clause += "'" + del_tags->ary[i].to_string() + "'";
				}
				pqcmd("DELETE FROM booru.tag WHERE name IN (%s)", in_clause.c_str());
			}
			if (del_grps) {
				std::string in_clause {};
				for (size_t i = 0; i < del_grps->ary.size(); i++) {
					if (i != 0) in_clause += ",";
					in_clause += "'" + del_grps->ary[i].to_string() + "'";
				}
				pqcmd("DELETE FROM booru.grp WHERE name IN (%s)", in_clause.c_str());
			}
		}
		
		if (set) {
			for (auto const & pair : set->map) {
				if (!pair.second.is_ary()) return generic_error(http::status_code::bad_request);
				std::string grp = pair.first;
				pqtup(res, "SELECT * FROM booru.add_group_ret_id('%s')", grp.c_str());
				std::string grp_id = res.get_value(0, 0);
				for (std::string tag : pair.second->ary) {
					pqtup(res, "SELECT * FROM booru.add_tag_ret_id('%s')", tag.c_str());
					std::string tag_id = res.get_value(0, 0);
					pqcmd("UPDATE booru.tag SET grp_id = %s WHERE id = %s", grp_id.c_str(), tag_id.c_str());
				}
			}
		}
		
		json ret = json::map();
		
		if (get) {
			if (get["all"].to_int() == 1) {
				pqtup(res, "SELECT name FROM booru.tag WHERE grp_id IS NULL ORDER BY name ASC");
				if (res.num_rows()) {
					json & nulls = ret["groupless"] = json::ary();
					for (int i = 0; i < res.num_rows(); i++) {
						nulls->ary.push_back(res.get_value(i, 0));
					}
				}
				pqtup(res, "SELECT booru.grp.name, booru.tag.name FROM booru.tag INNER JOIN booru.grp ON booru.grp.id = booru.tag.grp_id ORDER BY booru.grp.name ASC, booru.tag.name ASC");
				if (res.num_rows()) {
					json & groups = ret["groups"] = json::map();
					for (int i = 0; i < res.num_rows(); i++) {
						std::string grp = res.get_value(i, 0);
						std::string tag = res.get_value(i, 1);
						if (!groups[grp].is_ary()) groups[grp] = json::ary();
						groups[grp]->ary.push_back(tag);
					}
				}
			} else {
				json & get_grps = get["groups"];
				json & get_tags = get["tags"];
				if ((get_grps && get_tags) || (!get_grps && !get_tags)) return generic_error(http::status_code::bad_request);
				if (get_grps && !get_grps.is_ary()) return generic_error(http::status_code::bad_request);
				if (get_tags && !get_tags.is_ary()) return generic_error(http::status_code::bad_request);
				if (get_tags) {
					
				} else {
					
				}
			}
		}
		
		return response_query->set_body(ret.serialize(), "application/json");
	}
};

// ================================================================================================================================
// --------------------------------------------------------------------------------------------------------------------------------
// ================================================================================================================================

struct dogibooru_api_tagsrel : public basic_route {
	void respond() {
		
		json post;
		try {
			post = json::parse({body.begin(), body.end()});
		} catch (blueshift::general_exception const & ex) {
			return generic_error(http::status_code::bad_request, ex.what());
		}
		if (!post.is_map()) return generic_error(http::status_code::bad_request);
		
		postgres_conview pq = pqp->acquire();
		postgres_result res;
		
		json ret = json::ary();
		
		json & tags = post["tags"];
		json & omit = post["omit"];
		
		std::string filt_in_clause {};
		std::string omit_in_clause {};
		
		if (tags) for (size_t i = 0; i < tags->ary.size(); i++) {
			if (i != 0) filt_in_clause += ",";
			filt_in_clause += tags[i].to_string();
		}
		
		if (omit) for (size_t i = 0; i < omit->ary.size(); i++) {
			if (i != 0) omit_in_clause += ",";
			omit_in_clause += omit[i].to_string();
		}
		
		pqtup(res, "WITH tagsrel AS (SELECT * FROM booru.gets_tagsrel_from_tags('%s','%s')) SELECT booru.tag.name, tagsrel.count FROM booru.tag INNER JOIN tagsrel ON tagsrel.tag_id = booru.tag.id ORDER BY tagsrel.count DESC, booru.tag.name ASC", filt_in_clause.c_str(), omit_in_clause.c_str());
		
		for (int i = 0; i < res.num_rows(); i++) {
			json e = json::map();
			e["name"] = res.get_value(i, 0);
			e["count"] = json::nui(res.get_value(i, 1));
			ret->ary.push_back(e);
		}
		return response_query->set_body(ret.serialize(), "application/json");
	}
};

// ================================================================================================================================
// --------------------------------------------------------------------------------------------------------------------------------
// ================================================================================================================================

struct dogibooru_api_impls : public basic_route {
	void respond() {
		
		json post;
		try {
			post = json::parse({body.begin(), body.end()});
		} catch (blueshift::general_exception const & ex) {
			return generic_error(http::status_code::bad_request, ex.what());
		}
		if (!post.is_map()) return generic_error(http::status_code::bad_request);
		
		postgres_conview pq = pqp->acquire();
		postgres_result res;
		
		json & get = post["get"];
		json & set = post["set"];
		json & del = post["del"];
		
		if (!set && !get && !del) return generic_error(http::status_code::bad_request);
		if ((set || del) && !get_cookie_act(pq, request->cookie("act"))) return generic_error(http::status_code::forbidden, "editing requires admin authentication"); 
		if (get && !get.is_ary()) return generic_error(http::status_code::bad_request);
		if (set && !set.is_ary()) return generic_error(http::status_code::bad_request);
		if (del && !del.is_ary()) return generic_error(http::status_code::bad_request);
		
		if (del) {
			pq.begin();
			for (json & set_set : del->ary) {
				std::string tag = set_set["tag"];
				std::string imp = set_set["imp"];
				if (!tag.size() || !imp.size()) return generic_error(http::status_code::bad_request);
				pqcmd("WITH tagid AS ( SELECT booru.tag.id AS tag_id FROM booru.tag WHERE name = '%s' ), impid AS ( SELECT booru.tag.id AS implies_id FROM booru.tag WHERE name = '%s' ) DELETE FROM booru.impl WHERE tag_id IN (SELECT * FROM tagid) AND implies_id IN (SELECT * FROM impid)", tag.c_str(), imp.c_str());
			}
			pq.commit();
		}
		
		if (set) {
			pq.begin();
			for (json & set_set : set->ary) {
				std::string tag = set_set["tag"];
				std::string imp = set_set["imp"];
				if (!tag.size() || !imp.size()) return generic_error(http::status_code::bad_request);
				pqcmd("WITH tagid AS ( SELECT booru.tag.id AS tag_id FROM booru.tag WHERE name = '%s' ), impid AS ( SELECT booru.tag.id AS implies_id FROM booru.tag WHERE name = '%s' ) INSERT INTO booru.impl SELECT tagid.tag_id, impid.implies_id FROM tagid, impid", tag.c_str(), imp.c_str());
			}
			pq.commit();
		}
		
		if (get) {
			
			json ret = json::map();
			
			std::unordered_map<std::string, std::string> ids {};
			std::unordered_map<std::string, json> treecur {};
			
			if (get->ary.size()) {
				std::string in_clause;
				for (size_t i = 0; i < get->ary.size(); i++) {
					if (i != 0) in_clause += ",";
					in_clause += strf("'%s'", get[i].to_string().c_str());
				}
				pqtup(res, "SELECT id, name FROM booru.tag WHERE name IN (%s) ORDER BY name ASC", in_clause.c_str());
			} else {
				pqtup(res, "SELECT id, name FROM booru.tag ORDER BY name ASC");
			}
			
			for (int i = 0; i < res.num_rows(); i++) {
				
				std::string tagname = res.get_value(i, 1);
				json & cur = ret[tagname] = json::map();
				treecur[tagname] = cur;
				ids[tagname] = res.get_value(i, 0);
			}
			
			while (1) {
				std::unordered_map<std::string, std::string> ids_old {ids};
				std::unordered_map<std::string, json> treecur_old {treecur};
				ids.clear();
				treecur.clear();
				for (auto const & kvp : treecur_old) {
					std::string const & tag_id = ids_old[kvp.first];
					pqtup(res, "SELECT tag_id, booru.tag.name FROM booru.impl INNER JOIN booru.tag ON booru.tag.id = tag_id WHERE implies_id = %s ORDER BY booru.tag.name ASC", tag_id.c_str());
					if (!res.num_rows()) continue;
					
					for (int i = 0; i < res.num_rows(); i++) {
						std::string tagname = res.get_value(i, 1);
						ret->map.erase(tagname);
						json & cur = treecur_old[kvp.first];
						json & curm = cur[tagname] = json::map();
						treecur[tagname] = curm;
						ids[tagname] = res.get_value(i, 0);
					}
				}
				if (!treecur.size()) break;
			}
			return response_query->set_body(ret.serialize(), "application/json");
		}
	}
};

// ================================================================================================================================
// --------------------------------------------------------------------------------------------------------------------------------
// ================================================================================================================================

struct dogibooru_admin_login : public basic_route {
	void respond() {
		postgres_conview pq = pqp->acquire();
		if (request->content_type() != "application/x-www-form-urlencoded") return generic_error(http::status_code::unsupported_media_type);
		auto argm = module::form_urlencoded_decode({body.begin(), body.end()});
		std::string quer = strf("SELECT id, passhash, salt FROM booru.admin WHERE username = '%s'", argm["username"].c_str());
		postgres_result res = pq.exec(quer.c_str());
		if (!res.tuples_ok()) isemsg("SQL query did not return tuples");
		if (res.num_fields() != 3) isemsg("SQL query tuples have unexpected number of fields");
		if (res.num_rows() < 1) {
			response_query->set_body("<h1>ADMIN ACCOUNT NOT FOUND</h1>", "text/html");
			return;
		}
		std::string uid = res.get_value(0, 0);
		std::string passhash = res.get_value(0, 1);
		std::string pass = res.get_value(0, 2) + argm["password"];
		std::string passhash_test = crypto::hash_skein(pass).hex();
		if (passhash != passhash_test) {
			response_query->set_body("<h1>INCORRECT PASSWORD</h1>", "text/html");
			return;
		}
		std::string cookie_str = crypto::random(32).hex();
		quer = strf("INSERT INTO booru.cookie (cookie, user_id, expires) VALUES ('%s', %s, %s)", cookie_str.c_str(), uid.c_str(), "NOW() + interval'86400 seconds'");
		if (!pq.cmd(quer.c_str())) isemsg("SQL command failed");
		http::cookie cook {"act", cookie_str};
		cook.max_age = 86400;
		cook.secure = true;
		cook.httponly = true;
		cook.domain = "dogi.us";
		cook.path = "/";
		response->cookies.insert(cook);
		response_query->set_body("<h1>LOGIN SUCCESSFUL</h1>", "text/html");
	}
};

// ================================================================================================================================
// --------------------------------------------------------------------------------------------------------------------------------
// ================================================================================================================================

struct dogibooru_image_upload : public basic_route {
	void respond() {
		postgres_conview pq = pqp->acquire();
		if (!request->is_multipart) return generic_error(http::status_code::unsupported_media_type);
		if (!get_cookie_act(pq, request->cookie("act"))) return generic_error(http::status_code::forbidden); 
		auto file = std::find_if(multiparts.begin(), multiparts.end(), [](auto const & i)->bool{return i.header.name == "image";});
		if (file == multiparts.end() || file->header.filename == empty_str) return generic_error(http::status_code::bad_request);
		QImage img;
		if (!img.loadFromData((unsigned char const *)file->body.data(), file->body.size())) return generic_error(http::status_code::bad_request, "Uploaded image could not be parsed as an image.");
		std::filesystem::path imgname {file->header.filename};
		std::string ext = imgname.extension().generic_u8string();
		ext = ext.substr(1);
		std::string name = crypto::random(16).hexlow();
		std::string origstem = imgname.stem().generic_u8string();
		std::string quer = strf("INSERT INTO booru.img (name, ext, origname, width, height, size) VALUES ('%s', '%s', '%s', %u, %u, %zu) RETURNING id", name.c_str(), ext.c_str(), origstem.c_str(), img.width(), img.height(), file->body.size());
		postgres_result insres = pq.exec(quer.c_str());
		if (!insres.tuples_ok()) isemsg("Could not insert image into database: %s", quer.c_str());
		int img_id = strtoul(insres.get_value(0, 0).c_str(), nullptr, 10);
		create_thumbnail(pq, img, 200, img_id);
		std::ofstream si {images_root + name + "." + ext, std::ios::binary};
		si.write(file->body.data(), file->body.size());
		response->code = http::status_code::see_other;
		response->fields["Location"] = "/edit#" + std::to_string(img_id);
	}
};

// ================================================================================================================================
// --------------------------------------------------------------------------------------------------------------------------------
// ================================================================================================================================

inline static void pqinitcmd(postgres_conview & pq, char const * cmd) {
	if (!pq.cmd(cmd)) srcthrow("PSQL command failed: %s", cmd);
}

static module::import const * imp;
static router * rt = nullptr;

static std::chrono::high_resolution_clock::time_point last_pulse = std::chrono::high_resolution_clock::now();
static void dogibooru_pulse() {
	std::chrono::high_resolution_clock::time_point this_pulse = std::chrono::high_resolution_clock::now();
	if (this_pulse - last_pulse < std::chrono::milliseconds(5000)) return;
	last_pulse = this_pulse;
	postgres_conview pq = pqp->acquire();
	pq.cmd("DELETE FROM booru.cookie WHERE expires < NOW()");
	pq.cmd("DELETE FROM booru.tag WHERE id NOT IN (SELECT tag_id FROM booru.img_tag) AND id NOT IN (SELECT tag_id FROM booru.sub_tag)");
	pq.cmd("DELETE FROM booru.grp WHERE id NOT IN (SELECT grp_id FROM booru.tag WHERE grp_id IS NOT NULL)");
}

BLUESHIFT_MODULE_INIT_FUNC {
	
	::imp = imp;
	rt = new router {};
	
	rt->pulsefuncs.push_back(dogibooru_pulse);
	
	pqp = new postgres_pool("dogi");
	if (!pqp->ok()) {
		srcthrow("could not establish database connection");
	}
	
	postgres_conview pq = pqp->acquire();
	
	// ================================================================
	// TABLES
	
	pqinitcmd(pq, "CREATE SCHEMA IF NOT EXISTS booru");
	// IMAGES
	pqinitcmd(pq, R"(
		CREATE TABLE IF NOT EXISTS booru.img (
			id BIGSERIAL PRIMARY KEY,
			name CHARACTER(32) NOT NULL UNIQUE,
			ext VARCHAR(4) NOT NULL,
			origname TEXT,
			width INT NOT NULL CHECK ( width > 0 ),
			height INT NOT NULL CHECK ( height > 0 ),
			size BIGINT NOT NULL CHECK ( size > 0 )
		)
	)");
	// TAG GROUPS
	pqinitcmd(pq, R"(
		CREATE TABLE IF NOT EXISTS booru.grp ( 
			id BIGSERIAL PRIMARY KEY,
			name VARCHAR(32) NOT NULL UNIQUE
		)
	)");
	// TAGS
	pqinitcmd(pq, R"(
		CREATE TABLE IF NOT EXISTS booru.tag (
			id BIGSERIAL PRIMARY KEY,
			name VARCHAR(32) NOT NULL UNIQUE,
			grp_id BIGINT REFERENCES booru.grp ON DELETE SET NULL ON UPDATE CASCADE,
			description TEXT
		)
	)");
	// SUBJECTS
	pqinitcmd(pq, R"(
		CREATE TABLE IF NOT EXISTS booru.sub (
			id BIGSERIAL PRIMARY KEY,
			img_id BIGINT NOT NULL REFERENCES booru.img ON DELETE CASCADE ON UPDATE CASCADE
		)
	)");
	// IMAGE TAGS
	pqinitcmd(pq, R"(
		CREATE TABLE IF NOT EXISTS booru.img_tag (
			img_id BIGINT NOT NULL REFERENCES booru.img ON DELETE CASCADE ON UPDATE CASCADE,
			tag_id BIGINT NOT NULL REFERENCES booru.tag ON DELETE CASCADE ON UPDATE CASCADE,
			PRIMARY KEY ( img_id, tag_id )
		)
	)");
	// SUBJECT TAGS
	pqinitcmd(pq, R"(
		CREATE TABLE IF NOT EXISTS booru.sub_tag (
			sub_id BIGINT NOT NULL REFERENCES booru.sub ON DELETE CASCADE ON UPDATE CASCADE,
			tag_id BIGINT NOT NULL REFERENCES booru.tag ON DELETE CASCADE ON UPDATE CASCADE,
			PRIMARY KEY ( sub_id, tag_id )
		)
	)");
	// ADMINS
	pqinitcmd(pq, R"(
		CREATE TABLE IF NOT EXISTS booru.admin (
			id BIGSERIAL PRIMARY KEY,
			username VARCHAR(24) NOT NULL UNIQUE,
			passhash CHAR(128) NOT NULL,
			salt CHAR(16) NOT NULL
		)
	)");
	// LOGIN COOKIES
	pqinitcmd(pq, R"(
		CREATE TABLE IF NOT EXISTS booru.cookie (
			cookie CHAR(64) PRIMARY KEY,
			user_id BIGINT NOT NULL REFERENCES booru.admin ON DELETE CASCADE ON UPDATE CASCADE,
			expires TIMESTAMP NOT NULL
		)
	)");
	pqinitcmd(pq, R"(
		CREATE TABLE IF NOT EXISTS booru.thumb (
			img_id BIGINT NOT NULL REFERENCES booru.img ON DELETE CASCADE ON UPDATE CASCADE,
			sqsize SMALLINT NOT NULL,
			ext VARCHAR(4) NOT NULL,
			PRIMARY KEY ( img_id, sqsize ) 
		)
	)");
	pqinitcmd(pq, R"(
		CREATE TABLE IF NOT EXISTS booru.impl (
			tag_id BIGINT NOT NULL REFERENCES booru.tag ON DELETE CASCADE ON UPDATE CASCADE,
			implies_id BIGINT NOT NULL REFERENCES booru.tag ON DELETE CASCADE ON UPDATE CASCADE,
			PRIMARY KEY ( tag_id, implies_id ) 
		)
	)");
	
	// ================================================================
	// INDEXES
	
	pqinitcmd(pq, R"(
		CREATE INDEX IF NOT EXISTS tag_grp_idx ON booru.tag (grp_id) WHERE grp_id IS NOT NULL
	)");
	pqinitcmd(pq, R"(
		CREATE INDEX IF NOT EXISTS sub_img_idx ON booru.sub (img_id)
	)");
	pqinitcmd(pq, R"(
		CREATE INDEX IF NOT EXISTS img_tag_img_idx ON booru.img_tag (img_id)
	)");
	pqinitcmd(pq, R"(
		CREATE INDEX IF NOT EXISTS img_tag_tag_idx ON booru.img_tag (tag_id)
	)");
	pqinitcmd(pq, R"(
		CREATE INDEX IF NOT EXISTS sub_tag_sub_idx ON booru.sub_tag (sub_id)
	)");
	pqinitcmd(pq, R"(
		CREATE INDEX IF NOT EXISTS sub_tag_tag_idx ON booru.sub_tag (tag_id)
	)");
	pqinitcmd(pq, R"(
		CREATE INDEX IF NOT EXISTS cookie_expires_idx ON booru.cookie (expires)
	)");
	pqinitcmd(pq, R"(
		CREATE INDEX IF NOT EXISTS thumb_img_id_idx ON booru.thumb (img_id)
	)");
	pqinitcmd(pq, R"(
		CREATE INDEX IF NOT EXISTS thumb_sqsize_idx ON booru.thumb (sqsize)
	)");
	
	// ================================================================
	// PROCEDURES
	
	pqinitcmd(pq, R"(
		CREATE OR REPLACE FUNCTION booru.add_tag_ret_id(tag_name VARCHAR(32)) RETURNS BIGINT AS $$
		DECLARE
			tag_id BIGINT;
		BEGIN
			INSERT INTO booru.tag (name) VALUES (tag_name) 
				ON CONFLICT DO NOTHING 
				RETURNING id INTO tag_id;
			IF NOT FOUND THEN 
				SELECT id INTO tag_id 
				FROM booru.tag
				WHERE name = tag_name;
			END IF;
			RETURN tag_id;
		END;
		$$ LANGUAGE plpgsql;
	)");
	
	pqinitcmd(pq, R"(
		CREATE OR REPLACE FUNCTION booru.add_group_ret_id(group_name VARCHAR(32)) RETURNS BIGINT AS $$
		DECLARE
			group_id BIGINT;
		BEGIN
			INSERT INTO booru.grp (name) VALUES (group_name) 
				ON CONFLICT DO NOTHING 
				RETURNING id INTO group_id;
			IF NOT FOUND THEN 
				SELECT id INTO group_id 
				FROM booru.grp
				WHERE name = group_name;
			END IF;
			RETURN group_id;
		END;
		$$ LANGUAGE plpgsql;
	)");
	
	pqinitcmd(pq, R"(
		CREATE OR REPLACE FUNCTION booru.add_tag_to_img(img_id BIGINT, tag_name VARCHAR(32)) RETURNS void AS $$
		DECLARE
			tag_id BIGINT := booru.add_tag_ret_id(tag_name);
		BEGIN
			WITH tag_ids AS ( SELECT * FROM booru.get_tag_implications(tag_id) )
			INSERT INTO booru.img_tag (img_id, tag_id) SELECT img_id, tag_ids.tag_id FROM tag_ids ON CONFLICT DO NOTHING;
		END;
		$$ LANGUAGE plpgsql;
	)");
	
	pqinitcmd(pq, R"(
		CREATE OR REPLACE FUNCTION booru.add_tag_to_sub(sub_id BIGINT, tag_name VARCHAR(32)) RETURNS void AS $$
		DECLARE
			tag_id BIGINT := booru.add_tag_ret_id(tag_name);
		BEGIN
			WITH tag_ids AS ( SELECT * FROM booru.get_tag_implications(tag_id) )
			INSERT INTO booru.sub_tag (sub_id, tag_id) SELECT sub_id, tag_ids.tag_id FROM tag_ids ON CONFLICT DO NOTHING;
		END;
		$$ LANGUAGE plpgsql;
	)");
	
	pqinitcmd(pq, R"(
		CREATE OR REPLACE FUNCTION booru.get_imgs_tags(id_ary_txt TEXT) RETURNS TABLE (img_id BIGINT, tag_name VARCHAR(32)) AS $$
		DECLARE 
			id_ary BIGINT[] = string_to_array(id_ary_txt,',');
		BEGIN
			RETURN QUERY
			SELECT booru.img_tag.img_id, booru.tag.name 
			FROM booru.img_tag
			INNER JOIN booru.tag
				ON booru.img_tag.tag_id = booru.tag.id 
			WHERE booru.img_tag.img_id = ANY (id_ary)
			ORDER BY booru.img_tag.img_id ASC, booru.tag.name ASC;
		END;
		$$ LANGUAGE plpgsql;
	)");
	
	pqinitcmd(pq, R"(
		CREATE OR REPLACE FUNCTION booru.get_imgs_subs_tags(id_ary_txt TEXT) RETURNS TABLE (img_id BIGINT, sub_id BIGINT, tag_name VARCHAR(32)) AS $$
		DECLARE 
			id_ary BIGINT[] = string_to_array(id_ary_txt,',');
		BEGIN
			RETURN QUERY
			WITH stags AS (
				SELECT booru.sub.img_id, booru.tag.name, booru.sub_tag.sub_id
				FROM booru.sub_tag
				INNER JOIN booru.sub
					ON booru.sub.id = booru.sub_tag.sub_id 
				INNER JOIN booru.tag
					ON booru.tag.id = booru.sub_tag.tag_id
			) 
			SELECT stags.img_id, stags.sub_id, stags.name 
			FROM stags 
			WHERE stags.img_id = ANY (id_ary) 
			ORDER BY stags.img_id ASC, stags.sub_id ASC, stags.name ASC;
		END;
		$$ LANGUAGE plpgsql;
	)");
	
	pqinitcmd(pq, R"(
		CREATE OR REPLACE FUNCTION booru.filter_imgs_by_tags(filter_ary VARCHAR(32)[], omit_ary VARCHAR(32)[]) RETURNS TABLE (img_id BIGINT, sub_id BIGINT) AS $$
		BEGIN
			RETURN QUERY
			WITH filterids AS (
				SELECT DISTINCT id 
				FROM booru.tag
				WHERE name = ANY(filter_ary)
			), omitids AS (
				SELECT id 
				FROM booru.tag
				WHERE name = ANY(omit_ary)
			), tagtotal AS (
				SELECT booru.sub.img_id, booru.sub_tag.sub_id, booru.sub_tag.tag_id
				FROM booru.sub_tag
				INNER JOIN booru.sub ON booru.sub.id = booru.sub_tag.sub_id
				UNION
				SELECT booru.sub.img_id, booru.sub.id AS sub_id, booru.img_tag.tag_id
				FROM booru.sub
				INNER JOIN booru.img_tag ON booru.img_tag.img_id = booru.sub.img_id
			), filtermatches AS (
				SELECT DISTINCT tagtotal.img_id, tagtotal.sub_id
				FROM tagtotal 
				WHERE NOT EXISTS(SELECT * FROM filterids) OR (tagtotal.tag_id IN (SELECT * FROM filterids))
				GROUP BY tagtotal.img_id, tagtotal.sub_id 
				HAVING NOT EXISTS(SELECT * FROM filterids) OR (COUNT(tagtotal.tag_id) = (SELECT COUNT(*) FROM filterids))
			), omitmatches AS (
				SELECT DISTINCT tagtotal.img_id, tagtotal.sub_id
				FROM tagtotal
				WHERE tagtotal.tag_id IN (SELECT * FROM omitids)
				GROUP BY tagtotal.img_id, tagtotal.sub_id
			)
			SELECT DISTINCT filtermatches.img_id, filtermatches.sub_id
			FROM filtermatches 
			WHERE (filtermatches.img_id, filtermatches.sub_id) NOT IN (
				SELECT omitmatches.img_id, omitmatches.sub_id 
				FROM omitmatches
			)
			ORDER BY filtermatches.img_id DESC;
		END;
		$$ LANGUAGE plpgsql;
	)");
	
	pqinitcmd(pq, R"(
		CREATE OR REPLACE FUNCTION booru.filter_imgs_by_tags_txt(filter TEXT, omit TEXT) RETURNS TABLE (img_id BIGINT, sub_id BIGINT) AS $$
		DECLARE 
			filter_ary VARCHAR(32)[] = string_to_array(filter,',');
			omit_ary VARCHAR(32)[] = string_to_array(omit,',');
		BEGIN
			RETURN QUERY SELECT * FROM booru.filter_imgs_by_tags(filter_ary, omit_ary);
		END;
		$$ LANGUAGE plpgsql;
	)");
	
	pqinitcmd(pq, R"(
		CREATE OR REPLACE FUNCTION booru.gets_tagsrel_from_tags(filter TEXT, omit TEXT) RETURNS TABLE (tag_id BIGINT, count BIGINT) AS $$
		DECLARE 
			filter_ary VARCHAR(32)[] = string_to_array(filter,',');
			omit_ary VARCHAR(32)[] = string_to_array(omit,',');
		BEGIN
			RETURN QUERY
			WITH filterids AS (
				SELECT DISTINCT id 
				FROM booru.tag
				WHERE name = ANY(filter_ary)
			), omitids AS (
				SELECT id 
				FROM booru.tag
				WHERE name = ANY(omit_ary)
			), relstuff AS (
				SELECT * FROM booru.filter_imgs_by_tags(filter_ary, omit_ary)
			), tcounts AS (
				SELECT booru.img_tag.tag_id AS tag_id FROM booru.img_tag WHERE booru.img_tag.img_id IN (SELECT relstuff.img_id FROM relstuff)
				UNION ALL
				SELECT booru.sub_tag.tag_id AS tag_id FROM booru.sub_tag WHERE booru.sub_tag.sub_id IN (SELECT relstuff.sub_id FROM relstuff)
			)
			SELECT tcounts.tag_id, COUNT(tcounts.tag_id)
			FROM tcounts
			WHERE tcounts.tag_id NOT IN (SELECT * FROM filterids) AND tcounts.tag_id NOT IN (SELECT * FROM omitids)
			GROUP BY tcounts.tag_id;
		END;
		$$ LANGUAGE plpgsql;
	)");
	
	pqinitcmd(pq, R"(
		CREATE OR REPLACE FUNCTION booru.get_tag_implications(tag_id_root BIGINT) RETURNS TABLE (tag_id BIGINT) AS $$
		BEGIN
			RETURN QUERY
			WITH RECURSIVE impl_res(tag_impl) AS (
				SELECT booru.impl.tag_id
				FROM booru.impl
				WHERE booru.impl.tag_id = tag_id_root
				UNION SELECT booru.impl.implies_id
				FROM booru.impl, impl_res
				WHERE booru.impl.tag_id = impl_res.tag_impl
			)
			SELECT tag_impl FROM impl_res;
			IF NOT FOUND THEN
				RETURN QUERY SELECT booru.tag.id FROM booru.tag WHERE booru.tag.id = tag_id_root;
			END IF;
		END;
		$$ LANGUAGE plpgsql;
	)");
	
	pqinitcmd(pq, R"(
		CREATE OR REPLACE FUNCTION booru.reval_img(img_id_in BIGINT) RETURNS void AS $$
		DECLARE
			tag_id BIGINT;
			sub_id BIGINT;
		BEGIN
			FOR tag_id IN SELECT booru.img_tag.tag_id FROM booru.img_tag WHERE booru.img_tag.img_id = img_id_in
			LOOP
				WITH impl_ids AS ( SELECT * FROM booru.get_tag_implications(tag_id) )
				INSERT INTO booru.img_tag (img_id, tag_id) SELECT img_id_in, impl_ids.tag_id FROM impl_ids ON CONFLICT DO NOTHING;
			END LOOP;
			FOR sub_id, tag_id IN SELECT booru.sub_tag.sub_id, booru.sub_tag.tag_id FROM booru.sub_tag WHERE booru.sub_tag.sub_id IN (SELECT booru.sub.id FROM booru.sub WHERE booru.sub.img_id = img_id_in)
			LOOP
				WITH impl_ids AS ( SELECT * FROM booru.get_tag_implications(tag_id) )
				INSERT INTO booru.sub_tag (sub_id, tag_id) SELECT sub_id, impl_ids.tag_id FROM impl_ids ON CONFLICT DO NOTHING;
			END LOOP;
		END;
		$$ LANGUAGE plpgsql;
	)");
	
	pqinitcmd(pq, R"(
		CREATE OR REPLACE FUNCTION booru.reval_all() RETURNS void AS $$
		DECLARE
			img_id BIGINT;
		BEGIN
			FOR img_id IN SELECT booru.img.id FROM booru.img
			LOOP
				PERFORM booru.reval_img(img_id);
			END LOOP;
		END;
		$$ LANGUAGE plpgsql;
	)");
	
	// ================================================================
	
	std::filesystem::create_directory( std::filesystem::path {static_root} );
	std::filesystem::create_directory( std::filesystem::path {images_root} );
	std::filesystem::create_directory( std::filesystem::path {thumbs_root} );
	
	auto api = rt->route_route("api");
	api->route_exact<dogibooru_api_image>("img", {"POST"});
	api->route_exact<dogibooru_api_tags>("tags", {"POST"});
	api->route_exact<dogibooru_api_filter>("filter", {"POST"});
	api->route_exact<dogibooru_api_tgroups>("tgroups", {"POST"});
	api->route_exact<dogibooru_api_tagsrel>("tagsrel", {"POST"});
	api->route_exact<dogibooru_api_impls>("impls", {"POST"});
	
	rt->route_exact<dogibooru_admin_login>("dologin", {"POST"});
	rt->route_exact<dogibooru_image_upload>("doupload", {"POST"});
	
	rt->route_serve("/", static_root);
	rt->route_serve("/i/", images_root);
	rt->route_serve("/t/", thumbs_root);
	
	imp->start_server(2080, rt);
}

BLUESHIFT_MODULE_TERM_FUNC {
	if (rt) delete rt;
	delete pqp;
}

// ================================================================================================================================
// --------------------------------------------------------------------------------------------------------------------------------
// ================================================================================================================================
