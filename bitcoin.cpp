#include <algorithm>

#include "db.h"
#include "netbase.h"
#include "protocol.h"
#include "serialize.h"
#include "uint256.h"

#define BITCOIN_SEED_NONCE  0x0539a019ca550825ULL

using namespace std;

class CNode {
  SOCKET sock;
  CDataStream vSend;
  CDataStream vRecv;
  unsigned int nHeaderStart;
  unsigned int nMessageStart;
  int nVersion;
  string strSubVer;
  int nStartingHeight;
  vector<CAddress> *vAddr;
  int ban;
  int64 doneAfter;
  CAddress you;

  int GetTimeout() {
      if (you.IsTor())
          return 120;
      else
          return 30;
  }

  void BeginMessage(const char *pszCommand) {
    if (nHeaderStart != -1) AbortMessage();
    nHeaderStart = vSend.size();
    vSend << CMessageHeader(pszCommand, 0);
    nMessageStart = vSend.size();
//    printf("%s: SEND %s\n", ToString(you).c_str(), pszCommand); 
  }
  
  void AbortMessage() {
    if (nHeaderStart == -1) return;
    vSend.resize(nHeaderStart);
    nHeaderStart = -1;
    nMessageStart = -1;
  }
  
  void EndMessage() {
    if (nHeaderStart == -1) return;
    unsigned int nSize = vSend.size() - nMessageStart;
    memcpy((char*)&vSend[nHeaderStart] + offsetof(CMessageHeader, nMessageSize), &nSize, sizeof(nSize));
    if (vSend.GetVersion() >= 209) {
      uint256 hash = Hash(vSend.begin() + nMessageStart, vSend.end());
      unsigned int nChecksum = 0;
      memcpy(&nChecksum, &hash, sizeof(nChecksum));
      assert(nMessageStart - nHeaderStart >= offsetof(CMessageHeader, nChecksum) + sizeof(nChecksum));
      memcpy((char*)&vSend[nHeaderStart] + offsetof(CMessageHeader, nChecksum), &nChecksum, sizeof(nChecksum));
    }
    nHeaderStart = -1;
    nMessageStart = -1;
  }
  
  void Send() {
    if (sock == INVALID_SOCKET) return;
    if (vSend.empty()) return;
    int nBytes = send(sock, &vSend[0], vSend.size(), 0);
    if (nBytes > 0) {
      vSend.erase(vSend.begin(), vSend.begin() + nBytes);
    } else {
      close(sock);
      sock = INVALID_SOCKET;
    }
  }
  
  void PushVersion() {
    int64 nTime = time(NULL);
    uint64 nLocalNonce = BITCOIN_SEED_NONCE;
    int64 nLocalServices = 0;
    CAddress me(CService("0.0.0.0"));
    BeginMessage("version");
    int nBestHeight = GetRequireHeight();
    string ver = "/BitSend-seeder:0.15.0.3/";
    vSend << PROTOCOL_VERSION << nLocalServices << nTime << you << me << nLocalNonce << ver << nBestHeight;
    EndMessage();
  }
 
  void GotVersion() {
    // printf("\n%s: version %i\n", ToString(you).c_str(), nVersion);
    if (vAddr) {
      BeginMessage("getaddr");
      EndMessage();
      doneAfter = time(NULL) + GetTimeout();
    } else {
      doneAfter = time(NULL) + 1;
    }
  }

  bool ProcessMessage(string strCommand, CDataStream& vRecv) {
//    printf("%s: RECV %s\n", ToString(you).c_str(), strCommand.c_str());
    if (strCommand == "version") {
      int64 nTime;
      CAddress addrMe;
      CAddress addrFrom;
      uint64 nNonce = 1;
      vRecv >> nVersion >> you.nServices >> nTime >> addrMe;
      if (nVersion == 10300) nVersion = 300;
      if (nVersion >= 106 && !vRecv.empty())
        vRecv >> addrFrom >> nNonce;
      if (nVersion >= 106 && !vRecv.empty())
        vRecv >> strSubVer;
	
      if (strSubVer.find("/Satoshi:0.10.0/") != std::string::npos
              || strSubVer.find("/Satoshi:0.7.2/") != std::string::npos
              || strSubVer.find("/Satoshi:0.8.0/") != std::string::npos
              || strSubVer.find("/Satoshi:0.8.1/") != std::string::npos
              || strSubVer.find("/Satoshi:0.8.5/") != std::string::npos
              || strSubVer.find("/Satoshi:0.8.6/") != std::string::npos
              || strSubVer.find("/Satoshi:0.9.1/") != std::string::npos
              || strSubVer.find("/Satoshi:0.9.2/") != std::string::npos
              || strSubVer.find("/Satoshi:0.9.2.1/") != std::string::npos
              || strSubVer.find("/Satoshi:0.9.3/") != std::string::npos
              || strSubVer.find("/Satoshi:0.9.4/") != std::string::npos
              || strSubVer.find("/Satoshi:0.9.5/") != std::string::npos
              || strSubVer.find("/Satoshi:0.9.99/") != std::string::npos
              || strSubVer.find("/Satoshi:0.10.0/") != std::string::npos
              || strSubVer.find("/Satoshi:0.10.1/") != std::string::npos
              || strSubVer.find("/Satoshi:0.10.2/") != std::string::npos
              || strSubVer.find("/Satoshi:0.10.3/") != std::string::npos
              || strSubVer.find("/Satoshi:0.10.4/") != std::string::npos
              || strSubVer.find("/Satoshi:0.11.0/") != std::string::npos
              || strSubVer.find("/Satoshi:0.11.1/") != std::string::npos
              || strSubVer.find("/Satoshi:0.11.2/") != std::string::npos
              || strSubVer.find("/Satoshi:0.11.2(bitcore)/") != std::string::npos
              || strSubVer.find("/Satoshi:0.11.3/") != std::string::npos
              || strSubVer.find("/Satoshi:0.12.0/") != std::string::npos
              || strSubVer.find("/Satoshi:0.12.1/") != std::string::npos
              || strSubVer.find("/Satoshi:0.12.99/") != std::string::npos
              || strSubVer.find("/Statoshi:0.12.99/") != std::string::npos
              || strSubVer.find("/Satoshi:0.12.1(bitcore)/") != std::string::npos
              || strSubVer.find("/Satoshi:0.13.0/") != std::string::npos
              || strSubVer.find("/Satoshi:0.13.0(bitcore)/") != std::string::npos
              || strSubVer.find("/Satoshi:0.13.1/") != std::string::npos
              || strSubVer.find("/Satoshi:0.13.2/") != std::string::npos
              || strSubVer.find("/Satoshi:0.13.99/") != std::string::npos
              || strSubVer.find("/Satoshi:0.14.0/") != std::string::npos
              || strSubVer.find("/Satoshi:0.14.1/") != std::string::npos
              || strSubVer.find("/Satoshi:0.14.1(UASF-SegWit-BIP148)/") != std::string::npos
              || strSubVer.find("/Satoshi:0.14.1(UASF-SegWit-BIP148)/Knots:20170420/") != std::string::npos
              || strSubVer.find("/Satoshi:0.14.1.6/") != std::string::npos
              || strSubVer.find("/Satoshi:0.14.2/") != std::string::npos
              || strSubVer.find("/Satoshi:0.14.2(OI)/") != std::string::npos
              || strSubVer.find("/Satoshi:0.14.2(bitcore)/") != std::string::npos
              || strSubVer.find("/Satoshi:0.14.99/") != std::string::npos
              || strSubVer.find("/Satoshi:0.15.0/") != std::string::npos
              || strSubVer.find("/Satoshi:0.15.0(UASF-SegWit-BIP148)/") != std::string::npos
              || strSubVer.find("/Satoshi:0.15.0()/") != std::string::npos
              || strSubVer.find("/Satoshi:0.15.0(bitcore)/") != std::string::npos
              || strSubVer.find("/Satoshi:0.15.0(minerbitcoin)/") != std::string::npos
              || strSubVer.find("/Satoshi:0.15.0(bitcore-sl)/") != std::string::npos
              || strSubVer.find("/Satoshi:0.15.0.1/") != std::string::npos
              || strSubVer.find("/Satoshi:0.15.0.1(bitcore)/") != std::string::npos
              || strSubVer.find("/Satoshi:0.15.0.1(wzrdtales)/") != std::string::npos
              || strSubVer.find("/Satoshi:0.15.1/") != std::string::npos
              || strSubVer.find("/Satoshi:0.15.1(no2x; rBitcoin; HODL)/") != std::string::npos
              || strSubVer.find("/Satoshi:0.15.1(NO2X)/") != std::string::npos
              || strSubVer.find("/Satoshi:0.15.1(bitcore)/") != std::string::npos
              || strSubVer.find("/Satoshi:0.15.99/") != std::string::npos
              || strSubVer.find("/Satoshi:0.16.0()/") != std::string::npos
              || strSubVer.find("/Satoshi:0.16.0/") != std::string::npos
              || strSubVer.find("/Satoshi:0.16.0(UASF-SegWit-BIP 148)/") != std::string::npos
              || strSubVer.find("/Satoshi:0.16.0(Delete Conbase)/") != std::string::npos
              || strSubVer.find("/Satoshi:0.16.0(TheRoom)/") != std::string::npos
              || strSubVer.find("/Satoshi:0.16.0(bitcore)/") != std::string::npos
              || strSubVer.find("/Satoshi:0.16.0(prometheus)/") != std::string::npos
              || strSubVer.find("/Satoshi:0.16.0(BCASH-IS-AN-ALTCOIN)/") != std::string::npos
              || strSubVer.find("/Satoshi:0.16.0(needs-coffee)/") != std::string::npos
              || strSubVer.find("/Satoshi:0.16.0(Hostknight.net - Affordable DDOS protected hosting)/") != std::string::npos
              || strSubVer.find("/Satoshi:0.16.0(UASF-SegWit-BIP148)/") != std::string::npos
              || strSubVer.find("/Satoshi:0.16.0(NO2X)/") != std::string::npos
              || strSubVer.find("/Satoshi:0.16.0(the.lightning.land)/") != std::string::npos
              || strSubVer.find("/Satoshi:0.16.1/") != std::string::npos
              || strSubVer.find("/Satoshi:0.16.1()/") != std::string::npos
              || strSubVer.find("/Satoshi:0.16.1(SegWit)/") != std::string::npos
              || strSubVer.find("/Satoshi:0.16.1(MONEYBADGER_DONT_CARE)/") != std::string::npos
              || strSubVer.find("/Satoshi:0.16.1(LeiTech-GbR)/") != std::string::npos
              || strSubVer.find("/Satoshi:0.16.1(Quantitative Easing LOL)/") != std::string::npos
              || strSubVer.find("/Satoshi:0.16.1(UASF-SegWit-BIP148)/") != std::string::npos
              || strSubVer.find("/Satoshi:0.16.1(BIP91)/") != std::string::npos
              || strSubVer.find("/Satoshi:0.16.1(Bcash is not Bitcoin)/") != std::string::npos
              || strSubVer.find("/Satoshi:0.16.1.7/") != std::string::npos
              || strSubVer.find("/Satoshi:0.16.99(bitcoin-sl)/") != std::string::npos
              || strSubVer.find("/Statoshi:0.16.99/") != std::string::npos
              || strSubVer.find("/Satoshi:0.16.99/") != std::string::npos
              || strSubVer.find("/Statoshi:0.16.99/") != std::string::npos
              || strSubVer.find("/Satoshi:0.16.99(bitcoin-sl)/") != std::string::npos
              || strSubVer.find("/Satoshi:0.16.99(Medea)/") != std::string::npos
              || strSubVer.find("/Satoshi:1.1.1/") != std::string::npos
              || strSubVer.find("/Satoshi:1.14.4(2x)/") != std::string::npos
              || strSubVer.find("/Satoshi:1.15.0(2x)/") != std::string::npos
              || strSubVer.find("/Satoshi:1.15.1(2x)/") != std::string::npos
              || strSubVer.find("/Satoshi:1.15.1(2x; bitcore)/") != std::string::npos
              || strSubVer.find("/Satoshi:1.16.0/") != std::string::npos
              || strSubVer.find("/Satoshi:10.0.1/") != std::string::npos
              || strSubVer.find("/Bitcoin:0.16.1/") != std::string::npos
              || strSubVer.find("/SpuerBitcoin:0.16.0.2/") != std::string::npos
              || strSubVer.find("/SuperBitcoin:0.16.0.2/") != std::string::npos
              || strSubVer.find("/SuperBitcoin:0.17.0.1/") != std::string::npos
              || strSubVer.find("/BitcoinUnlimited:1.0.0.1(EB16; AD12)/") != std::string::npos
              || strSubVer.find("/BitcoinUnlimited:1.0.1.1(EB16; AD12)/") != std::string::npos
              || strSubVer.find("/BitcoinUnlimited:1.0.1.2(EB16; AD12)/") != std::string::npos
              || strSubVer.find("/BitcoinUnlimited:1.0.2(EB16; AD12)/") != std::string::npos
              || strSubVer.find("/BitcoinUnlimited:1.0.2(EB64; AD4)/") != std::string::npos
              || strSubVer.find("/BitcoinUnlimited:1.0.3(EB1; AD6)/") != std::string::npos
              || strSubVer.find("/BitcoinUnlimited:1.0.3(EB8; AD12)/") != std::string::npos
              || strSubVer.find("/BitcoinUnlimited:1.0.3(EB16; AD12)/") != std::string::npos
              || strSubVer.find("/BitcoinUnlimited:1.1.0.99(EB16; AD12)/") != std::string::npos
              || strSubVer.find("/BUCash:1.1.1.1(EB16; AD12)/") != std::string::npos
              || strSubVer.find("/BUCash:1.1.2(EB16; AD12)/") != std::string::npos
              || strSubVer.find("/BUCash:1.1.2(EB16; AD15)/") != std::string::npos
              || strSubVer.find("/BUCash:1.1.2(EB8; AD12)/more_nodes:1runBUxLk1MXAgcqFuXjUP1TEZ2AJCieW/") != std::string::npos
              || strSubVer.find("/BUCash:1.1.2(; AD12)/") != std::string::npos
              || strSubVer.find("/BUCash:1.1.2(EB512; AD2)/") != std::string::npos
              || strSubVer.find("/Bitcoin ABC:0.14.6(EB8.0; bitcore)/") != std::string::npos
              || strSubVer.find("/Bitcoin ABC:0.14.6(EB8.0)/") != std::string::npos
              || strSubVer.find("/Bitcoin ABC:0.15.0(EB8.0)/") != std::string::npos
              || strSubVer.find("/Bitcoin ABC:0.15.1(EB8.0)/") != std::string::npos
              || strSubVer.find("/Bitcoin ABC:0.16.0(EB8.0)/") != std::string::npos
              || strSubVer.find("/Bitcoin ABC:0.16.1(EB8.0)/") != std::string::npos
              || strSubVer.find("/Bitcoin ABC:0.16.0(EB8.0; bitcore)/") != std::string::npos
              || strSubVer.find("/Bitcoin ABC:0.16.1(EB8.0; bitcore)/") != std::string::npos
              || strSubVer.find("/Bitcoin Gold:0.15.0.1/") != std::string::npos
              || strSubVer.find("/CKCoind:0.15.1/") != std::string::npos
              || strSubVer.find("/CKCoind:0.16.0/") != std::string::npos
              || strSubVer.find("/btcwire:0.2.0/btcd:0.9.0/") != std::string::npos
              || strSubVer.find("/btcwire:0.4.0/btcd:0.12.0/") != std::string::npos
              || strSubVer.find("/btcwire:0.5.0/btcd:0.12.0/") != std::string::npos
              || strSubVer.find("/libbitcoin:3.4.0/") != std::string::npos
              || strSubVer.find("/WorldBitcoin:0.16.1/") != std::string::npos
              || strSubVer.find("/therealbitcoin.org:0.5.4.2/") != std::string::npos
              || strSubVer.find("/therealbitcoin.org:0.5.4.2/") != std::string::npos
              || strSubVer.find("/therealbitcoin.org:0.7.0.1/") != std::string::npos
              || strSubVer.find("/therealbitcoin.org:0.9.99.99/") != std::string::npos
              || strSubVer.find("/Cornell-Falcon-Network:0.1.0/") != std::string::npos
              || strSubVer.find("/Classic:1.2.0(EB3.7)/") != std::string::npos
              || strSubVer.find("/Classic:1.3.1(EB8)/") != std::string::npos
              || strSubVer.find("/Classic:1.3.3(EB8)/") != std::string::npos
              || strSubVer.find("/Classic:1.3.4(EB8)/") != std::string::npos
              || strSubVer.find("/Classic:1.3.8(EB8)/") != std::string::npos
              || strSubVer.find("/Bitcoin XBC:0.16.1(EB8.0)/") != std::string::npos
              || strSubVer.find("/Bitcoin XT:0.11.0/") != std::string::npos
              || strSubVer.find("/Bitcoin XT:0.11.0C/") != std::string::npos
              || strSubVer.find("/Bitcoin Core SQ:0.16.1(EB8.0)/") != std::string::npos
              || strSubVer.find("/bcoin:v1.0.0-pre/") != std::string::npos
              || strSubVer.find("/bcoin:v1.0.0-beta.12/") != std::string::npos
              || strSubVer.find("/Gocoin:1.9.5/") != std::string::npos
              || strSubVer.find("/QuantumBitcoin:0.16.0.2/") != std::string::npos
              || strSubVer.find("/Bitcoin World:0.15.0.1/") != std::string::npos
              || strSubVer.find("/Aurum:0.12.2/") != std::string::npos
              ) {
        ban = 100000;
        return true;
      }	
	
      if (nVersion >= 209 && !vRecv.empty())
        vRecv >> nStartingHeight;
      
      if (nVersion >= 209) {
        BeginMessage("verack");
        EndMessage();
      }
      vSend.SetVersion(min(nVersion, PROTOCOL_VERSION));
      if (nVersion < 209) {
        this->vRecv.SetVersion(min(nVersion, PROTOCOL_VERSION));
        GotVersion();
      }
      return false;
    }
    
    if (strCommand == "verack") {
      this->vRecv.SetVersion(min(nVersion, PROTOCOL_VERSION));
      GotVersion();
      return false;
    }
    
    if (strCommand == "addr" && vAddr) {
      vector<CAddress> vAddrNew;
      vRecv >> vAddrNew;
      // printf("%s: got %i addresses\n", ToString(you).c_str(), (int)vAddrNew.size());
      int64 now = time(NULL);
      vector<CAddress>::iterator it = vAddrNew.begin();
      if (vAddrNew.size() > 1) {
        if (doneAfter == 0 || doneAfter > now + 1) doneAfter = now + 1;
      }
      while (it != vAddrNew.end()) {
        CAddress &addr = *it;
//        printf("%s: got address %s\n", ToString(you).c_str(), addr.ToString().c_str(), (int)(vAddr->size()));
        it++;
        if (addr.nTime <= 100000000 || addr.nTime > now + 600)
          addr.nTime = now - 5 * 86400;
        if (addr.nTime > now - 604800)
          vAddr->push_back(addr);
//        printf("%s: added address %s (#%i)\n", ToString(you).c_str(), addr.ToString().c_str(), (int)(vAddr->size()));
        if (vAddr->size() > 1000) {doneAfter = 1; return true; }
      }
      return false;
    }
    
    return false;
  }
  
  bool ProcessMessages() {
    if (vRecv.empty()) return false;
    do {
      CDataStream::iterator pstart = search(vRecv.begin(), vRecv.end(), BEGIN(pchMessageStart), END(pchMessageStart));
      int nHeaderSize = vRecv.GetSerializeSize(CMessageHeader());
      if (vRecv.end() - pstart < nHeaderSize) {
        if (vRecv.size() > nHeaderSize) {
          vRecv.erase(vRecv.begin(), vRecv.end() - nHeaderSize);
        }
        break;
      }
      vRecv.erase(vRecv.begin(), pstart);
      vector<char> vHeaderSave(vRecv.begin(), vRecv.begin() + nHeaderSize);
      CMessageHeader hdr;
      vRecv >> hdr;
      if (!hdr.IsValid()) { 
        // printf("%s: BAD (invalid header)\n", ToString(you).c_str());
        ban = 100000; return true;
      }
      string strCommand = hdr.GetCommand();
      unsigned int nMessageSize = hdr.nMessageSize;
      if (nMessageSize > MAX_SIZE) { 
        // printf("%s: BAD (message too large)\n", ToString(you).c_str());
        ban = 100000;
        return true; 
      }
      if (nMessageSize > vRecv.size()) {
        vRecv.insert(vRecv.begin(), vHeaderSave.begin(), vHeaderSave.end());
        break;
      }
      if (vRecv.GetVersion() >= 209) {
        uint256 hash = Hash(vRecv.begin(), vRecv.begin() + nMessageSize);
        unsigned int nChecksum = 0;
        memcpy(&nChecksum, &hash, sizeof(nChecksum));
        if (nChecksum != hdr.nChecksum) continue;
      }
      CDataStream vMsg(vRecv.begin(), vRecv.begin() + nMessageSize, vRecv.nType, vRecv.nVersion);
      vRecv.ignore(nMessageSize);
      if (ProcessMessage(strCommand, vMsg))
        return true;
//      printf("%s: done processing %s\n", ToString(you).c_str(), strCommand.c_str());
    } while(1);
    return false;
  }
  
public:
  CNode(const CService& ip, vector<CAddress>* vAddrIn) : you(ip), nHeaderStart(-1), nMessageStart(-1), vAddr(vAddrIn), ban(0), doneAfter(0), nVersion(0) {
    vSend.SetType(SER_NETWORK);
    vSend.SetVersion(0);
    vRecv.SetType(SER_NETWORK);
    vRecv.SetVersion(0);
    if (time(NULL) > 1329696000) {
      vSend.SetVersion(209);
      vRecv.SetVersion(209);
    }
  }
  bool Run() {
    bool res = true;
    if (!ConnectSocket(you, sock)) return false;
    PushVersion();
    Send();
    int64 now;
    while (now = time(NULL), ban == 0 && (doneAfter == 0 || doneAfter > now) && sock != INVALID_SOCKET) {
      char pchBuf[0x10000];
      fd_set set;
      FD_ZERO(&set);
      FD_SET(sock,&set);
      struct timeval wa;
      if (doneAfter) {
        wa.tv_sec = doneAfter - now;
        wa.tv_usec = 0;
      } else {
        wa.tv_sec = GetTimeout();
        wa.tv_usec = 0;
      }
      int ret = select(sock+1, &set, NULL, &set, &wa);
      if (ret != 1) {
        if (!doneAfter) res = false;
        break;
      }
      int nBytes = recv(sock, pchBuf, sizeof(pchBuf), 0);
      int nPos = vRecv.size();
      if (nBytes > 0) {
        vRecv.resize(nPos + nBytes);
        memcpy(&vRecv[nPos], pchBuf, nBytes);
      } else if (nBytes == 0) {
        // printf("%s: BAD (connection closed prematurely)\n", ToString(you).c_str());
        res = false;
        break;
      } else {
        // printf("%s: BAD (connection error)\n", ToString(you).c_str());
        res = false;
        break;
      }
      ProcessMessages();
      Send();
    }
    if (sock == INVALID_SOCKET) res = false;
    close(sock);
    sock = INVALID_SOCKET;
    return (ban == 0) && res;
  }
  
  int GetBan() {
    return ban;
  }
  
  int GetClientVersion() {
    return nVersion;
  }
  
  std::string GetClientSubVersion() {
    return strSubVer;
  }
  
  int GetStartingHeight() {
    return nStartingHeight;
  }
};

bool TestNode(const CService &cip, int &ban, int &clientV, std::string &clientSV, int &blocks, vector<CAddress>* vAddr) {
  try {
    CNode node(cip, vAddr);
    bool ret = node.Run();
    if (!ret) {
      ban = node.GetBan();
    } else {
      ban = 0;
    }
    clientV = node.GetClientVersion();
    clientSV = node.GetClientSubVersion();
    blocks = node.GetStartingHeight();
//  printf("%s: %s!!!\n", cip.ToString().c_str(), ret ? "GOOD" : "BAD");
    return ret;
  } catch(std::ios_base::failure& e) {
    ban = 0;
    return false;
  }
}

/*
int main(void) {
  CService ip("bitsend.info", 8886, true);
  vector<CAddress> vAddr;
  vAddr.clear();
  int ban = 0;
  bool ret = TestNode(ip, ban, vAddr);
  printf("ret=%s ban=%i vAddr.size()=%i\n", ret ? "good" : "bad", ban, (int)vAddr.size());
}
*/

