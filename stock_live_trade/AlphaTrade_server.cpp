// This autogenerated skeleton file illustrates how to build a server.
// You should copy it to another filename to avoid overwriting it.

#include <time.h>

#include "AlphaTrade.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/server/TNonblockingServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>

#include "thread_broker_adapter.h"
#include "GBase64.h"
#include "utility.h"
#include "hexin_stock_broker_huatai.h"
#include "hexin_stock_broker_guangfa.h"
#include "GWin32CriticalSection.h"
#include "GScopedLock.h"
#include "license_manager.h"

#include <openssl/des.h>

#ifdef _WINDOWS
#define NULL_DEVICE "NUL:"
#else
#define NULL_DEVICE "/dev/null"
#endif

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

std::map<std::string, boost::shared_ptr<thread_broker_adapter>> g_map_tradeid_broker;
std::deque<boost::shared_ptr<thread_broker_adapter>> g_deq_wait_4_delete;


class AlphaTradeHandler : virtual public AlphaTradeIf {
 public:
  AlphaTradeHandler() {
    // Your initialization goes here
  }

  bool Running() {
	  // Your implementation goes here
	  return true;
  }

  int32_t GetPID() {
#ifdef _WINDOWS
	  return GetCurrentProcessId();
#else
	  return ::getpid();
#endif
  }


  void KeepAlive(const std::string& sessionID)
  {
	  LicenseManager::get_instance()->set_session_id(sessionID);
  }

  void LiveTradeLogin(LiveTradeLoginResp& _return, const std::string& sessionID, const std::string& account, const std::string& password1, const std::string& password2, const std::string& brokerStr) {
    
	std::string strUUID = gen_uuid();
	thread_broker_adapter *broker_ptr = NULL;

	if (brokerStr == "stock_huatai") {

		broker_ptr = new thread_broker_adapter(new hexin_stock_broker_huatai(LicenseManager::get_instance()->get_broker_server("stock_huatai"), account, password1, password2, true), false);
	}
	else if (brokerStr == "stock_guangfa") {
		broker_ptr = new thread_broker_adapter(new hexin_stock_broker_guangfa(LicenseManager::get_instance()->get_broker_server("stock_guangfa"), account, password1, password2, true), false);
	}
	else if (brokerStr == "stock_haitong") {
		std::vector<boost::tuple<std::string, uint16_t>> server_vec;
		server_vec.push_back(boost::make_tuple("Htjybeta.htsec.com", 8002));
		broker_ptr = new thread_broker_adapter(new hexin_stock_broker_huatai(server_vec, account, password1, password2, true), false);
	}
	else if (brokerStr == "stock_yinhe") {
		std::vector<boost::tuple<std::string, uint16_t>> server_vec;
		server_vec.push_back(boost::make_tuple("219.143.214.202", 6002));
		broker_ptr = new thread_broker_adapter(new hexin_stock_broker_guangfa(server_vec, account, password1, password2, true), false);
	}
	else {
		_return.ret_code = 1;
		return;
	}

	broker_ptr->start();
	g_map_tradeid_broker[strUUID] = boost::shared_ptr<thread_broker_adapter>(broker_ptr);

	_return.result = strUUID;
	_return.ret_code = 0;

	while (g_deq_wait_4_delete.size() > 0 && g_deq_wait_4_delete[0]->stopped()) {
		g_deq_wait_4_delete.pop_front();
	}
  }

  void LiveTradeLogout(LiveTradeLogoutResp& _return, const std::string& sessionID, const std::string& liveTradeID) {
    
	std::map<std::string, boost::shared_ptr<thread_broker_adapter>>::iterator iter = g_map_tradeid_broker.find(liveTradeID);
	if (iter != g_map_tradeid_broker.end()) {
		iter->second->stop();
		g_deq_wait_4_delete.push_back(iter->second);
		g_map_tradeid_broker.erase(iter);

		//TODO
	}
	else {

	}

	_return.ret_code = 0;
  }

  void GetAccountState(GetAccountStateResp& _return, const std::string& sessionID, const std::string& liveTradeID) {
    
	std::map<std::string, boost::shared_ptr<thread_broker_adapter>>::iterator iter = g_map_tradeid_broker.find(liveTradeID);
	if (iter != g_map_tradeid_broker.end()) {
		_return.ret_code = 0;
		_return.state = iter->second->getAccountState();
	}
	else {

	}
  }

  void GetAccountBalance(GetAccountBalanceResp& _return, const std::string& sessionID, const std::string& liveTradeID) {
    
	std::map<std::string, boost::shared_ptr<thread_broker_adapter>>::iterator iter = g_map_tradeid_broker.find(liveTradeID);
	if (iter != g_map_tradeid_broker.end()) {

		std::vector<double> balance = iter->second->getBalance();
		if (balance.size() > 0) {
			_return.ret_code = 0;
			_return.result.total_value = balance[0];
			_return.result.money_left = balance[1];
		}
		else {
			_return.ret_code = 3;
		}
	}
	else {

	}

  }

  void GetHoldingStock(GetHoldingStockResp& _return, const std::string& sessionID, const std::string& liveTradeID) {
    
	std::map<std::string, boost::shared_ptr<thread_broker_adapter>>::iterator iter = g_map_tradeid_broker.find(liveTradeID);
	if (iter != g_map_tradeid_broker.end()) {
		_return.ret_code = 0;
		std::vector<holding_item> holding = iter->second->getHoldingStock();

		for (int i = 0; i < holding.size(); ++i) {
			HoldingStock item;
			item.stock_id = holding[i].sid;
			item.long_short = "long";
			item.buy_price = holding[i].buy_price;
			item.quant = holding[i].quant;
			item.quant_sellable = holding[i].quant_available;
			
			_return.result.push_back(item);
		}
	}

  }

  void GetAllOrder(GetAllOrderResp& _return, const std::string& sessionID, const std::string& liveTradeID) {
	  std::map<std::string, boost::shared_ptr<thread_broker_adapter>>::iterator iter = g_map_tradeid_broker.find(liveTradeID);
	  if (iter != g_map_tradeid_broker.end()) {
		  _return.ret_code = 0;
		  std::vector<order_state> order_vec = iter->second->getAllOrder();

		  for (int i = 0; i < order_vec.size(); ++i) {
			  OrderState os;
			  os.operation = order_vec[i].operation;
			  os.direction = order_vec[i].direction;
			  os.deal_price = order_vec[i].deal_price;
			  os.deal_quant = order_vec[i].deal_quant;
			  os.desc = Base64::Encode(order_vec[i].desc);
			  os.direction = order_vec[i].direction;
			  os.price = order_vec[i].price;
			  os.quant = order_vec[i].quant;
			  os.sid = order_vec[i].sid;
			  os.state = order_vec[i].state;
			  os.time = order_vec[i].time;
			  os.internal_order_id = order_vec[i].internal_order_id;
			  os.order_id = order_vec[i].user_order_id;
			  _return.result.push_back(os);
		  }
	  }
  }

  void GetOrderState(GetOrderStateResp& _return, const std::string& sessionID, const std::string& liveTradeID, const std::string& orderID) {
   
	std::map<std::string, boost::shared_ptr<thread_broker_adapter>>::iterator iter = g_map_tradeid_broker.find(liveTradeID);
	if (iter != g_map_tradeid_broker.end()) {
		_return.ret_code = 0;
		order_state ost = iter->second->getOrderState(orderID);
		_return.result.operation = ost.operation;
		_return.result.deal_price = ost.deal_price;
		_return.result.deal_quant = ost.deal_quant;
		_return.result.desc = Base64::Encode(ost.desc);
		_return.result.direction = ost.direction;
		_return.result.price = ost.price;
		_return.result.quant = ost.quant;
		_return.result.sid = ost.sid;
		_return.result.state = ost.state;
		_return.result.time = ost.time;
		_return.result.order_id = ost.user_order_id;
		_return.result.internal_order_id = ost.internal_order_id;
	}
  }

  void CloseOrder(CloseOrderResp& _return, const std::string& sessionID, const std::string& liveTradeID, const std::string& orderID) {
    
	std::map<std::string, boost::shared_ptr<thread_broker_adapter>>::iterator iter = g_map_tradeid_broker.find(liveTradeID);
	if (iter != g_map_tradeid_broker.end()) {
		_return.ret_code = 0;
		iter->second->closeOrder(orderID);
	}

  }

  void CancelOrder(CancelOrderResp& _return, const std::string& sessionID, const std::string& liveTradeID, const std::string& orderID) {
	  std::map<std::string, boost::shared_ptr<thread_broker_adapter>>::iterator iter = g_map_tradeid_broker.find(liveTradeID);
	  if (iter != g_map_tradeid_broker.end()) {
		  _return.ret_code = 0;
		  iter->second->closeOrder(orderID);
	  }
  }

  void LiveTradeBuyOpen(PlaceOrderResp& _return, const std::string& sessionID, const std::string& liveTradeID, const std::string& sid, const double price, const double quant, const std::string& orderType) 
  {
    
	  LicenseManager::get_instance()->set_session_id(sessionID);

	std::string msg;
	if (!LicenseManager::get_instance()->license_ok(_return.ret_code, msg)) {
		return;
	}

	std::map<std::string, boost::shared_ptr<thread_broker_adapter>>::iterator iter = g_map_tradeid_broker.find(liveTradeID);
	if (iter != g_map_tradeid_broker.end()) {
		_return.ret_code = 0;
		std::string orderID = iter->second->buy(sid, quant, price, orderType);
		_return.result = orderID;

		if (orderID == "") {
			_return.ret_code = 10;
		}
	}

  }

  void LiveTradeSellClose(PlaceOrderResp& _return, const std::string& sessionID, const std::string& liveTradeID, const std::string& sid, const double price, const double quant, const std::string& orderType, const bool closeToday) 
{

	  LicenseManager::get_instance()->set_session_id(sessionID);

	std::string msg;
	if (!LicenseManager::get_instance()->license_ok(_return.ret_code, msg)) {
		return;
	}

	std::map<std::string, boost::shared_ptr<thread_broker_adapter>>::iterator iter = g_map_tradeid_broker.find(liveTradeID);
	if (iter != g_map_tradeid_broker.end()) {
		_return.ret_code = 0;
		std::string orderID = iter->second->sell(sid, quant, price, orderType);

		_return.result = orderID;
		if (orderID == "") {
			_return.ret_code = 10;
		}
	}
  }

  void LiveTradeSellOpen(PlaceOrderResp& _return, const std::string& sessionID, const std::string& liveTradeID, const std::string& sid, const double price, const double quant, const std::string& orderType) 
  {
    
  }

  void LiveTradeBuyClose(PlaceOrderResp& _return, const std::string& sessionID, const std::string& liveTradeID, const std::string& sid, const double price, const double quant, const std::string& orderType, const bool closeToday) {
	 
  }
};

int main(int argc, char **argv) {
	
	//freopen(NULL_DEVICE, "w", stderr);

	int port = 58899;

	init_lib();

	try {
		if (argc >= 3) {
			port = atoi(argv[2]);
		}
		boost::shared_ptr<AlphaTradeHandler> handler(new AlphaTradeHandler());
		boost::shared_ptr<TProcessor> processor(new AlphaTradeProcessor(handler));
		boost::shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
		boost::shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
		boost::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

		TNonblockingServer server(processor, protocolFactory, port);

		LicenseManager::get_instance()->start();

		std::cout << "AlphaQuant 1.1.8.3396 listen on port " << port << std::endl;

		//TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);
		server.serve();
	}
	catch (std::exception& e) {
		std::cout << "except: " << e.what() << endl;
	}
	return 0;
}

