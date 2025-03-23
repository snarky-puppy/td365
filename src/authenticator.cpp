/*
 * Copyright (c) 2025, Matt Wlazlo
 *
 * This file is part of the td365 project.
 * Use in compliance with the Prosperity Public License 3.0.0.
 */

#include "authenticator.h"
#include "http_client.h"
#include "utils.h"
#include <cassert>
#include <charconv>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

static constexpr auto OAuthTokenHost = "td365.eu.auth0.com";
static constexpr auto PortalSiteHost = "portal-api.tradenation.com";

static constexpr auto ProdSiteHost = "traders.td365.com";
static constexpr auto ProdAPIHost = "prod-api.finsa.com.au";
static constexpr auto ProdSockHost = "prod-api.finsa.com.au";

static constexpr auto DemoSiteHost = "demo.tradedirect365.com.au";
static constexpr auto DemoAPIHost = "demo-api.finsa.com.au";
static constexpr auto DemoSockHost = "demo-api.finsa.com.au";

struct auth_token {
    static auth_token load() {
        std::ifstream file("auth_token.json");
        if (file) {
            auth_token token;
            nlohmann::json j;
            file >> j;
            token.access_token = j.value("access_token", "");
            token.id_token = j.value("id_token", "");
            std::time_t expiry = j.value("expiry_time", 0);
            token.expiry_time = std::chrono::system_clock::from_time_t(expiry);
            return token;
        }
        return auth_token{};
    }

    void save() {
        nlohmann::json j;
        j["access_token"] = access_token;
        j["id_token"] = id_token;
        std::time_t expiry = std::chrono::system_clock::to_time_t(expiry_time);
        j["expiry_time"] = expiry;
        std::ofstream file("auth_token.json", std::ios::trunc);
        file << j.dump(4);
    }

    std::string access_token;
    std::string id_token;
    std::chrono::system_clock::time_point expiry_time;
};

auth_token
login(const std::string &username, const std::string &password) {
    http_client cli(OAuthTokenHost);
    json body = {
        {"realm", "Username-Password-Authentication"},
        {"client_id", "eeXrVwSMXPZ4pJpwStuNyiUa7XxGZRX9"},
        {"scope", "openid"},
        {"grant_type", "http://auth0.com/oauth/grant-type/password-realm"},
        {"username", username},
        {"password", password},
    };
    const auto response =
            cli.post("/oauth/token", "application/json", body.dump());
    if (response.result() != boost::beast::http::status::ok) {
        throw std::runtime_error(response.body());
    }

    auto json_response = json::parse(response.body());

    return auth_token{
        .access_token = json_response["access_token"].get<std::string>(),
        .id_token = json_response["id_token"].get<std::string>(),
        .expiry_time =
        std::chrono::system_clock::now() +
        std::chrono::seconds(json_response["expires_in"].get<int>())
    };
}

json select_account(http_client &client, const std::string &account_id) {
    auto response = client.get("/TD365/user/accounts/");
    for (auto j = json::parse(response.body());
         const auto &account: j["results"]) {
        if (account["account"] == account_id) {
            return account;
        }
    }
    throw std::runtime_error("account not found");
}

splitted_url
fetch_platform_url(http_client &client, const std::string &launch_url) {
    auto response = client.get(launch_url);
    assert(response.result() == boost::beast::http::status::ok);

    auto j = json::parse(response.body());
    auto loginagent_url = j["url"].get<std::string>();

    return split_url(loginagent_url);
}

namespace authenticator {
    account_detail
    authenticate() {
        return account_detail{
            // the "?aid=1026" is required for valid login
            .platform_url = {"demo.tradedirect365.com", "/finlogin/OneClickDemo.aspx?aid=1026"},
            .login_id = "",
            .account_type = demo,
            .site_host = DemoSiteHost,
            .api_host = DemoAPIHost,
            .sock_host = DemoSockHost,
        };
    }

    account_detail
    authenticate(std::string username, std::string password,
                 std::string account_id) {
        auto token = auth_token::load();
        if (std::chrono::system_clock::now() > token.expiry_time) {
            token = login(username, password);
            token.save();
        }

        http_client client(PortalSiteHost);

        std::ostringstream ostr;
        ostr << "Bearer " << token.access_token;
        client.set_default_headers(
            {{boost::beast::http::field::authorization, ostr.str()}});

        auto account = select_account(client, account_id);

        account_detail details;
        details.account_type = account["accountType"] == "DEMO" ? demo : prod;

        details.platform_url = fetch_platform_url(
            client, account["button"]["linkTo"].get<std::string>());
        details.login_id = account["ct_login_id"].get<std::string>();

        details.site_host =
                std::string(details.account_type == demo ? DemoSiteHost : ProdSiteHost);
        details.api_host =
                std::string(details.account_type == demo ? DemoAPIHost : ProdAPIHost);
        details.sock_host =
                std::string(details.account_type == demo ? DemoSockHost : ProdSockHost);

        return details;
    }
}
