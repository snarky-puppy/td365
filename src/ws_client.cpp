/*
 * Copyright (c) 2025, Matt Wlazlo
 *
 * This file is part of the td365 project.
 * Use in compliance with the Prosperity Public License 3.0.0.
 */

#include "ws_client.h"
#include "constants.h"
#include "parsing.h"
#include "td365.h"
#include "utils.h"
#include "ws.h"
#include <boost/asio/detached.hpp>
#include <boost/asio/ssl.hpp>
#include <iostream>
#include <nlohmann/json.hpp>
#include <print>
#include <cstdlib>
#include <ranges>
#include <boost/lexical_cast.hpp>

namespace net = boost::asio;
using net::awaitable;
using net::use_awaitable;
namespace ssl = boost::asio::ssl;
using json = nlohmann::json;

enum class payload_type {
  heartbeat,
  connect_response,
  reconnect_response,
  authentication_response,
  unknown,
  subscribe_response,
  price_data
};

constexpr auto port = "443";

bool
is_debug_enabled() {
  static bool enabled = [] {
    auto value = std::getenv("DEBUG");
    return value ? boost::lexical_cast<bool>(value) : false;
  }();
  return enabled;
}

payload_type string_to_payload_type(std::string_view str) {
  static const std::unordered_map<std::string_view, payload_type> lookup = {
    {"heartbeat", payload_type::heartbeat},
    {"connectResponse", payload_type::connect_response},
    {"reconnectResponse", payload_type::reconnect_response},
    {"authenticationResponse", payload_type::authentication_response},
    {"subscribeResponse", payload_type::subscribe_response},
    {"p", payload_type::price_data},
  };

  auto it = lookup.find(str);
  return (it != lookup.end()) ? it->second : payload_type::unknown;
}

// Using string_to_price_type from parsing.h

template<typename Awaitable>
auto ws_client::run_awaitable(Awaitable awaitable) ->
  typename Awaitable::value_type {
  std::promise<typename Awaitable::value_type> promise;
  auto future = promise.get_future();

  net::co_spawn(
    io_context_.get_executor(),
    [awaitable = std::move(awaitable),
      &promise]() mutable -> net::awaitable<void> {
      try {
        auto result = co_await std::move(awaitable);
        promise.set_value(std::move(result));
      } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        promise.set_exception(std::current_exception());
      }
    },
    net::detached);

  return future.get();
}

auto ws_client::run_awaitable(net::awaitable<void> awaitable) -> void {
  std::promise<void> promise;
  auto future = promise.get_future();

  net::co_spawn(
    io_context_.get_executor(),
    [awaitable = std::move(awaitable),
      &promise]() mutable -> net::awaitable<void> {
      try {
        co_await std::move(awaitable);
        promise.set_value();
      } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        promise.set_exception(std::current_exception());
      }
    },
    net::detached);

  future.get();
}

ws_client::ws_client(const std::atomic<bool> &shutdown,
                     std::function<void(const tick &)> &&tick_callback)
  : work_guard_(io_context_.get_executor())
    , ws_(std::make_unique<ws>(io_context_.get_executor()))
    , shutdown_(shutdown)
    , auth_future_(auth_promise_.get_future())
    , tick_callback_(std::move(tick_callback))
    , disconnect_future_(disconnect_promise_.get_future()) {
  // Start the IO thread
  io_thread_ = std::thread([this]() {
    try {
      ssl_ctx.set_default_verify_paths();
      io_context_.run();
    } catch (const std::exception &e) {
      std::cerr << "IO thread exception: " << e.what() << std::endl;
    }
  });
}

ws_client::~ws_client() {
  try {
    // Signal to stop the IO context
    work_guard_.reset();
    io_context_.stop();

    // Join the IO thread
    if (io_thread_.joinable()) {
      io_thread_.join();
    }
  } catch (const std::exception &e) {
    std::cerr << "ws_client dtor: " << e.what() << std::endl;
  }
}

void ws_client::start_loop(std::string host, std::string login_id, std::string token) {
  net::co_spawn(
    io_context_.get_executor(),
    [this, token, login_id, host]() -> net::awaitable<void> {
      co_await connect(host);
      while (!shutdown_) {
        auto ec = co_await process_messages(login_id, token);
        if (ec)
          if (ec == boost::beast::websocket::error::closed) {
            co_await reconnect();
            continue;
          }
        std::println(std::cerr, "ws_client: {} ({})", ec.message(), ec.what());

        // Mark as disconnected and notify any waiting threads
        connected_ = false;
        disconnect_promise_.set_value();

        throw boost::system::system_error(ec);
      }
    },
    net::detached);

  auth_future_.wait();
}

boost::asio::awaitable<void> ws_client::close() {
  if (connected_) {
    co_await ws_->close();
    connected_ = false;
    disconnect_promise_.set_value();
  }
}

void ws_client::close_sync() {
  if (connected_) {
    run_awaitable(close());
  }
}

boost::asio::awaitable<void> ws_client::subscribe(int quote_id) {
  co_await send({
    {"quoteId", quote_id},
    {"priceGrouping", "Sampled"},
    {"action", "subscribe"}
  });
}

void ws_client::subscribe_sync(int quote_id) {
  run_awaitable(subscribe(quote_id));
}

boost::asio::awaitable<void> ws_client::unsubscribe(int quote_id) {
  co_await send({
    {"quoteId", quote_id},
    {"priceGrouping", "Sampled"},
    {"action", "unsubscribe"}
  });
}

void ws_client::unsubscribe_sync(int quote_id) {
  run_awaitable(unsubscribe(quote_id));
}

void ws_client::wait_for_disconnect() {
  if (connected_) {
    // Wait for the disconnect_future to be ready
    disconnect_future_.wait();
  }
}

boost::asio::awaitable<void> ws_client::reconnect() {
  ws_.reset(new ws(io_context_.get_executor()));

  // Optionally wait a second before reconnecting.
  net::steady_timer timer(io_context_);
  timer.expires_after(std::chrono::seconds(1));
  co_await timer.async_wait(use_awaitable);
}

boost::asio::awaitable<void> ws_client::connect(const std::string &host) {
  co_await ws_->connect(host, port);

  // Mark as connected and reset disconnect promise
  connected_ = true;
  // Create a new promise in case this is a reconnect
  disconnect_promise_ = std::promise<void>();
  disconnect_future_ = disconnect_promise_.get_future();
}

boost::asio::awaitable<void> ws_client::send(const nlohmann::json &body) {
  co_await ws_->send(body.dump());
}

boost::asio::awaitable<::boost::beast::error_code>
ws_client::process_messages(const std::string &login_id,
                            const std::string &token) {
  while (!shutdown_) {
    auto [ec, buf] = co_await ws_->read_message();
    if (ec) {
      co_return ec;
    }

    auto msg = nlohmann::json::parse(buf);

    if (is_debug_enabled()) {
      std::cout << msg.dump() << std::endl;
    }

    switch (string_to_payload_type(msg["t"].get<std::string>())) {
      case payload_type::connect_response:
        co_await process_connect_response(msg, login_id, token);
        break;
      case payload_type::heartbeat:
        co_await process_heartbeat(msg);
        break;
      case payload_type::authentication_response:
        co_await process_authentication_response(msg);
        break;
      case payload_type::subscribe_response:
        process_subscribe_response(msg);
        break;
      case payload_type::price_data:
        process_price_data(msg);
        break;
      default:
        std::cerr << "Unhandled message" << msg.dump() << std::endl;
    }
  }
  co_return boost::system::error_code();
}

boost::asio::awaitable<void>
ws_client::process_heartbeat(const nlohmann::json &j) {
  auto now = now_utc();
  co_await send({
    {"SentByServer", j["d"]["SentByServer"]},
    {"MessagesReceived", j["d"]["MessagesReceived"]},
    {"PricesReceived", j["d"]["PricesReceived"]},
    {"MessagesSent", j["d"]["MessagesSent"]},
    {"PricesSent", j["d"]["PricesSent"]},
    {"Visible", true},
    {"action", "heartbeat"},
  });
  co_return;
}

boost::asio::awaitable<void>
ws_client::process_connect_response(const nlohmann::json &msg,
                                    const std::string &login_id,
                                    const std::string &token) {
  co_await send({
    {"action", "authentication"},
    {"loginId", login_id},
    {"tradingAccountType", "SPREAD"},
    {"token", token},
    {"reason", "Connect"},
    {"clientVersion", supported_version_}
  });
  co_return;
}

boost::asio::awaitable<void>
ws_client::process_authentication_response(const nlohmann::json &msg) {
  // FIXME: assumes everything is good here
  auth_promise_.set_value();
  co_return;
}

void ws_client::deliver_tick(tick &&t) {
  if (tick_callback_) {
    tick_callback_(t);
  }
}

void ws_client::process_price_data(const nlohmann::json &msg) {
  const auto &data = msg["d"];

  for (const auto &key: grouping_map) {
    if (auto it = data.find(key.first);
      it != data.end() && it->is_array() && !it->empty()) {
      try {
        auto prices = it->get<std::vector<std::string> >();
        for (const auto &price: prices) {
          deliver_tick(parse_tick(price, key.second));
        }
      } catch (const nlohmann::json::exception &e) {
        std::cerr << "Error parsing price data: " << e.what() << std::endl;
      }
    }
  }
}

void ws_client::process_subscribe_response(const nlohmann::json &msg) {
  auto d = msg["d"];
  assert(d["HasError"].get<bool>() == false);
  auto prices = d["Current"].get<std::vector<std::string> >();
  auto g = string_to_price_type(d["PriceGrouping"].get<std::string>());
  for (const auto &p: prices) {
    deliver_tick(parse_tick(p, g));
  }
}
