/*
 * Copyright (c) 2025, Matt Wlazlo
 *
 * This file is part of the td365 project.
 * Use in compliance with the Prosperity Public License 3.0.0.
 */

#include "api_client.h"
#include "json.h"
#include "utils.h"
#include <cassert>
#include <nlohmann/json.hpp>
#include <regex>

namespace beast = boost::beast;   // from <boost/beast.hpp>
namespace http = beast::http;     // from <boost/beast/http.hpp>
using json = nlohmann::json;

api_client::api_client(std::string_view host)
    : client_(host) {}

std::string api_client::connect(std::string path) {
  auto response = client_.get(path);
  if (response.result() == boost::beast::http::status::ok) {
    // extract the ots value here while we have the path
    // GET /Advanced.aspx?ots=WJFUMNFE
    // ots is the name of the cookie with the session token
    static std::regex re(R"(^.*?ots=(.*)$)");
    std::smatch m;
    assert(std::regex_match(path, m, re));
    auto ots = m[1].str();
    return ots;
  }
  assert(response.result() == boost::beast::http::status::found);
  const auto location = response.at(boost::beast::http::field::location);
  return connect(std::move(location));
}

std::string api_client::login(std::string path) {
  auto ots = connect(path);
  auto token = client_.jar().get(ots);

  client_.set_default_headers({
      {http::field::origin, "https://demo.tradedirect365.com"},
      {http::field::referer,
       "https://demo.tradedirect365.com/Advanced.aspx?ots=" + ots},
  });
  return token.value;
}

void api_client::update_session_token() {
  auto resp = client_.post("/UTSAPI.asmx/UpdateClientSessionID",
                           "application/json; charset=utf-8", "");
  assert(resp.result() == boost::beast::http::status::ok);
}

std::vector<market_group>
api_client::get_market_super_group() {
  auto resp = client_.post("/UTSAPI.asmx/GetMarketSuperGroup",
                           "application/json", "");
  assert(resp.result() == boost::beast::http::status::ok);
  auto j = json::parse(resp.body());
  return j["d"].get<std::vector<market_group>>();
}

std::vector<market_group>
api_client::get_market_group(unsigned int id) {
  json body = {{"superGroupId", id}};
  auto resp =
      client_.post("/UTSAPI.asmx/GetMarketGroup",
                   "application/json; charset=utf-8", body.dump());
  assert(resp.result() == boost::beast::http::status::ok);
  auto j = json::parse(resp.body());
  return j["d"].get<std::vector<market_group>>();
}

std::vector<market>
api_client::get_market_quote(unsigned int id) {
  json body = {
      {"groupID", id},      {"keyword", ""},   {"popular", false},
      {"portfolio", false}, {"search", false},
  };
  auto resp = client_.post("/UTSAPI.asmx/GetMarketQuote",
                           "application/json", body.dump());
  assert(resp.result() == boost::beast::http::status::ok);
  auto j = json::parse(resp.body());
  return j["d"].get<std::vector<market>>();
}
