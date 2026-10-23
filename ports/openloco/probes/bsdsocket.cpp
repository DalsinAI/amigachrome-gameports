#include "Network/Socket.h"
#undef NDEBUG
#include <cassert>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <proto/dos.h>
using namespace OpenLoco::Network;
int main() {
    try {
        auto receiver=Socket::createUdp();auto sender=Socket::createUdp();
        receiver->listen(Protocol::ipv4,"127.0.0.1",35871);
        const char message[]="OpenLoco AC090 IPv4";
        assert(sender->sendData(Protocol::ipv4,"127.0.0.1",35871,message,sizeof message)==sizeof message);
        char buffer[128]{};size_t received=0;std::unique_ptr<INetworkEndpoint> peer;
        for(int i=0;i<100 && !received;++i) {
            receiver->receiveData(buffer,sizeof buffer,&received,&peer);
            if(!received)Delay(1);
        }
        assert(received==sizeof message && !std::memcmp(buffer,message,received));
        assert(peer && peer->getIpAddress()=="127.0.0.1");
        auto resolved=Socket::resolve(Protocol::any,"localhost",35871);
        assert(resolved->getProtocol()==Protocol::ipv4);
        bool rejected=false;
        try { Socket::resolve(Protocol::ipv6,"::1",35871); }
        catch(const std::exception&) { rejected=true; }
        assert(rejected);
        sender->close();receiver->close();
        std::puts("PASS: native bsdsocket IPv4 UDP loopback, resolver, nonblocking receive and explicit IPv6 rejection");
        return 0;
    } catch(const std::exception& e) { std::fprintf(stderr,"FAIL: %s\n",e.what());return 10; }
}
