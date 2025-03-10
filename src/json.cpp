/*
 * Copyright (c) 2025, Matt Wlazlo
 *
 * This file is part of the td365 project.
 * Use in compliance with the Prosperity Public License 3.0.0.
 */

#include "td365.h"
#include <nlohmann/json.hpp>

// Serialization
void to_json(nlohmann::json &j, const market_group &mg) {
  j = {{"ID", mg.id},
       {"Name", mg.name},
       {"IsSuperGroup", mg.is_super_group},
       {"IsWhiteLabelPopularMarket", mg.is_white_label_popular_market},
       {"HasSubscription", mg.has_subscription}};
}

// Deserialization
void from_json(const nlohmann::json &j, market_group &mg) {
  j.at("ID").get_to(mg.id);
  j.at("Name").get_to(mg.name);
  j.at("IsSuperGroup").get_to(mg.is_super_group);
  j.at("IsWhiteLabelPopularMarket").get_to(mg.is_white_label_popular_market);
  j.at("HasSubscription").get_to(mg.has_subscription);
}

// Serialization
void to_json(nlohmann::json &j, const market &m) {
  j = {{"MarketID", m.market_id},
       {"QuoteID", m.quote_id},
       {"AtQuoteAtMarket", m.at_quote_at_market},
       {"ExchangeID", m.exchange_id},
       {"PrcGenFractionalPrice", m.prc_gen_fractional_price},
       {"PrcGenDecimalPlaces", m.prc_gen_decimal_places},
       {"High", m.high},
       {"Low", m.low},
       {"DailyChange", m.daily_change},
       {"Bid", m.bid},
       {"Ask", m.ask},
       {"BetPer", m.bet_per},
       {"IsGSLPercent", m.is_gsl_percent},
       {"GSLDis", m.gsl_dis},
       {"MinCloseOrderDisTicks", m.min_close_order_dis_ticks},
       {"MinOpenOrderDisTicks", m.min_open_order_dis_ticks},
       {"DisplayBetPer", m.display_bet_per},
       {"IsInPortfolio", m.is_in_portfolio},
       {"Tradable", m.tradable},
       {"TradeOnWeb", m.trade_on_web},
       {"CallOnly", m.call_only},
       {"MarketName", m.market_name},
       {"TradeStartTime", m.trade_start_time},
       {"Currency", m.currency},
       {"AllowGtdsStops", m.allow_gtds_stops},
       {"ForceOpen", m.force_open},
       {"Margin", m.margin},
       {"MarginType", m.margin_type},
       {"GSLCharge", m.gsl_charge},
       {"IsGSLChargePercent", m.is_gsl_charge_percent},
       {"Spread", m.spread},
       {"TradeRateType", m.trade_rate_type},
       {"OpenTradeRate", m.open_trade_rate},
       {"CloseTradeRate", m.close_trade_rate},
       {"MinOpenTradeRate", m.min_open_trade_rate},
       {"MinCloseTradeRate", m.min_close_trade_rate},
       {"PriceDecimal", m.price_decimal},
       {"Subscription", m.subscription},
       {"SuperGroupID", m.super_group_id}};
}

// Deserialization
void from_json(const nlohmann::json &j, market &m) {
  j.at("MarketID").get_to(m.market_id);
  j.at("QuoteID").get_to(m.quote_id);
  j.at("AtQuoteAtMarket").get_to(m.at_quote_at_market);
  j.at("ExchangeID").get_to(m.exchange_id);
  j.at("PrcGenFractionalPrice").get_to(m.prc_gen_fractional_price);
  j.at("PrcGenDecimalPlaces").get_to(m.prc_gen_decimal_places);
  j.at("High").get_to(m.high);
  j.at("Low").get_to(m.low);
  j.at("DailyChange").get_to(m.daily_change);
  j.at("Bid").get_to(m.bid);
  j.at("Ask").get_to(m.ask);
  j.at("BetPer").get_to(m.bet_per);
  j.at("IsGSLPercent").get_to(m.is_gsl_percent);
  j.at("GSLDis").get_to(m.gsl_dis);
  j.at("MinCloseOrderDisTicks").get_to(m.min_close_order_dis_ticks);
  j.at("MinOpenOrderDisTicks").get_to(m.min_open_order_dis_ticks);
  j.at("DisplayBetPer").get_to(m.display_bet_per);
  j.at("IsInPortfolio").get_to(m.is_in_portfolio);
  j.at("Tradable").get_to(m.tradable);
  j.at("TradeOnWeb").get_to(m.trade_on_web);
  j.at("CallOnly").get_to(m.call_only);
  j.at("MarketName").get_to(m.market_name);
  j.at("TradeStartTime").get_to(m.trade_start_time);
  j.at("Currency").get_to(m.currency);
  j.at("AllowGtdsStops").get_to(m.allow_gtds_stops);
  j.at("ForceOpen").get_to(m.force_open);
  j.at("Margin").get_to(m.margin);
  j.at("MarginType").get_to(m.margin_type);
  j.at("GSLCharge").get_to(m.gsl_charge);
  j.at("IsGSLChargePercent").get_to(m.is_gsl_charge_percent);
  j.at("Spread").get_to(m.spread);
  j.at("TradeRateType").get_to(m.trade_rate_type);
  j.at("OpenTradeRate").get_to(m.open_trade_rate);
  j.at("CloseTradeRate").get_to(m.close_trade_rate);
  j.at("MinOpenTradeRate").get_to(m.min_open_trade_rate);
  j.at("MinCloseTradeRate").get_to(m.min_close_trade_rate);
  j.at("PriceDecimal").get_to(m.price_decimal);
  j.at("Subscription").get_to(m.subscription);
  j.at("SuperGroupID").get_to(m.super_group_id);
}
